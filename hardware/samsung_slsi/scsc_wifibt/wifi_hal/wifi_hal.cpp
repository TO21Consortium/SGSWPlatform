#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/attr.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>

#include <dirent.h>
#include <net/if.h>

#include "sync.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>
#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"


#define WIFI_HAL_CMD_SOCK_PORT       644
#define WIFI_HAL_EVENT_SOCK_PORT     645

#define FEATURE_SET                  0
#define FEATURE_SET_MATRIX           1
#define ATTR_NODFS_VALUE             3

static void internal_event_handler(wifi_handle handle, int events);
static int internal_no_seq_check(nl_msg *msg, void *arg);
static int internal_valid_message_handler(nl_msg *msg, void *arg);
static int wifi_get_multicast_id(wifi_handle handle, const char *name, const char *group);
static int wifi_add_membership(wifi_handle handle, const char *group);
static wifi_error wifi_init_interfaces(wifi_handle handle);

typedef enum wifi_attr {
    ANDR_WIFI_ATTRIBUTE_NUM_FEATURE_SET,
    ANDR_WIFI_ATTRIBUTE_FEATURE_SET,
    ANDR_WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI
} wifi_attr_t;

enum wifi_rssi_monitor_attr {
    RSSI_MONITOR_ATTRIBUTE_MAX_RSSI,
    RSSI_MONITOR_ATTRIBUTE_MIN_RSSI,
    RSSI_MONITOR_ATTRIBUTE_START,
};

static wifi_error wifi_start_rssi_monitoring(wifi_request_id id, wifi_interface_handle
                        iface, s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh);
static wifi_error wifi_stop_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface);

/* Initialize/Cleanup */

void wifi_socket_set_local_port(struct nl_sock *sock, uint32_t port)
{
    uint32_t pid = getpid() & 0x3FFFFF;
    nl_socket_set_local_port(sock, pid + (port << 22));
}

static nl_sock * wifi_create_nl_socket(int port)
{
     ALOGI("Creating socket");
    struct nl_sock *sock = nl_socket_alloc();
    if (sock == NULL) {
        ALOGE("Could not create handle");
        return NULL;
    }

    wifi_socket_set_local_port(sock, port);

    ALOGI("Connecting socket");
    if (nl_connect(sock, NETLINK_GENERIC)) {
        ALOGE("Could not connect handle");
        nl_socket_free(sock);
        return NULL;
    }

    return sock;
}

/* Initialize HAL function pointer table */
wifi_error init_wifi_vendor_hal_func_table(wifi_hal_fn *fn)
{
    if (fn == NULL) {
        return WIFI_ERROR_UNKNOWN;
    }
    fn->wifi_initialize = wifi_initialize;
    fn->wifi_cleanup = wifi_cleanup;
    fn->wifi_event_loop = wifi_event_loop;
    fn->wifi_get_supported_feature_set = wifi_get_supported_feature_set;
    fn->wifi_get_concurrency_matrix = wifi_get_concurrency_matrix;
    fn->wifi_set_scanning_mac_oui =  wifi_set_scanning_mac_oui;
    fn->wifi_get_ifaces = wifi_get_ifaces;
    fn->wifi_get_iface_name = wifi_get_iface_name;
    fn->wifi_start_gscan = wifi_start_gscan;
    fn->wifi_stop_gscan = wifi_stop_gscan;
    fn->wifi_get_cached_gscan_results = wifi_get_cached_gscan_results;
    fn->wifi_set_bssid_hotlist = wifi_set_bssid_hotlist;
    fn->wifi_reset_bssid_hotlist = wifi_reset_bssid_hotlist;
    fn->wifi_set_significant_change_handler = wifi_set_significant_change_handler;
    fn->wifi_reset_significant_change_handler = wifi_reset_significant_change_handler;
    fn->wifi_get_gscan_capabilities = wifi_get_gscan_capabilities;
    fn->wifi_get_valid_channels = wifi_get_valid_channels;
    fn->wifi_rtt_range_request = wifi_rtt_range_request;
    fn->wifi_rtt_range_cancel = wifi_rtt_range_cancel;
    fn->wifi_get_rtt_capabilities = wifi_get_rtt_capabilities;
    fn->wifi_set_nodfs_flag = wifi_set_nodfs_flag;
    fn->wifi_start_sending_offloaded_packet = wifi_start_sending_offloaded_packet;
    fn->wifi_stop_sending_offloaded_packet = wifi_stop_sending_offloaded_packet;
    fn->wifi_set_epno_list = wifi_set_epno_list;
    fn->wifi_set_passpoint_list = wifi_set_passpoint_list;
    fn->wifi_reset_passpoint_list = wifi_reset_passpoint_list;
    fn->wifi_set_bssid_blacklist = wifi_set_bssid_blacklist;
    fn->wifi_start_rssi_monitoring = wifi_start_rssi_monitoring;
    fn->wifi_stop_rssi_monitoring = wifi_stop_rssi_monitoring;
    fn->wifi_set_link_stats = wifi_set_link_stats;
    fn->wifi_get_link_stats = wifi_get_link_stats;
    fn->wifi_clear_link_stats = wifi_clear_link_stats;

    return WIFI_SUCCESS;
}

