
#include <stdint.h>
#include <stddef.h>
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
#include <netlink/handlers.h>

#include "sync.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

typedef enum {

    GSCAN_ATTRIBUTE_NUM_BUCKETS = 10,
    GSCAN_ATTRIBUTE_BASE_PERIOD,
    GSCAN_ATTRIBUTE_BUCKETS_BAND,
    GSCAN_ATTRIBUTE_BUCKET_ID,
    GSCAN_ATTRIBUTE_BUCKET_PERIOD,
    GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS,
    GSCAN_ATTRIBUTE_BUCKET_CHANNELS,
    GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN,
    GSCAN_ATTRIBUTE_REPORT_THRESHOLD,
    GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE,
    GSCAN_ATTRIBUTE_REPORT_THRESHOLD_NUM_SCANS,
    GSCAN_ATTRIBUTE_BAND = GSCAN_ATTRIBUTE_BUCKETS_BAND,

    GSCAN_ATTRIBUTE_ENABLE_FEATURE = 20,
    GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE,              /* indicates no more results */
    GSCAN_ATTRIBUTE_REPORT_EVENTS,

    /* remaining reserved for additional attributes */
    GSCAN_ATTRIBUTE_NUM_OF_RESULTS = 30,
    GSCAN_ATTRIBUTE_SCAN_RESULTS,                       /* flat array of wifi_scan_result */
    GSCAN_ATTRIBUTE_NUM_CHANNELS,
    GSCAN_ATTRIBUTE_CHANNEL_LIST,
    GSCAN_ATTRIBUTE_SCAN_ID,
    GSCAN_ATTRIBUTE_SCAN_FLAGS,

    /* remaining reserved for additional attributes */

    GSCAN_ATTRIBUTE_SSID = 40,
    GSCAN_ATTRIBUTE_BSSID,
    GSCAN_ATTRIBUTE_CHANNEL,
    GSCAN_ATTRIBUTE_RSSI,
    GSCAN_ATTRIBUTE_TIMESTAMP,
    GSCAN_ATTRIBUTE_RTT,
    GSCAN_ATTRIBUTE_RTTSD,

    /* remaining reserved for additional attributes */

    GSCAN_ATTRIBUTE_HOTLIST_BSSIDS = 50,
    GSCAN_ATTRIBUTE_RSSI_LOW,
    GSCAN_ATTRIBUTE_RSSI_HIGH,
    GSCAN_ATTRIBUTE_HOTLIST_ELEM,
    GSCAN_ATTRIBUTE_HOTLIST_FLUSH,
    GSCAN_ATTRIBUTE_CHANNEL_NUMBER,

    /* remaining reserved for additional attributes */
    GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE = 60,
    GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE,
    GSCAN_ATTRIBUTE_MIN_BREACHING,
    GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_BSSIDS,

    GSCAN_ATTRIBUTE_BUCKET_STEP_COUNT = 70,
    GSCAN_ATTRIBUTE_BUCKET_EXPONENT,
    GSCAN_ATTRIBUTE_BUCKET_MAX_PERIOD,

    GSCAN_ATTRIBUTE_NUM_BSSID,
    GSCAN_ATTRIBUTE_BLACKLIST_BSSID,

    GSCAN_ATTRIBUTE_MAX

} GSCAN_ATTRIBUTE;

typedef enum {
    EPNO_ATTRIBUTE_SSID_LIST,
    EPNO_ATTRIBUTE_SSID_NUM,
    EPNO_ATTRIBUTE_SSID,
    EPNO_ATTRIBUTE_SSID_LEN,
    EPNO_ATTRIBUTE_RSSI,
    EPNO_ATTRIBUTE_FLAGS,
    EPNO_ATTRIBUTE_AUTH,
    EPNO_ATTRIBUTE_MAX
} EPNO_ATTRIBUTE;

typedef enum {
    EPNO_ATTRIBUTE_HS_PARAM_LIST,
    EPNO_ATTRIBUTE_HS_NUM,
    EPNO_ATTRIBUTE_HS_ID,
    EPNO_ATTRIBUTE_HS_REALM,
    EPNO_ATTRIBUTE_HS_CONSORTIUM_IDS,
    EPNO_ATTRIBUTE_HS_PLMN,
    EPNO_ATTRIBUTE_HS_MAX
} EPNO_HS_ATTRIBUTE;


class GetCapabilitiesCommand : public WifiCommand
{
    wifi_gscan_capabilities *mCapabilities;
public:
    GetCapabilitiesCommand(wifi_interface_handle iface, wifi_gscan_capabilities *capabitlites)
        : WifiCommand(iface, 0), mCapabilities(capabitlites)
    {
        memset(mCapabilities, 0, sizeof(*mCapabilities));
    }

