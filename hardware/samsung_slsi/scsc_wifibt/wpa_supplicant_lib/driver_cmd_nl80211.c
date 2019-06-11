/*
 * Driver interaction with extended Linux CFG8021
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#include "includes.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/sockios.h>
#include "nl80211_copy.h"
#include "common.h"
#include "eloop.h"
#include "utils/list.h"
#include "common/ieee802_11_defs.h"
#include "netlink.h"
#include "linux_ioctl.h"
#include "radiotap.h"
#include "radiotap_iter.h"
#include "rfkill.h"
#include "driver.h"
#include "wpa_supplicant_i.h"
#include "driver_cmd_nl80211.h"
#ifdef ANDROID
#include "android_drv.h"
#endif

struct android_wifi_priv_cmd {
#ifdef SLSI_64_BIT_IPC
	u64 buf;
#else
	char *buf;
#endif
	int  used_len;
	int  total_len;
};

int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf,
				  size_t buf_len)
{
	struct i802_bss                *bss = priv;
	struct wpa_driver_nl80211_data *drv = bss->drv;
	int    status = 0;

	if (os_strcasecmp(cmd, "MACADDR") == 0) {
		struct i802_bss                *bss = priv;
		struct wpa_driver_nl80211_data *drv = bss->drv;
		u8 macaddr[ETH_ALEN] = {};
		status = linux_get_ifhwaddr(drv->global->ioctl_sock, bss->ifname, macaddr);
		if (!status)
			status = os_snprintf(buf, buf_len,
					  "Macaddr = " MACSTR "\n", MAC2STR(macaddr));
	} else if (os_strncasecmp(cmd, "STOP", 4) == 0) {
		/*
		 * Framework sends the DRIVER STOP command if there is no activity for
		 * DEFAULT_IDLE_MS (15 minutes). Before sending the DRIVER STOP it
		 * disconnects the station from the AP. If the station is not connected with any
		 * AP then the chip will remain in deepsleep mode. So the chip will not
		 * consume any power. As there is no need to do anything in the driver for
		 * DRIVER STOP/START command, so it is not required to send these
		 * commands to the driver. We just have to make the interface down to
		 * move the WifiStateMachine from DriverStoppingState to DriverStoppedState.
		 */
		linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 0);
	} else if (os_strncasecmp(cmd, "START", 5) == 0) {
		/*
		 * "DRIVER START" command comes following the "DRIVER STOP" command.
		 * This command comes when the user turns on the screen(resumes the platform).
		 */
		linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 1);
	} else {
		struct ifreq                   ifrq;
		struct android_wifi_priv_cmd          priv_cmd;

		memset(&ifrq, 0, sizeof(ifrq));
		memset(&priv_cmd, 0, sizeof(priv_cmd));

		/* array size of ifrq.fir_name is IFNAMSIZ and bss->ifname is IFNAMSIZ+1 */
		os_strlcpy(ifrq.ifr_name, bss->ifname, IFNAMSIZ);
		os_memcpy(buf, cmd, strlen(cmd) + 1);
#ifdef SLSI_64_BIT_IPC
		priv_cmd.buf = (u64)(uintptr_t)buf;
#else
		priv_cmd.buf = buf;
#endif
		priv_cmd.used_len = buf_len;
		priv_cmd.total_len = buf_len;

		ifrq.ifr_data = (void *)&priv_cmd;

		if (drv->global->ioctl_sock < 0) {
			wpa_printf(MSG_ERROR, "Sock error.");
			return -1;
		}
		wpa_printf(MSG_DEBUG, "IOCTL invoke");
		status = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 2, &ifrq);
		if (status < 0)
			wpa_printf(MSG_ERROR, "Failed to handle %s command", cmd);
		else {
			if ((os_strncasecmp(cmd, "COUNTRY", 7) == 0) ||
                          (os_strncasecmp(cmd, "SETBAND", 7) == 0)) {
				union wpa_event_data event;

				os_memset(&event, 0, sizeof(event));
				event.channel_list_changed.initiator = REGDOM_SET_BY_USER;
				if (os_strncasecmp(cmd, "COUNTRY", 7) == 0) {
					event.channel_list_changed.type = REGDOM_TYPE_COUNTRY;
					if (os_strlen(cmd) > 9) {
						event.channel_list_changed.alpha2[0] = cmd[8];
						event.channel_list_changed.alpha2[1] = cmd[9];
					}
				} else {
					event.channel_list_changed.type = REGDOM_TYPE_UNKNOWN;
				}
				wpa_supplicant_event(drv->ctx, EVENT_CHANNEL_LIST_CHANGED, &event);
			}
		}
	}

	return status;
}