wifi_error wifi_initialize(wifi_handle *handle)
{
    srand(getpid());

    ALOGI("Initializing wifi");
    hal_info *info = (hal_info *)malloc(sizeof(hal_info));
    if (info == NULL) {
        ALOGE("Could not allocate hal_info");
        return WIFI_ERROR_UNKNOWN;
    }

    memset(info, 0, sizeof(*info));

    ALOGI("Creating socket");
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, info->cleanup_socks) == -1) {
        ALOGE("Could not create cleanup sockets");
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    struct nl_sock *cmd_sock = wifi_create_nl_socket(WIFI_HAL_CMD_SOCK_PORT);
    if (cmd_sock == NULL) {
        ALOGE("Could not create handle");
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    struct nl_sock *event_sock = wifi_create_nl_socket(WIFI_HAL_EVENT_SOCK_PORT);
    if (event_sock == NULL) {
        ALOGE("Could not create handle");
        nl_socket_free(cmd_sock);
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    struct nl_cb *cb = nl_socket_get_cb(event_sock);
    if (cb == NULL) {
        ALOGE("Could not create handle");
        nl_socket_free(cmd_sock);
        nl_socket_free(event_sock);
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

//     ALOGI("cb->refcnt = %d", cb->cb_refcnt);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, internal_no_seq_check, info);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, internal_valid_message_handler, info);
    nl_cb_put(cb);

    info->cmd_sock = cmd_sock;
    info->event_sock = event_sock;
    info->clean_up = false;
    info->in_event_loop = false;

    info->event_cb = (cb_info *)malloc(sizeof(cb_info) * DEFAULT_EVENT_CB_SIZE);
    info->alloc_event_cb = DEFAULT_EVENT_CB_SIZE;
    info->num_event_cb = 0;

    info->cmd = (cmd_info *)malloc(sizeof(cmd_info) * DEFAULT_CMD_SIZE);
    info->alloc_cmd = DEFAULT_CMD_SIZE;
    info->num_cmd = 0;

    info->nl80211_family_id = genl_ctrl_resolve(cmd_sock, "nl80211");
    if (info->nl80211_family_id < 0) {
        ALOGE("Could not resolve nl80211 familty id");
        nl_socket_free(cmd_sock);
        nl_socket_free(event_sock);
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    pthread_mutex_init(&info->cb_lock, NULL);

    *handle = (wifi_handle) info;
	    ALOGD("wifi_initialize, handle = %p\n", handle);
	    ALOGD("wifi_initialize, *handle = %p\n", *handle);
		ALOGD("wifi_initialize, info = %p\n", info);
		ALOGD("wifi_initialize, *info = %pn", *info);
    wifi_add_membership(*handle, "scan");
    wifi_add_membership(*handle, "mlme");
    wifi_add_membership(*handle, "regulatory");
    wifi_add_membership(*handle, "vendor");

    wifi_init_interfaces(*handle);
     ALOGD("Found %d interfaces", info->num_interfaces);


    ALOGI("Initialized Wifi HAL Successfully; vendor cmd = %d", NL80211_CMD_VENDOR);
    return WIFI_SUCCESS;
}

static int wifi_add_membership(wifi_handle handle, const char *group)
{
    hal_info *info = getHalInfo(handle);

    int id = wifi_get_multicast_id(handle, "nl80211", group);
    if (id < 0) {
        ALOGE("Could not find group %s", group);
        return id;
    }

    int ret = nl_socket_add_membership(info->event_sock, id);
    if (ret < 0) {
        ALOGE("Could not add membership to group %s", group);
    }

     ALOGI("Successfully added membership for group %s", group);
    return ret;
}

static void internal_cleaned_up_handler(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    wifi_cleaned_up_handler cleaned_up_handler = info->cleaned_up_handler;

    if (info->cmd_sock != 0) {
        close(info->cleanup_socks[0]);
        close(info->cleanup_socks[1]);
        nl_socket_free(info->cmd_sock);
        nl_socket_free(info->event_sock);
        info->cmd_sock = NULL;
        info->event_sock = NULL;
    }

    (*cleaned_up_handler)(handle);
    pthread_mutex_destroy(&info->cb_lock);
    free(info);

    ALOGI("Internal cleanup completed");
}

void wifi_cleanup(wifi_handle handle, wifi_cleaned_up_handler handler)
{
    hal_info *info = getHalInfo(handle);
    char buf[64];

    info->cleaned_up_handler = handler;
    if (write(info->cleanup_socks[0], "Exit", 4) < 1) {
        ALOGE("could not write to the cleanup socket");
    } else {
        // Listen to the response
        // Hopefully we dont get errors or get hung up
        // Not much can be done in that case, but assume that
        // it has rx'ed the Exit message to exit the thread.
        // As a fallback set the cleanup flag to TRUE
        memset(buf, 0, sizeof(buf));
        int result = read(info->cleanup_socks[0], buf, sizeof(buf));
        ALOGE("%s: Read after POLL returned %d, error no = %d", __FUNCTION__, result, errno);
        if (strncmp(buf, "Done", 4) == 0) {
            ALOGE("Event processing terminated");
        } else {
            ALOGD("Rx'ed %s", buf);
        }
    }
    info->clean_up = true;
    pthread_mutex_lock(&info->cb_lock);

    int bad_commands = 0;

    while (info->num_cmd > bad_commands) {
        int num_cmd = info->num_cmd;
        cmd_info *cmdi = &(info->cmd[bad_commands]);
        WifiCommand *cmd = cmdi->cmd;
        if (cmd != NULL) {
            pthread_mutex_unlock(&info->cb_lock);
            cmd->cancel();
            pthread_mutex_lock(&info->cb_lock);
            /* release reference added when command is saved */
            cmd->releaseRef();
            if (num_cmd == info->num_cmd) {
                bad_commands++;
            }
        }
    }

    for (int i = 0; i < info->num_event_cb; i++) {
        cb_info *cbi = &(info->event_cb[i]);
        WifiCommand *cmd = (WifiCommand *)cbi->cb_arg;
        ALOGE("Leaked command %p", cmd);
    }
    pthread_mutex_unlock(&info->cb_lock);
    internal_cleaned_up_handler(handle);
}

static int internal_pollin_handler(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
		ALOGI("even_loop info = %p", info);
    struct nl_cb *cb = nl_socket_get_cb(info->event_sock);
    int res = nl_recvmsgs(info->event_sock, cb);
     ALOGD("nl_recvmsgs returned %d", res);
    nl_cb_put(cb);
    return res;
}

/* Run event handler */
void wifi_event_loop(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
	ALOGI("even_loop info = %p", info);
	ALOGI("even_loop info = %p", handle);
    if (info->in_event_loop) {
        return;
    } else {
        info->in_event_loop = true;
    }

    pollfd pfd[2];
    memset(&pfd[0], 0, sizeof(pollfd) * 2);

    pfd[0].fd = nl_socket_get_fd(info->event_sock);
    pfd[0].events = POLLIN;
    pfd[1].fd = info->cleanup_socks[1];
    pfd[1].events = POLLIN;

    char buf[2048];

    do {
        int timeout = -1;                   /* Infinite timeout */
        pfd[0].revents = 0;
        pfd[1].revents = 0;
        int result = poll(pfd, 2, timeout);
        if (result < 0) {
        } else if (pfd[0].revents & POLLERR) {
            ALOGE("POLL Error; error no = %d", errno);
            int result2 = read(pfd[0].fd, buf, sizeof(buf));
            ALOGE("Read after POLL returned %d, error no = %d", result2, errno);
        } else if (pfd[0].revents & POLLHUP) {
            ALOGE("Remote side hung up");
            break;
        } else if (pfd[0].revents & POLLIN) {
            internal_pollin_handler(handle);
        } else if (pfd[1].revents & POLLIN) {
            memset(buf, 0, sizeof(buf));
            int result2 = read(pfd[1].fd, buf, sizeof(buf));
            ALOGE("%s: Read after POLL returned %d, error no = %d", __FUNCTION__, result2, errno);
            if (strncmp(buf, "Exit", 4) == 0) {
                ALOGD("Got a signal to exit!!!");
                if (write(pfd[1].fd, "Done", 4) < 1) {
                    ALOGE("could not write to the cleanup socket");
                }
                break;
            } else {
                ALOGD("Rx'ed %s on the cleanup socket\n", buf);
            }
        } else {
            ALOGE("Unknown event - %0x, %0x", pfd[0].revents, pfd[1].revents);
        }
    } while (!info->clean_up);
    ALOGI("Exit %s", __FUNCTION__);
}

///////////////////////////////////////////////////////////////////////////////////////

static int internal_no_seq_check(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

static int internal_valid_message_handler(nl_msg *msg, void *arg)
{
    wifi_handle handle = (wifi_handle)arg;
    hal_info *info = getHalInfo(handle);
   ALOGI("even_loop info = %p", handle);
   ALOGD("internal_valid_message_handler, info = %p", info);

    WifiEvent event(msg);
    int res = event.parse();
    if (res < 0) {
        ALOGE("Failed to parse event: %d", res);
        return NL_SKIP;
    }

    int cmd = event.get_cmd();
    uint32_t vendor_id = 0;
    int subcmd = 0;

    if (cmd == NL80211_CMD_VENDOR) {
        vendor_id = event.get_u32(NL80211_ATTR_VENDOR_ID);
        subcmd = event.get_u32(NL80211_ATTR_VENDOR_SUBCMD);
        ALOGI("event received %s, vendor_id = 0x%0x, subcmd = 0x%0x",
                event.get_cmdString(), vendor_id, subcmd);
    } else {
         ALOGI("event received %s", event.get_cmdString());
    }

     //ALOGI("event received %s, vendor_id = 0x%0x", event.get_cmdString(), vendor_id);
     //event.log();

    pthread_mutex_lock(&info->cb_lock);

    ALOGI("Number of events %d", info->num_event_cb);

    for (int i = 0; i < info->num_event_cb; i++) {
        if (cmd == info->event_cb[i].nl_cmd) {
            if (cmd == NL80211_CMD_VENDOR
                && ((vendor_id != info->event_cb[i].vendor_id)
                || (subcmd != info->event_cb[i].vendor_subcmd)))
            {
                /* event for a different vendor, ignore it */
                continue;
            }

            cb_info *cbi = &(info->event_cb[i]);
            nl_recvmsg_msg_cb_t cb_func = cbi->cb_func;
            void *cb_arg = cbi->cb_arg;
            WifiCommand *cmd = (WifiCommand *)cbi->cb_arg;
            if (cmd != NULL) {
                cmd->addRef();
            }

            pthread_mutex_unlock(&info->cb_lock);
            if (cb_func)
                (*cb_func)(msg, cb_arg);
            if (cmd != NULL) {
                cmd->releaseRef();
            }

            return NL_OK;
        }
    }

    pthread_mutex_unlock(&info->cb_lock);
    return NL_OK;
}

///////////////////////////////////////////////////////////////////////////////////////

class GetMulticastIdCommand : public WifiCommand
{
private:
    const char *mName;
    const char *mGroup;
    int   mId;
public:
    GetMulticastIdCommand(wifi_handle handle, const char *name, const char *group)
        : WifiCommand(handle, 0)
    {
        mName = name;
        mGroup = group;
        mId = -1;
    }

    int getId() {
        return mId;
    }

    virtual int create() {
        int nlctrlFamily = genl_ctrl_resolve(mInfo->cmd_sock, "nlctrl");
        ALOGI("ctrl family = %d", nlctrlFamily);
        int ret = mMsg.create(nlctrlFamily, CTRL_CMD_GETFAMILY, 0, 0);
        if (ret < 0) {
            return ret;
        }
        ret = mMsg.put_string(CTRL_ATTR_FAMILY_NAME, mName);
        return ret;
    }

    virtual int handleResponse(WifiEvent& reply) {

         ALOGE("handling reponse in %s", __func__);

        struct nlattr **tb = reply.attributes();
        struct nlattr *mcgrp = NULL;
        int i;

        if (!tb[CTRL_ATTR_MCAST_GROUPS]) {
            ALOGE("No multicast groups found");
            return NL_SKIP;
        } else {
             ALOGE("Multicast groups attr size = %d", nla_len(tb[CTRL_ATTR_MCAST_GROUPS]));
        }

        for_each_attr(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], i) {

             ALOGE("Processing group");
            struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];
            nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, (nlattr *)nla_data(mcgrp),
                nla_len(mcgrp), NULL);
            if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] || !tb2[CTRL_ATTR_MCAST_GRP_ID]) {
                continue;
            }

            char *grpName = (char *)nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]);
            int grpNameLen = nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME]);

             ALOGE("Found group name %s", grpName);

            if (strncmp(grpName, mGroup, grpNameLen) != 0)
                continue;

            mId = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);
            break;
        }

        return NL_SKIP;
    }

};