    virtual int create() {
        ALOGD("Creating message to get scan capablities; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_GET_CAPABILITIES);
        if (ret < 0) {
	    ALOGD("NL message creation failed");
            return ret;
        }

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        ALOGD("In GetCapabilities::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        void *data = reply.get_vendor_data();
        int len = reply.get_vendor_data_len();

        ALOGD("Id = %0x, subcmd = %d, len = %d, expected len = %d", id, subcmd, len,
                    sizeof(*mCapabilities));

        memcpy(mCapabilities, data, min(len, (int) sizeof(*mCapabilities)));

        return NL_OK;
    }
};


wifi_error wifi_get_gscan_capabilities(wifi_interface_handle handle,
        wifi_gscan_capabilities *capabilities)
{
    GetCapabilitiesCommand command(handle, capabilities);
    return (wifi_error) command.requestResponse();
}

class GetChannelListCommand : public WifiCommand
{
    wifi_channel *channels;
    int max_channels;
    int *num_channels;
    int band;
public:
    GetChannelListCommand(wifi_interface_handle iface, wifi_channel *channel_buf, int *ch_num,
        int num_max_ch, int band)
        : WifiCommand(iface, 0), channels(channel_buf), max_channels(num_max_ch), num_channels(ch_num),
        band(band)
    {
        memset(channels, 0, sizeof(wifi_channel) * max_channels);
    }
    virtual int create() {
        ALOGD("Creating message to get channel list; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_GET_VALID_CHANNELS);
        if (ret < 0) {
            return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(GSCAN_ATTRIBUTE_BAND, band);
        if (ret < 0) {
            return ret;
        }

        mMsg.attr_end(data);

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        ALOGD("In GetChannelList::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();
        int num_channels_to_copy = 0;

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        ALOGD("Id = %0x, subcmd = %d, len = %d", id, subcmd, len);
        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetChannelList response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == GSCAN_ATTRIBUTE_NUM_CHANNELS) {
                num_channels_to_copy = it.get_u32();
                ALOGD("Got channel list with %d channels", num_channels_to_copy);
                if(num_channels_to_copy > max_channels)
                    num_channels_to_copy = max_channels;
                *num_channels = num_channels_to_copy;
            } else if (it.get_type() == GSCAN_ATTRIBUTE_CHANNEL_LIST && num_channels_to_copy) {
                memcpy(channels, it.get_data(), sizeof(int) * num_channels_to_copy);
            } else {
                ALOGW("Ignoring invalid attribute type = %d, size = %d",
                        it.get_type(), it.get_len());
            }
        }

        return NL_OK;
    }
};

wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
        int band, int max_channels, wifi_channel *channels, int *num_channels)
{
    GetChannelListCommand command(handle, channels, num_channels,
                                        max_channels, band);
    return (wifi_error) command.requestResponse();
}
/////////////////////////////////////////////////////////////////////////////

/* helper functions */

static int parseScanResults(wifi_scan_result *results, int num, nlattr *attr)
{
    memset(results, 0, sizeof(wifi_scan_result) * num);

    int i = 0;
    for (nl_iterator it(attr); it.has_next() && i < num; it.next(), i++) {

        int index = it.get_type();
        ALOGD("retrieved scan result %d", index);
        nlattr *sc_data = (nlattr *) it.get_data();
        wifi_scan_result *result = results + i;

        for (nl_iterator it2(sc_data); it2.has_next(); it2.next()) {
            int type = it2.get_type();
            if (type == GSCAN_ATTRIBUTE_SSID) {
                strncpy(result->ssid, (char *) it2.get_data(), it2.get_len());
                result->ssid[it2.get_len()] = 0;
            } else if (type == GSCAN_ATTRIBUTE_BSSID) {
                memcpy(result->bssid, (byte *) it2.get_data(), sizeof(mac_addr));
            } else if (type == GSCAN_ATTRIBUTE_TIMESTAMP) {
                result->ts = it2.get_u64();
            } else if (type == GSCAN_ATTRIBUTE_CHANNEL) {
                result->ts = it2.get_u16();
            } else if (type == GSCAN_ATTRIBUTE_RSSI) {
                result->rssi = it2.get_u8();
            } else if (type == GSCAN_ATTRIBUTE_RTT) {
                result->rtt = it2.get_u64();
            } else if (type == GSCAN_ATTRIBUTE_RTTSD) {
                result->rtt_sd = it2.get_u64();
            }
        }

    }

    if (i >= num) {
        ALOGE("Got too many results; skipping some");
    }

    return i;
}

int createFeatureRequest(WifiRequest& request, int subcmd) {

    int result = request.create(GOOGLE_OUI, subcmd);
    if (result < 0) {
        return result;
    }

    return WIFI_SUCCESS;
}

class ScanCommand : public WifiCommand
{
    wifi_scan_cmd_params *mParams;
    wifi_scan_result_handler mHandler;
    static unsigned mGlobalFullScanBuckets;
    bool mLocalFullScanBuckets;
public:
    ScanCommand(wifi_interface_handle iface, int id, wifi_scan_cmd_params *params,
                wifi_scan_result_handler handler)
        : WifiCommand(iface, id), mParams(params), mHandler(handler),
          mLocalFullScanBuckets(0)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_ADD_GSCAN);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_BASE_PERIOD, mParams->base_period);
        if (result < 0) {
            return result;
        }

