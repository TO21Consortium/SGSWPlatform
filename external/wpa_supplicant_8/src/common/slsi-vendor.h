/*
 * Samsung Electronics OUI and vendor specific assignments
 * Copyright (c) 2012 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef SLSI_VENDOR_H
#define SLSI_VENDOR_H

/*
 * This file is a registry of identifier assignments from the Samsung Electronics
 * OUI 00:00:f0 for purposes other than MAC address assignment. New identifiers
 * can be assigned through normal review process for changes to the upstream
 * hostap.git repository.
 */

#define OUI_SAMSUNG 0x0000f0

/**
 * enum qca_nl80211_vendor_subcmds - SLSI nl80211 vendor command identifiers
 *
 * @SLSI_NL80211_VENDOR_SUBCMD_UNSPEC: Reserved value 0
 * @SLSI_NL80211_VENDOR_SUBCMD_KEY_MGMT_SET_KEY: Set key operation that can be
 *	used to configure PMK to the driver even when not connected. This can
 *	be used to request offloading of key management operations. Only used
 *	if device supports SLSI_WLAN_VENDOR_FEATURE_KEY_MGMT_OFFLOAD.
 * @SLSI_NL80211_VENDOR_SUBCMD_KEY_MGMT_ROAM_AUTH: An extended version of
 *	NL80211_CMD_ROAM event with optional attributes including information
 *	from offloaded key management operation. Uses
 *	enum slsi_wlan_vendor_attr_roam_auth attributes. Only used
 *	if device supports SLSI_WLAN_VENDOR_FEATURE_KEY_MGMT_OFFLOAD.
 */
enum slsi_nl80211_vendor_subcmds {
	SLSI_NL80211_VENDOR_SUBCMD_UNSPEC = 0,
	SLSI_NL80211_VENDOR_SUBCMD_KEY_MGMT_SET_KEY = 1,
	SLSI_NL80211_VENDOR_SUBCMD_KEY_MGMT_ROAM_AUTH = 6,
	SLSI_NL80211_VENDOR_HANGED_EVENT = 7,
};

enum slsi_wlan_vendor_attr_roam_auth {
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_INVALID = 0,
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_BSSID,
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_REQ_IE,
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_RESP_IE,
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_AUTHORIZED,
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_KEY_REPLAY_CTR,
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_PTK_KCK,
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_PTK_KEK,
	/* keep last */
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_AFTER_LAST,
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_MAX =
	SLSI_WLAN_VENDOR_ATTR_ROAM_AUTH_AFTER_LAST - 1
};

enum slsi_wlan_vendor_attr_hanged_event {
	SLSI_WLAN_VENDOR_ATTR_HANGED_EVENT_PANIC_CODE = 1,
	SLSI_WLAN_VENDOR_ATTR_HANGED_EVENT_MAX
};

#endif /* SLSI_VENDOR_H */