class SetPnoMacAddrOuiCommand : public WifiCommand {

private:
    byte *mOui;
    feature_set *fset;
    feature_set *feature_matrix;
    int *fm_size;
    int set_size_max;
public:
    SetPnoMacAddrOuiCommand(wifi_interface_handle handle, oui scan_oui)
        : WifiCommand(handle, 0)
    {
        mOui = scan_oui;
    }

    int createRequest(WifiRequest& request, int subcmd, byte *scan_oui) {
        int result = request.create(GOOGLE_OUI, subcmd);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put(ANDR_WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI, scan_oui, DOT11_OUI_LEN);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;

    }

    int start() {
        ALOGD("Sending mac address OUI");
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, SLSI_NL80211_VENDOR_SUBCMD_SET_GSCAN_OUI, mOui);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to set scanning mac OUI; result = %d", result);
        }

        return result;
    }
protected:
    virtual int handleResponse(WifiEvent& reply) {
         ALOGD("Request complete!");
        /* Nothing to do on response! */
        return NL_SKIP;
    }
};

class SetNodfsCommand : public WifiCommand {

private:
    u32 mNoDfs;
public:
    SetNodfsCommand(wifi_interface_handle handle, u32 nodfs)
        : WifiCommand(handle, 0) {
        mNoDfs = nodfs;
    }
    virtual int create() {
        int ret;

        ret = mMsg.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_SET_NODFS);
        if (ret < 0) {
            ALOGE("Can't create message to send to driver - %d", ret);
            return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(ATTR_NODFS_VALUE, mNoDfs);
        if (ret < 0) {
             return ret;
        }

        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

class SetRSSIMonitorCommand : public WifiCommand {
private:
    s8 mMax_rssi;
    s8 mMin_rssi;
    wifi_rssi_event_handler mHandler;
public:
    SetRSSIMonitorCommand(wifi_request_id id, wifi_interface_handle handle,
                s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh)
        : WifiCommand(handle, id), mMax_rssi(max_rssi), mMin_rssi
        (min_rssi), mHandler(eh)
        {
        }
   int createRequest(WifiRequest& request, int enable) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_SET_RSSI_MONITOR);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(RSSI_MONITOR_ATTRIBUTE_MAX_RSSI, (enable ? mMax_rssi: 0));
        if (result < 0) {
            return result;
        }
        ALOGD("create request");
        result = request.put_u8(RSSI_MONITOR_ATTRIBUTE_MIN_RSSI, (enable? mMin_rssi: 0));
        if (result < 0) {
            return result;
        }
        result = request.put_u8(RSSI_MONITOR_ATTRIBUTE_START, enable);
        if (result < 0) {
            return result;
        }
        request.attr_end(data);
        return result;
    }