	result = request.put_u32(GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN, mParams->max_ap_per_scan);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_REPORT_THRESHOLD, mParams->report_threshold_percent);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_REPORT_THRESHOLD_NUM_SCANS, mParams->report_threshold_num_scans);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_BUCKETS, mParams->num_buckets);
        if (result < 0) {
            return result;
        }

        for (int i = 0; i < mParams->num_buckets; i++) {
            nlattr * bucket = request.attr_start(i);    // next bucket
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_ID, mParams->buckets[i].bucket);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_PERIOD, mParams->buckets[i].period);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKETS_BAND,
                    mParams->buckets[i].band);
            if (result < 0) {
                return result;
            }

            result = request.put_u32(GSCAN_ATTRIBUTE_REPORT_EVENTS,
                    mParams->buckets[i].report_events);
            if (result < 0) {
                return result;
            }

            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS,
                    mParams->buckets[i].num_channels);
            if (result < 0) {
                return result;
            }

            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_EXPONENT,
                    mParams->buckets[i].exponent);
            if (result < 0) {
                return result;
            }

            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_MAX_PERIOD,
                    mParams->buckets[i].max_period);
            if (result < 0) {
                return result;
            }

            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_STEP_COUNT,
                    mParams->buckets[i].step_count);
            if (result < 0) {
                return result;
            }

            if (mParams->buckets[i].num_channels) {
                nlattr *channels = request.attr_start(GSCAN_ATTRIBUTE_BUCKET_CHANNELS);
                for (int j = 0; j < mParams->buckets[i].num_channels; j++) {
                    result = request.put_u32(j, mParams->buckets[i].channels[j].channel);
                    if (result < 0) {
                        return result;
                    }
                }
                request.attr_end(channels);
            }

            request.attr_end(bucket);
        }

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createStartRequest(WifiRequest& request) {
        return createFeatureRequest(request, SLSI_NL80211_VENDOR_SUBCMD_ADD_GSCAN);
    }

    int createStopRequest(WifiRequest& request) {
        return createFeatureRequest(request, SLSI_NL80211_VENDOR_SUBCMD_DEL_GSCAN);
    }

    int start() {
        ALOGD(" sending scan req to driver");
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create setup request; result = %d", result);
            return result;
        }
        ALOGD("Starting scan");

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);

        int nBuckets = 0;
        for (int i = 0; i < mParams->num_buckets; i++) {
            if (mParams->buckets[i].report_events == 2) {
                nBuckets++;
            }
        }

        if (nBuckets != 0) {
           ALOGI("Full scan requested with nBuckets = %d", nBuckets);
           registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_FULL_SCAN_RESULTS);
        }
        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to start scan; result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
            return result;
        }

      
        return result;
    }

    virtual int cancel() {
        ALOGD("Stopping scan");

        WifiRequest request(familyId(), ifaceId());
        int result = createStopRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create stop request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to stop scan; result = %d", result);
            }
        }

        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_FULL_SCAN_RESULTS);

        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGD("Got a scan results event");

        event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        unsigned int len = event.get_vendor_data_len();
        int event_id = event.get_vendor_subcmd();
        ALOGD("handleEvent, event_id = %d", event_id);

        if(event_id == GSCAN_EVENT_COMPLETE_SCAN) {
            if (vendor_data == NULL || len != 4) {
                ALOGD("Scan complete type not mentioned!");
                return NL_SKIP;
            }
            wifi_scan_event evt_type;

            evt_type = (wifi_scan_event) event.get_u32(NL80211_ATTR_VENDOR_DATA);
            ALOGD("Scan complete: Received event type %d", evt_type);
            if(*mHandler.on_scan_event)
                (*mHandler.on_scan_event)(evt_type, evt_type);
        } else if(event_id == GSCAN_EVENT_FULL_SCAN_RESULTS) {
	    if (vendor_data == NULL || len < sizeof(wifi_scan_result)) {
	        ALOGD("No scan results found");
	        return NL_SKIP;
	    }
	    wifi_scan_result *result = (wifi_scan_result *)event.get_vendor_data();

	    if(*mHandler.on_full_scan_result)
	        (*mHandler.on_full_scan_result)(id(), result);

	    ALOGD("%-32s\t", result->ssid);

	    ALOGD("%02x:%02x:%02x:%02x:%02x:%02x ", result->bssid[0], result->bssid[1],
	            result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5]);

	    ALOGD("%d\t", result->rssi);
	    ALOGD("%d\t", result->channel);
	    ALOGD("%lld\t", result->ts);
	    ALOGD("%lld\t", result->rtt);
	    ALOGD("%lld\n", result->rtt_sd);
        } else {

            if (vendor_data == NULL || len != 4) {
                ALOGD("No scan results found");
                return NL_SKIP;
            }

            int num = event.get_u32(NL80211_ATTR_VENDOR_DATA);
            ALOGD("Found %d scan results", num);
            if(*mHandler.on_scan_results_available)
                (*mHandler.on_scan_results_available)(id(), num);
        }
        return NL_SKIP;
    }
};