int wpa_driver_set_p2p_noa(void *priv, u8 count, int start, int duration)
{
	char buf[MAX_DRV_CMD_SIZE];

	memset(buf, 0, sizeof(buf));
	wpa_printf(MSG_DEBUG, "%s: Entry", __func__);
	snprintf(buf, sizeof(buf), "P2P_SET_NOA %d %d %d", count, start, duration);
	return wpa_driver_nl80211_driver_cmd(priv, buf, buf, strlen(buf)+1);
}

int wpa_driver_get_p2p_noa(void *priv, u8 *buf, size_t len)
{
	return 0;
}

int wpa_driver_set_p2p_ps(void *priv, int legacy_ps, int opp_ps, int ctwindow)
{
	char buf[MAX_DRV_CMD_SIZE];
	int value = 0;

	memset(buf, 0, sizeof(buf));
	wpa_printf(MSG_DEBUG, "%s: Entry, legacy_ps(%d), opp_ps(%d), ctwindow(%d)\n", __func__, legacy_ps, opp_ps, ctwindow);

	if (legacy_ps >= 0)
		return FALSE;
	if (opp_ps < 0) {
		/* ctwindow should be atleast 10msec so that it is visible to other devices*/
		if (ctwindow < 10)
			return FALSE;
		value = ctwindow;
	} else {
		if (opp_ps)
			/* Ignore enabling of opp_ps, as it was enabled when ctwindow>10 received.*/
			return FALSE;
		else
			/* Disable opp_ps */
			value = opp_ps;
	}
	snprintf(buf, sizeof(buf), "P2P_SET_PS %d", value);
	return wpa_driver_nl80211_driver_cmd(priv, buf, buf, strlen(buf) + 1);
}

int wpa_driver_set_ap_wps_p2p_ie(void *priv, const struct wpabuf *beacon,
				 const struct wpabuf *proberesp,
				 const struct wpabuf *assocresp)
{
	char *buf;
	const struct wpabuf *ap_wps_p2p_ie = NULL;

	u8 *_cmd = (u8 *)"SET_AP_P2P_WPS_IE";
	u8 *pbuf;
	int ret = 0;
	int i, buf_len;
	enum if_type {
		NONE,
		IF_TYPE_P2P_DEVICE,
		IF_TYPE_P2P_GROUP
	};
	enum if_type iftype = NONE;
	struct cmd_desc {
		int cmd;
		const struct wpabuf *src;
	} cmd_arr[] = {
		{0x1, beacon},
		{0x2, proberesp},
		{0x4, assocresp},
		{-1, NULL}
	};

	wpa_printf(MSG_DEBUG, "%s: Entry", __func__);
	if (((proberesp != NULL) && (beacon == NULL)) ||
		((proberesp == NULL) && (beacon == NULL) && (assocresp == NULL)))
		/* P2P Device mode Probe Response IEs */
		iftype = IF_TYPE_P2P_DEVICE;
	else if ((proberesp != NULL) && (beacon != NULL) && (assocresp != NULL))
		iftype = IF_TYPE_P2P_GROUP;

	for (i = 0; cmd_arr[i].cmd != -1; i++) {
		ap_wps_p2p_ie = cmd_arr[i].src;
		if (ap_wps_p2p_ie) {
			buf_len = strlen((char *)_cmd) + 5 + wpabuf_len(ap_wps_p2p_ie);
			buf = os_zalloc(buf_len);
			if (NULL == buf) {
				wpa_printf(MSG_ERROR, "%s: Out of memory",
					   __func__);
				ret = -1;
				break;
			}
		} else {
			continue;
		}
		pbuf = (u8 *)buf;
		pbuf += snprintf((char *)pbuf, buf_len - wpabuf_len(ap_wps_p2p_ie),
				 "%s %d %d", _cmd, cmd_arr[i].cmd, iftype);
		*pbuf++ = '\0';
		os_memcpy(pbuf, wpabuf_head(ap_wps_p2p_ie), wpabuf_len(ap_wps_p2p_ie));
		ret = wpa_driver_nl80211_driver_cmd(priv, buf, buf, buf_len);

		os_free(buf);
		if (ret < 0)
			break;
	}

	return ret;
}