    int start() {
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, 1);
        if (result < 0) {
            return result;
        }
        result = requestResponse(request);
        if (result < 0) {
            ALOGI("Failed to set RSSI Monitor, result = %d", result);
            return result;
        }
        ALOGI("Successfully set RSSI monitoring");
        registerVendorHandler(GOOGLE_OUI, WIFI_RSSI_REPORT_EVENT);


        if (result < 0) {
            unregisterVendorHandler(GOOGLE_OUI, WIFI_RSSI_REPORT_EVENT);
            return result;
        }
        ALOGI("Done!");
        return result;
    }

    virtual int cancel() {

        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, 0);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to stop RSSI monitoring = %d", result);
            }
        }
        unregisterVendorHandler(GOOGLE_OUI, WIFI_RSSI_REPORT_EVENT);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

   virtual int handleEvent(WifiEvent& event) {
        ALOGI("Got a RSSI monitor event");

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGI("RSSI monitor: No data");
            return NL_SKIP;
        }

        typedef struct {
            s8 cur_rssi;
            mac_addr BSSID;
        } rssi_monitor_evt;

        rssi_monitor_evt *data = (rssi_monitor_evt *)event.get_vendor_data();

        if (*mHandler.on_rssi_threshold_breached) {
            (*mHandler.on_rssi_threshold_breached)(id(), data->BSSID, data->cur_rssi);
        } else {
            ALOGW("No RSSI monitor handler registered");
        }

        return NL_SKIP;
    }

};