unsigned ScanCommand::mGlobalFullScanBuckets = 0;

wifi_error wifi_start_gscan(
        wifi_request_id id,
        wifi_interface_handle iface,
        wifi_scan_cmd_params params,
        wifi_scan_result_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    ALOGD("Starting GScan, halHandle = %p", handle);

    ScanCommand *cmd = new ScanCommand(iface, id, &params, handler);
    wifi_register_cmd(handle, id, cmd);
    return (wifi_error)cmd->start();
}

wifi_error wifi_stop_gscan(wifi_request_id id, wifi_interface_handle iface)
{
    ALOGD("Stopping GScan");
    wifi_handle handle = getWifiHandle(iface);

    if(id == -1) {
        wifi_scan_result_handler handler;
        wifi_scan_cmd_params dummy_params;
        wifi_handle handle = getWifiHandle(iface);
        memset(&handler, 0, sizeof(handler));

        ScanCommand *cmd = new ScanCommand(iface, id, &dummy_params, handler);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }


    WifiCommand *cmd = wifi_unregister_cmd(handle, id);
    if (cmd) {
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return WIFI_ERROR_INVALID_ARGS;
}

class GetScanResultsCommand : public WifiCommand {
    wifi_cached_scan_results *mScans;
    int mMax;
    int *mNum;
    int mRetrieved;
    byte mFlush;
    int mCompleted;
    static const int MAX_RESULTS = 320;
    wifi_scan_result mScanResults[MAX_RESULTS];
    int mNextScanResult;
public:
    GetScanResultsCommand(wifi_interface_handle iface, byte flush,
            wifi_cached_scan_results *results, int max, int *num)
        : WifiCommand(iface, -1), mScans(results), mMax(max), mNum(num),
                mRetrieved(0), mFlush(flush), mCompleted(0)
    { }

    int createRequest(WifiRequest& request, int num, byte flush) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_GET_SCAN_RESULTS);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_OF_RESULTS, num);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int execute() {
        WifiRequest request(familyId(), ifaceId());
        ALOGD("retrieving %d scan results", mMax);

        for (int i = 0; i < 10 && mRetrieved < mMax; i++) {
            int result = createRequest(request, (mMax - mRetrieved), mFlush);
            if (result < 0) {
                ALOGE("failed to create request");
                return result;
            }

            int prev_retrieved = mRetrieved;

            result = requestResponse(request);

            if (result != WIFI_SUCCESS) {
                ALOGE("failed to retrieve scan results; result = %d", result);
                return result;
            }

            if (mRetrieved == prev_retrieved || mCompleted) {
                /* no more items left to retrieve */
                break;
            }

            request.destroy();
        }

        ALOGE("GetScanResults read %d results", mRetrieved);
        *mNum = mRetrieved;
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        ALOGD("In GetScanResultsCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        ALOGD("Id = %0x, subcmd = %d", id, subcmd);

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetScanResults response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE) {
                mCompleted = it.get_u8();
                ALOGD("retrieved mCompleted flag : %d", mCompleted);
            } else if (it.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS || it.get_type() == 0) {
                int scan_id = 0, flags = 0, num = 0;
                for (nl_iterator it2(it.get()); it2.has_next(); it2.next()) {
                    if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_ID) {
                        scan_id = it2.get_u32();
                        ALOGD("retrieved scan_id : 0x%0x", scan_id);
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_FLAGS) {
                        flags = it2.get_u8();
                        ALOGD("retrieved scan_flags : 0x%0x", flags);
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_NUM_OF_RESULTS) {
                        num = it2.get_u32();
                        ALOGD("retrieved num_results: %d", num);
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS) {
                        if (mRetrieved >= mMax) {
                            ALOGW("Stored %d scans, ignoring excess results", mRetrieved);
                            break;
                        }
                        num = it2.get_len() / sizeof(wifi_scan_result);
                        num = min(MAX_RESULTS - mNextScanResult, num);
                        num = min((int)MAX_AP_CACHE_PER_SCAN, num);
                        memcpy(mScanResults + mNextScanResult, it2.get_data(),
                                sizeof(wifi_scan_result) * num);
                        ALOGD("Retrieved %d scan results", num);
                        wifi_scan_result *results = (wifi_scan_result *)it2.get_data();
                        for (int i = 0; i < num; i++) {
                            wifi_scan_result *result = results + i;
                            ALOGD("%02d  %-32s  %02x:%02x:%02x:%02x:%02x:%02x  %04d", i,
                                result->ssid, result->bssid[0], result->bssid[1], result->bssid[2],
                                result->bssid[3], result->bssid[4], result->bssid[5],
                                result->rssi);
                        }
                        mScans[mRetrieved].scan_id = scan_id;
                        mScans[mRetrieved].flags = flags;
                        mScans[mRetrieved].num_results = num;
                        ALOGD("Setting result of scan_id : 0x%0x", mScans[mRetrieved].scan_id);
                        memcpy(mScans[mRetrieved].results,
                                &(mScanResults[mNextScanResult]), num * sizeof(wifi_scan_result));
                        mNextScanResult += num;
                        mRetrieved++;
                    } else {
                        ALOGW("Ignoring invalid attribute type = %d, size = %d",
                                it.get_type(), it.get_len());
                    }
                }
            } else {
                ALOGW("Ignoring invalid attribute type = %d, size = %d",
                        it.get_type(), it.get_len());
            }
        }

        return NL_OK;
    }
};

wifi_error wifi_get_cached_gscan_results(wifi_interface_handle iface, byte flush,
        int max, wifi_cached_scan_results *results, int *num) {
    ALOGD("Getting cached scan results, iface handle = %p, num = %d", iface, *num);

    GetScanResultsCommand *cmd = new GetScanResultsCommand(iface, flush, results, max, num);
    return (wifi_error)cmd->execute();
}

/////////////////////////////////////////////////////////////////////////////

class BssidHotlistCommand : public WifiCommand
{
private:
    wifi_bssid_hotlist_params mParams;
    wifi_hotlist_ap_found_handler mHandler;
    static const int MAX_RESULTS = 64;
    wifi_scan_result mResults[MAX_RESULTS];
public:
    BssidHotlistCommand(wifi_interface_handle handle, int id,
            wifi_bssid_hotlist_params params, wifi_hotlist_ap_found_handler handler)
        : WifiCommand(handle, id), mParams(params), mHandler(handler)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_SET_BSSID_HOTLIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        result = request.put_u32(GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE, mParams.lost_ap_sample_size);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_HOTLIST_BSSIDS);
        for (int i = 0; i < mParams.num_bssid; i++) {
            nlattr *attr2 = request.attr_start(GSCAN_ATTRIBUTE_HOTLIST_ELEM);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_addr(GSCAN_ATTRIBUTE_BSSID, mParams.ap[i].bssid);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_HIGH, mParams.ap[i].high);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_LOW, mParams.ap[i].low);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);
        return result;
    }

    int createTeardownRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_RESET_BSSID_HOTLIST);
        if (result < 0) {
            return result;
        }

        return result;
    }

    int start() {
        ALOGD("Executing hotlist setup request, num = %d", mParams.num_bssid);
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGD("Failed to execute hotlist setup request, result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);
            return result;
        }

        ALOGD("Successfully set %d APs in the hotlist", mParams.num_bssid);

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);

        return result;
    }

    virtual int cancel() {
        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);
        /* create set hotlist message with empty hotlist */
        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            return result;
        }

        ALOGD("Successfully reset APs in current hotlist");
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGD("Hotlist AP event");
        int event_id = event.get_vendor_subcmd();
        event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGD("No scan results found");
            return NL_SKIP;
        }

        memset(mResults, 0, sizeof(wifi_scan_result) * MAX_RESULTS);

        int num = len / sizeof(wifi_scan_result);
        num = min(MAX_RESULTS, num);
        memcpy(mResults, event.get_vendor_data(), num * sizeof(wifi_scan_result));

        if (event_id == GSCAN_EVENT_HOTLIST_RESULTS_FOUND) {
            ALOGD("FOUND %d hotlist APs", num);
            if (*mHandler.on_hotlist_ap_found)
                (*mHandler.on_hotlist_ap_found)(id(), num, mResults);
        } else if (event_id == GSCAN_EVENT_HOTLIST_RESULTS_LOST) {
            ALOGD("LOST %d hotlist APs", num);
            if (*mHandler.on_hotlist_ap_lost)
                (*mHandler.on_hotlist_ap_lost)(id(), num, mResults);
        }
        return NL_SKIP;
    }
};