class GetFeatureSetCommand : public WifiCommand {

private:
    int feature_type;
    feature_set *fset;
    int *fm_size;
    int set_size_max;
public:
    GetFeatureSetCommand(wifi_interface_handle handle, feature_set *set)
        : WifiCommand(handle, 0)
    {
        fset = set;
    }

    virtual int create() {
        int ret;

        ret = mMsg.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_GET_FEATURE_SET);
        if (ret < 0) {
            ALOGE("create failed - %d", ret);
        }

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignore reply; cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGE("vendor data in GetFeatureSetCommand missing!!");
            return NL_SKIP;
        }

        void *data = reply.get_vendor_data();
        if(!fset) {
            ALOGE("feature_set Pointer not set");
            return NL_SKIP;
        }
        memcpy(fset, data, min(len, (int) sizeof(*fset)));
        return NL_OK;
    }

};

static int wifi_get_multicast_id(wifi_handle handle, const char *name, const char *group)
{
    GetMulticastIdCommand cmd(handle, name, group);
    int res = cmd.requestResponse();
    if (res < 0)
        return res;
    else
        return cmd.getId();
}

/////////////////////////////////////////////////////////////////////////

static bool is_wifi_interface(const char *name)
{
    if (strncmp(name, "wlan", 4) != 0 && strncmp(name, "p2p", 3) != 0) {
        /* not a wifi interface; ignore it */
        return false;
    } else {
        return true;
    }
}