wifi_error wifi_set_bssid_hotlist(wifi_request_id id, wifi_interface_handle iface,
        wifi_bssid_hotlist_params params, wifi_hotlist_ap_found_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    BssidHotlistCommand *cmd = new BssidHotlistCommand(iface, id, params, handler);
    wifi_register_cmd(handle, id, cmd);
    return (wifi_error)cmd->start();
}

wifi_error wifi_reset_bssid_hotlist(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);

    WifiCommand *cmd = wifi_unregister_cmd(handle, id);
    if (cmd) {
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return WIFI_ERROR_INVALID_ARGS;
}


/////////////////////////////////////////////////////////////////////////////

class SignificantWifiChangeCommand : public WifiCommand
{
    typedef struct {
        mac_addr bssid;                     // BSSID
        wifi_channel channel;               // channel frequency in MHz
        int num_rssi;                       // number of rssi samples
        wifi_rssi rssi[8];                   // RSSI history in db
    } wifi_significant_change_result_internal;

private:
    wifi_significant_change_params mParams;
    wifi_significant_change_handler mHandler;
    static const int MAX_RESULTS = 64;
    wifi_significant_change_result_internal mResultsBuffer[MAX_RESULTS];
    wifi_significant_change_result *mResults[MAX_RESULTS];
public:
    SignificantWifiChangeCommand(wifi_interface_handle handle, int id,
            wifi_significant_change_params params, wifi_significant_change_handler handler)
        : WifiCommand(handle, id), mParams(params), mHandler(handler)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_SET_SIGNIFICANT_CHANGE);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        result = request.put_u16(GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE, mParams.rssi_sample_size);
        if (result < 0) {
            return result;
        }
        result = request.put_u16(GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE, mParams.lost_ap_sample_size);
        if (result < 0) {
            return result;
        }
        result = request.put_u16(GSCAN_ATTRIBUTE_MIN_BREACHING, mParams.min_breaching);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_BSSIDS);

        for (int i = 0; i < mParams.num_bssid; i++) {

            nlattr *attr2 = request.attr_start(i);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_addr(GSCAN_ATTRIBUTE_BSSID, mParams.ap[i].bssid);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_HIGH, mParams.ap[i].high);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_LOW, mParams.ap[i].low);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);

        return result;
    }

    int createTeardownRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_RESET_SIGNIFICANT_CHANGE);
        if (result < 0) {
            return result;
        }

        return result;
    }

    int start() {
        ALOGD("Set significant wifi change");
        WifiRequest request(familyId(), ifaceId());

        int result = createSetupRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGD("failed to set significant wifi change %d", result);
            return result;
        }
        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS);

        return result;
    }

    virtual int cancel() {
        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS);

        /* create set significant change monitor message with empty hotlist */
        WifiRequest request(familyId(), ifaceId());

        int result = createTeardownRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            return result;
        }

        ALOGD("successfully reset significant wifi change");
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGD("Got a significant wifi change event");

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGD("No scan results found");
            return NL_SKIP;
        }

        typedef struct {
            uint16_t channel;
            mac_addr bssid;
            int16_t rssi_history[8];
        } ChangeInfo;

        int num = min(len / sizeof(ChangeInfo), MAX_RESULTS);
        ChangeInfo *ci = (ChangeInfo *)event.get_vendor_data();

        for (int i = 0; i < num; i++) {
            memcpy(mResultsBuffer[i].bssid, ci[i].bssid, sizeof(mac_addr));
            mResultsBuffer[i].channel = ci[i].channel;
			/* Driver sends N samples and the rest 8-N are filled 0x7FFF
			 * N = no of rssi samples to average sent in significant change request. */
            int num_rssi = 0;
            for (int j = 0; j < 8; j++) {
                if (ci[i].rssi_history[j] == 0x7FFF) {
                    num_rssi = j;
                    break;
                }
                mResultsBuffer[i].rssi[j] = (int) ci[i].rssi_history[j];
            }
            mResultsBuffer[i].num_rssi = num_rssi;
            mResults[i] = reinterpret_cast<wifi_significant_change_result *>(&(mResultsBuffer[i]));
        }

        ALOGD("Retrieved %d scan results", num);

        if (num != 0) {
            (*mHandler.on_significant_change)(id(), num, mResults);
        } else {
            ALOGW("No significant change reported");
        }

        return NL_SKIP;
    }
};