static int get_interface(const char *name, interface_info *info)
{
    strcpy(info->name, name);
    info->id = if_nametoindex(name);
     ALOGI("found an interface : %s, id = %d", name, info->id);
    return WIFI_SUCCESS;
}

wifi_error wifi_init_interfaces(wifi_handle handle)
{
    hal_info *info = (hal_info *)handle;
	ALOGD("wifi_init_interfaces, info = %p", info);

    struct dirent *de;

    DIR *d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    int n = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name) ) {
            n++;
        }
    }

    closedir(d);

    d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    info->interfaces = (interface_info **)malloc(sizeof(interface_info *) * n);

    int i = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name)) {
            interface_info *ifinfo = (interface_info *)malloc(sizeof(interface_info));
            if (get_interface(de->d_name, ifinfo) != WIFI_SUCCESS) {
                free(ifinfo);
                continue;
            }
            ifinfo->handle = handle;
            info->interfaces[i] = ifinfo;
            i++;
        }
    }

    closedir(d);

    info->num_interfaces = n;
    return WIFI_SUCCESS;
}

wifi_error wifi_get_ifaces(wifi_handle handle, int *num, wifi_interface_handle **interfaces)
{
    hal_info *info = (hal_info *)handle;

    *interfaces = (wifi_interface_handle *)info->interfaces;
    *num = info->num_interfaces;

    return WIFI_SUCCESS;
}