wifi_error wifi_set_significant_change_handler(wifi_request_id id, wifi_interface_handle iface,
        wifi_significant_change_params params, wifi_significant_change_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    SignificantWifiChangeCommand *cmd = new SignificantWifiChangeCommand(
            iface, id, params, handler);
    wifi_register_cmd(handle, id, cmd);
    return (wifi_error)cmd->start();
}

wifi_error wifi_reset_significant_change_handler(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);

    WifiCommand *cmd = wifi_unregister_cmd(handle, id);
    if (cmd) {
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return WIFI_ERROR_INVALID_ARGS;
}

class ePNOCommand : public WifiCommand
{
private:
    wifi_epno_network *ssid_list;
    int               num_ssid;
    wifi_epno_handler mHandler;
    wifi_scan_result  mResults;
public:
    ePNOCommand(wifi_interface_handle handle, int id,
            int num_networks, wifi_epno_network *networks, wifi_epno_handler handler)
        : WifiCommand(handle, id), mHandler(handler)
    {
        ssid_list = networks;
        num_ssid = num_networks;
    }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_SET_EPNO_LIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        result = request.put_u8(EPNO_ATTRIBUTE_SSID_NUM, num_ssid);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(EPNO_ATTRIBUTE_SSID_LIST);
        for (int i = 0; i < num_ssid; i++) {
            nlattr *attr2 = request.attr_start(i);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put(EPNO_ATTRIBUTE_SSID, ssid_list[i].ssid, 32);
            ALOGI("ePNO [SSID:%s rssi_thresh:%d flags:%d auth:%d]", ssid_list[i].ssid,
                (signed char)ssid_list[i].rssi_threshold, ssid_list[i].flags,
                ssid_list[i].auth_bit_field);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(EPNO_ATTRIBUTE_SSID_LEN, strlen(ssid_list[i].ssid));
            if (result < 0) {
                return result;
            }

            result = request.put_u8(EPNO_ATTRIBUTE_RSSI, ssid_list[i].rssi_threshold);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(EPNO_ATTRIBUTE_FLAGS, ssid_list[i].flags);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(EPNO_ATTRIBUTE_AUTH, ssid_list[i].auth_bit_field);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGI("ePNO num_network=%d", num_ssid);
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGI("Failed: ePNO setup request, result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, WIFI_EPNO_EVENT);
            return result;
        }

        ALOGI("Successfully set %d SSIDs for ePNO", num_ssid);
        registerVendorHandler(GOOGLE_OUI, WIFI_EPNO_EVENT);
        return result;
    }

    virtual int cancel() {
        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, WIFI_EPNO_EVENT);
        return 0;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGI("ePNO event");
        int event_id = event.get_vendor_subcmd();
        // event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGI("No scan results found");
            return NL_SKIP;
        }

        mResults = *(wifi_scan_result *) event.get_vendor_data();
        if (*mHandler.on_network_found)
            (*mHandler.on_network_found)(id(), 1, &mResults);
        return NL_SKIP;
    }
};

wifi_error wifi_set_epno_list(wifi_request_id id,
                              wifi_interface_handle iface,
                              int num_networks,
                              wifi_epno_network * networks,
                              wifi_epno_handler handler)
{
     wifi_handle handle = getWifiHandle(iface);

     ePNOCommand *cmd = new ePNOCommand(iface, id, num_networks, networks, handler);
     wifi_register_cmd(handle, id, cmd);
     wifi_error result = (wifi_error)cmd->start();
     if (result != WIFI_SUCCESS) {
         wifi_unregister_cmd(handle, id);
     }
     return result;
}

class HsListCommand : public WifiCommand
{
    int num_hs;
    wifi_passpoint_network *mNetworks;
    wifi_passpoint_event_handler mHandler;
public:
    HsListCommand(wifi_request_id id, wifi_interface_handle iface,
        int num, wifi_passpoint_network *hs_list, wifi_passpoint_event_handler handler)
        : WifiCommand(iface, id), num_hs(num), mNetworks(hs_list),
            mHandler(handler)
    {
    }

    HsListCommand(wifi_request_id id, wifi_interface_handle iface,
        int num)
        : WifiCommand(iface, id), num_hs(num), mNetworks(NULL)
    {
    }

    int createRequest(WifiRequest& request, int val) {
        int result;

        if (val) {
            result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_SET_HS_LIST);
            result = request.put_u32(EPNO_ATTRIBUTE_HS_NUM, num_hs);
            if (result < 0) {
                return result;
            }
            nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

            struct nlattr * attr = request.attr_start(EPNO_ATTRIBUTE_HS_PARAM_LIST);
            for (int i = 0; i < num_hs; i++) {
                nlattr *attr2 = request.attr_start(i);
                if (attr2 == NULL) {
                    return WIFI_ERROR_OUT_OF_MEMORY;
                }
                result = request.put_u32(EPNO_ATTRIBUTE_HS_ID, mNetworks[i].id);
                if (result < 0) {
                    return result;
                }
                result = request.put(EPNO_ATTRIBUTE_HS_REALM, mNetworks[i].realm, 256);
                if (result < 0) {
                    return result;
                }
                result = request.put(EPNO_ATTRIBUTE_HS_CONSORTIUM_IDS, mNetworks[i].roamingConsortiumIds, 128);
                if (result < 0) {
                    return result;
                }
                result = request.put(EPNO_ATTRIBUTE_HS_PLMN, mNetworks[i].plmn, 3);
                if (result < 0) {
                    return result;
                }
                request.attr_end(attr2);
            }
            request.attr_end(attr);
            request.attr_end(data);
        }else {
            result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_RESET_HS_LIST);
            if (result < 0) {
                return result;
            }
        }

        return WIFI_SUCCESS;
    }

    int start() {

        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, num_hs);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, WIFI_HOTSPOT_MATCH);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to set ANQPO networks; result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, WIFI_HOTSPOT_MATCH);
            return result;
        }

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
                ALOGE("failed to reset ANQPO networks;result = %d", result);
            }
        }

        unregisterVendorHandler(GOOGLE_OUI, WIFI_HOTSPOT_MATCH);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
         ALOGD("Request complete!");
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {

        ALOGI("hotspot matched event");
        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        unsigned int len = event.get_vendor_data_len();
        if (vendor_data == NULL || len < sizeof(wifi_scan_result)) {
            ALOGE("ERROR: No scan results found");
            return NL_SKIP;
        }

        wifi_scan_result *result = (wifi_scan_result *)event.get_vendor_data();
        byte *anqp = (byte *)result + offsetof(wifi_scan_result, ie_data) + result->ie_length;
        int networkId = *(int *)anqp;
        anqp += sizeof(int);
        int anqp_len = *(u16 *)anqp;
        anqp += sizeof(u16);

        ALOGI("%-32s\t", result->ssid);

        ALOGI("%02x:%02x:%02x:%02x:%02x:%02x ", result->bssid[0], result->bssid[1],
                result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5]);

        ALOGI("%d\t", result->rssi);
        ALOGI("%d\t", result->channel);
        ALOGI("%lld\t", result->ts);
        ALOGI("%lld\t", result->rtt);
        ALOGI("%lld\n", result->rtt_sd);

        if(*mHandler.on_passpoint_network_found)
            (*mHandler.on_passpoint_network_found)(id(), networkId, result, anqp_len, anqp);

        return NL_SKIP;
    }
};

wifi_error wifi_set_passpoint_list(wifi_request_id id, wifi_interface_handle iface, int num,
        wifi_passpoint_network *networks, wifi_passpoint_event_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);
    HsListCommand *cmd = new HsListCommand(id, iface, num, networks, handler);

    wifi_register_cmd(handle, id, cmd);
    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}

wifi_error wifi_reset_passpoint_list(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle   handle = getWifiHandle(iface);
    wifi_error    result;
    HsListCommand *cmd = (HsListCommand *)(wifi_get_cmd(handle, id));

    if (cmd == NULL) {
        cmd = new HsListCommand(id, iface, 0);
        wifi_register_cmd(handle, id, cmd);
    }
    result = (wifi_error)cmd->cancel();
    wifi_unregister_cmd(handle, id);
    return result;
}
class BssidBlacklistCommand : public WifiCommand
{
private:
    wifi_bssid_params *mParams;
public:
    BssidBlacklistCommand(wifi_interface_handle handle, int id,
            wifi_bssid_params *params)
        : WifiCommand(handle, id), mParams(params)
    { }
     int createRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_SET_BSSID_BLACKLIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_BSSID, mParams->num_bssid);
        if (result < 0) {
            return result;
        }

        for (int i = 0; i < mParams->num_bssid; i++) {
            result = request.put_addr(GSCAN_ATTRIBUTE_BLACKLIST_BSSID, mParams->bssids[i]);
            if (result < 0) {
                return result;
            }
        }
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGD("Executing bssid blacklist request, num = %d", mParams->num_bssid);
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to execute bssid blacklist request, result = %d", result);
            return result;
        }

        ALOGI("Successfully added %d blacklist bssids", mParams->num_bssid);
        return result;
    }


    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }
};

wifi_error wifi_set_bssid_blacklist(wifi_request_id id, wifi_interface_handle iface,
        wifi_bssid_params params)
{
    wifi_handle handle = getWifiHandle(iface);

    BssidBlacklistCommand *cmd = new BssidBlacklistCommand(iface, id, &params);
    wifi_error result = (wifi_error)cmd->start();
    //release the reference of command as well
    cmd->releaseRef();
    return result;
}