wifi_error wifi_get_iface_name(wifi_interface_handle handle, char *name, size_t size)
{
    interface_info *info = (interface_info *)handle;
    strcpy(name, info->name);
    return WIFI_SUCCESS;
}

wifi_error wifi_get_concurrency_matrix(wifi_interface_handle handle, int set_size_max,
       feature_set set[], int *set_size)
{
    return WIFI_ERROR_NOT_SUPPORTED;
}

wifi_error wifi_set_scanning_mac_oui(wifi_interface_handle handle, oui scan_oui)
{
    SetPnoMacAddrOuiCommand command(handle, scan_oui);
    return (wifi_error)command.start();

}

wifi_error wifi_set_nodfs_flag(wifi_interface_handle handle, u32 nodfs)
{
    SetNodfsCommand command(handle, nodfs);
    return (wifi_error) command.requestResponse();
}

static wifi_error wifi_start_rssi_monitoring(wifi_request_id id, wifi_interface_handle
                        iface, s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh)
{
    ALOGD("Start RSSI monitor %d", id);
    wifi_handle handle = getWifiHandle(iface);
    SetRSSIMonitorCommand *cmd = new SetRSSIMonitorCommand(id, iface, max_rssi, min_rssi, eh);
    wifi_register_cmd(handle, id, cmd);

    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}


static wifi_error wifi_stop_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface)
{
    ALOGD("Stopping RSSI monitor");

    if(id == -1) {
        wifi_rssi_event_handler handler;
        wifi_handle handle = getWifiHandle(iface);
        memset(&handler, 0, sizeof(handler));
        SetRSSIMonitorCommand *cmd = new SetRSSIMonitorCommand(id, iface,
                                                    0, 0, handler);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }
    return wifi_cancel_cmd(id, iface);
}

wifi_error wifi_get_supported_feature_set(wifi_interface_handle handle, feature_set *set)
{
    GetFeatureSetCommand command(handle, set);
    return (wifi_error) command.requestResponse();
}

/////////////////////////////////////////////////////////////////////////////
