/****************************************************************************
 *
 * Copyright (c) 2012 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#include "cac.h"

static struct cac_tspec *tspec_list;
static int              tspec_list_next_id;
static u8               dialog_token_next;

/* Define the meta-data info for tspec_fields */
static struct tspec_field tspec_fields[] = {
	{ "traffic_type", 0, 1, 0x1, 0 },
	{ "tsid", 0, 1, 0xF, 1 },
	{ "direction", 0, 1, 0x3, 5 },
	{ "access_policy", 0, 1, 0x3, 7 }, /* WMM - always set to 1 */
	{ "aggregation", 0, 1, 0x1, 9 },   /* WMM - not supported */
	{ "psb", 0, 1, 0x1, 10 },
	{ "user_priority", 0, 1, 0x7, 11 },
	{ "tsinfo_ack_policy", 0, 1, 0x3, 14 }, /* WMM - not supported */
	{ "schedule", 0, 1, 0x1, 16 },          /* WMM - not supported */
	{ "nominal_msdu_size", 0, 0, 2, OFFSETOF(nominal_msdu_size) },
	{ "max_msdu_size", 0, 0, 2, OFFSETOF(maximum_msdu_size) },
	{ "min_service_interval", 0, 0, 4, OFFSETOF(minimum_service_interval) },
	{ "max_service_interval", 0, 0, 4, OFFSETOF(maximum_service_interval) },
	{ "inactivity_interval", 0, 0, 4, OFFSETOF(inactivity_interval) },
	{ "suspension_interval", 0, 0, 4, OFFSETOF(suspension_interval) },
	{ "service_start_time", 0, 0, 4, OFFSETOF(service_start_time) },
	{ "min_data_rate", 0, 0, 4, OFFSETOF(minimum_data_rate) },
	{ "mean_data_rate", 0, 0, 4, OFFSETOF(mean_data_rate) },
	{ "peak_data_rate", 0, 0, 4, OFFSETOF(peak_data_rate) },
	{ "max_burst_size", 0, 0, 4, OFFSETOF(maximum_burst_size) },
	{ "delay_bound", 0, 0, 4, OFFSETOF(delay_bound) },
	{ "min_phy_rate", 0, 0, 4, OFFSETOF(minimum_phy_rate) },
	{ "surplus_bw_allowance", 0, 0, 2,
	  OFFSETOF(surplus_bandwidth_allowance) },
	{ "medium_time", 1, 0, 2, OFFSETOF(medium_time) },
};

/* Define the OUI type data for the corresponding IE's */
static const u8  TSRS_OUI_TYPE[] = { 0x00, 0x40, 0x96, 0x08 };
static const u8  EBW_OUI_TYPE[]  = { 0x00, 0x40, 0x96, 0x0F };

static const int NUM_TSPEC_FIELDS = sizeof(tspec_fields) / sizeof(struct tspec_field);
static u32       previous_msdu_lifetime = MAX_TRANSMIT_MSDU_LIFETIME_NOT_VALID;
static u8        ccx_status = BSS_CCX_DISABLED;
/* Name: strtoint
 * Desc: Converts a string to a decimal or hexadecimal integer
 * s: the string to be converted
 * res: pointer to the calculated integer
 * return: 0 (success), 1(failure)
 */
static int strtoint(const char *s, int *res)
{
	int base = 10;

	if (strlen(s) > 2)
		if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
			base = 16;
	return kstrtoint(s, base, res);
}

/* Name: find_tspec_entry_by_id
 * Desc: Finds a tspec entry in the list of tspecs (tspec_list)
 * according to tspec id.
 * id: the tspec id
 * return: pointer to the tspec struct or NULL if a tspec doesn't exist
 */
static struct cac_tspec *find_tspec_entry_by_id(int id)
{
	struct cac_tspec *itr;

	itr = tspec_list;
	while (itr != NULL) {
		if (itr->id == id)
			break;
		itr = itr->next;
	}

	return itr;
}

/* Name: find_tspec_entry_by_tspec
 * Desc: Finds a tspec entry in the list of tspecs (tspec_list) according to a
 * pointer to a tspec struct.
 * tspec: pointer to a tspec
 * return: pointer to the tspec struct or NULL if a tspec doesn't exist
 */
static struct cac_tspec *find_tspec_entry_by_tspec(
	struct wmm_tspec_element *tspec)
{
	struct cac_tspec *itr;
	u32              tsinfo_a, tsinfo_b;

	tsinfo_a = CAC_GET_LE24(&tspec->ts_info[0]) & TSINFO_MASK;
	itr = tspec_list;
	while (itr != NULL) {
		tsinfo_b = CAC_GET_LE24(&itr->tspec.ts_info[0]) & TSINFO_MASK;
		if (tsinfo_a == tsinfo_b)
			break;
		itr = itr->next;
	}

	return itr;
}

/* Name: cac_query_tspec_field
 * Desc: Get the value of a tspec's field.
 * sdev: pointer to the slsi_dev struct
 * entry: pointer to the tspec
 * field: the field name
 * value: poinet to the field value
 * return: 0 (success), -1 (failure)
 */
static int cac_query_tspec_field(struct slsi_dev *sdev, struct cac_tspec *entry, const char *field, u32 *value)
{
	int i;
	u32 tsinfo;
	u8  mask;
	u8  *pos;

	if ((entry == NULL) || (field == NULL) || (value == NULL))
		return -1;

	for (i = 0; i < NUM_TSPEC_FIELDS; i++)
		if (strcasecmp(field, tspec_fields[i].name) == 0)
			break;
	if (i >= NUM_TSPEC_FIELDS) {
		SLSI_ERR(sdev, "CAC: Invalid TSPEC config field\n");
		return -1;
	}
	if (tspec_fields[i].is_tsinfo_field) {
		mask = tspec_fields[i].size;
		tsinfo = CAC_GET_LE24(&entry->tspec.ts_info[0]) & TSINFO_MASK;
		*value = (tsinfo >> tspec_fields[i].offset) & mask;
	} else {
		pos = (u8 *)(&entry->tspec) + tspec_fields[i].offset;
		if (tspec_fields[i].size == 1)
			*value = (*pos & 0xFF);
		else if (tspec_fields[i].size == 2)
			*value = CAC_GET_LE16(pos);
		else
			*value = CAC_GET_LE32(pos);
	}

	return 0;
}

/* Name: get_netdev_for_station
 * Desc: Get the pointer to net_device struct with vif_type == FAPI_VIFTYPE_STATION
 * sdev: pointer to the slsi_dev struct
 * return: pointer to the net_device struct or NULL if the it doesn't exist
 */
static struct net_device *get_netdev_for_station(struct slsi_dev *sdev)
{
	struct net_device *dev;
	struct netdev_vif *ndev_vif;
	s32               vif;

	for (vif = 1; vif <= CONFIG_SCSC_WLAN_MAX_INTERFACES; vif++) {
		dev = slsi_get_netdev_locked(sdev, vif);
		if (!dev)
			continue;
		ndev_vif = netdev_priv(dev);
		if (!ndev_vif)
			continue;
		if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION &&
		    ndev_vif->iftype == NL80211_IFTYPE_STATION)
			return dev;
	}
	return NULL;
}

/* Name: add_ebw_ie
 * Desc: Add ebw ie
 * buf: pointer to buf that the ie is going to added
 * buf_len: the byte length of the ie
 * tsid: tspec id
 * return: length of bytes that were added
 */
static int add_ebw_ie(u8 *buf, size_t buf_len, u8 tsid)
{
	u8 *pos;

	if ((buf == NULL) || (buf_len < 8))
		return -1;

	pos = buf;
	*pos++ = WLAN_EID_VENDOR_SPECIFIC; /* element id */
	*pos++ = 6;                        /* length */
	memcpy(pos, EBW_OUI_TYPE, sizeof(EBW_OUI_TYPE));
	pos += sizeof(EBW_OUI_TYPE);
	*pos++ = tsid;
	*pos++ = 0;

	return pos - buf;
}

/* Name: add_tsrs_ie
 * Desc: Add tsrs_ie
 * buf: pointer to buf that the ie is going to added
 * buf_len: the byte length of the ie
 * tsid: tspec id
 * rates: list of rates that are supported
 * num_rates: number of rates that are supported
 * return: length of bytes that were added
 */
static int add_tsrs_ie(u8 *buf, size_t buf_len, u8 tsid,
		       u8 rates[CCX_MAX_NUM_RATES], int num_rates)
{
	u8     *pos;
	size_t ie_len = (size_t)(7 + num_rates);
	int    i;

	if ((buf == NULL) || (buf_len < ie_len) || (rates == NULL) ||
	    (num_rates > CCX_MAX_NUM_RATES))
		return -1;

	pos = buf;
	memset(pos, 0, ie_len);
	*pos++ = WLAN_EID_VENDOR_SPECIFIC; /* element id */
	*pos++ = ie_len - 2;               /* length */
	memcpy(pos, TSRS_OUI_TYPE, sizeof(TSRS_OUI_TYPE));
	pos += sizeof(TSRS_OUI_TYPE);
	*pos++ = tsid;
	for (i = 0; i < num_rates; i++)
		*pos++ = rates[i];

	return pos - buf;
}

/* Name: bss_get_ie
 * Desc: Get the buffer of an IE that is included in a bss
 * bss: pointer to the cfg80211_bss struct
 * ie: the IE id that is going to be extracted
 * return: pointer to the start of the IE buffer
 */
static const u8 *bss_get_ie(struct cfg80211_bss *bss, u8 ie)
{
	const u8 *pos;
	u8       ies_len, ies_cur_len;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	pos = (const u8 *)(bss->ies);
	ies_len = (u8)bss->ies->len;
#else
	pos = (const u8 *)(bss->information_elements);
	ies_len = (u8)bss->len_information_elements;
#endif
	ies_cur_len = 1;

	while (ies_cur_len <= ies_len) {
		if (pos[0] == ie)
			return pos;

		pos += 2 + pos[1];
		ies_cur_len++;
	}

	return NULL;
}

/* Name: bss_get_bit_rates
 * Desc: Get the buffer of an IE that is included in a bss
 * bss: pointer to the cfg80211_bss struct
 * rates: the rates that are supported
 * return: 0 (succes), -1 (failure)
 */
static int bss_get_bit_rates(struct cfg80211_bss *bss, u8 **rates)
{
	const u8     *ie, *ie2;
	int          i, j;
	unsigned int len;
	u8           *r;

	ie = bss_get_ie(bss, WLAN_EID_SUPP_RATES);
	ie2 = bss_get_ie(bss, WLAN_EID_EXT_SUPP_RATES);

	len = (ie ? ie[1] : 0) + (ie2 ? ie2[1] : 0);

	if (!len)
		return -1;

	r = kmalloc(len, GFP_KERNEL);
	if (!r)
		return -1;

	for (i = 0; ie && i < ie[1]; i++)
		r[i] = ie[i + 2] & 0x7f;

	for (j = 0; ie2 && j < ie2[1]; j++)
		r[i + j] = ie2[j + 2] & 0x7f;

	*rates = r;
	return len;
}

/* Name: cac_send_addts
 * Desc: Build and send the ADDTS action frame
 * sdev: pointer to the slsi_dev struct
 * id: the tspec id that is going to be included in the ADDTS action frame
 * ebw: 1 (add ebw IE), 0 (don't add ebw IE)
 * return: 0 (succes), -1 (failure)
 */
static int cac_send_addts(struct slsi_dev *sdev, int id, int ebw)
{
	struct action_addts_req *req;
	size_t                  extra_ie_len = 50;
	int                     ie_len = 0;
	size_t                  req_len;
	struct cac_tspec        *entry;
	u8                      tsid, i;
	u8                      *rates;
	u8                      rate = 0;
	u8                      *pos;
	int                     num_rates;
	struct netdev_vif       *ndev_vif;
	struct net_device       *netdev;
	u16                     host_tag = slsi_tx_mgmt_host_tag(sdev);
	struct ieee80211_hdr    *hdr;
	u8                      *buf = NULL;
	u8                      *bssid;
	u8                      r = 0;

	entry = find_tspec_entry_by_id(id);
	if (entry == NULL) {
		SLSI_ERR(sdev, "CAC-ADDTS: Invalid TSPEC ID\n");
		return -1;
	}

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	netdev = get_netdev_for_station(sdev);
	if (netdev == NULL) {
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
		return -1;
	}
	ndev_vif = netdev_priv(netdev);
	if ((ndev_vif == NULL) || (ndev_vif->sta.sta_bss == NULL)) {
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
		return -1;
	}
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if ((!ndev_vif->activated) || (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) ||
	    (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED)) {
		SLSI_ERR(sdev, "CAC-ADDTS: Not connected, can't send ADDTS\n");
		r = -1;
		goto exit;
	}
	bssid = ndev_vif->sta.sta_bss->bssid;
	if (entry->accepted) {
		SLSI_ERR(sdev, "CAC-ADDTS: TSPEC already accepted\n");
		r = -1;
		goto exit;
	}
	buf = kmalloc(IEEE80211_HEADER_SIZE + sizeof(*req) + extra_ie_len, GFP_KERNEL);
	if (buf == NULL) {
		SLSI_ERR(sdev, "CAC-ADDTS: Failed to allocate ADDTS request\n");
		r = -1;
		goto exit;
	}

	hdr = (struct ieee80211_hdr *)buf;
	hdr->frame_control = IEEE80211_FC(IEEE80211_FTYPE_MGMT, IEEE80211_STYPE_ACTION);
	SLSI_ETHER_COPY(hdr->addr1, bssid);
	SLSI_ETHER_COPY(hdr->addr2, sdev->hw_addr);
	SLSI_ETHER_COPY(hdr->addr3, bssid);

	req = (struct action_addts_req *)(buf + IEEE80211_HEADER_SIZE);
	req->hdr.category = WLAN_CATEGORY_WMM;
	req->hdr.action = WMM_ACTION_CODE_ADDTS_REQ;
	if (dialog_token_next == 0)
		dialog_token_next++;
	req->hdr.dialog_token = dialog_token_next++;
	req->hdr.status_code = 0;
	tsid = (CAC_GET_LE24(req->tspec.ts_info) >> 1) & 0xF;

	/* Find the value of PSB in TSPEC. If PSB is unspecified; fill the
	 * value from UAPSD value stored in Peer structure for the AC
	 */
	if (((entry->tspec.ts_info[1] >> 2) & 0xF) == WMM_TSPEC_PSB_UNSPECIFIED) {
		struct slsi_peer        *peer;

		peer = slsi_get_peer_from_qs(sdev, netdev, SLSI_STA_PEER_QUEUESET);
		if (!peer) {
			SLSI_ERR(sdev, "CAC-ADDTS: no Peer found\n");
			r = -1;
			goto exit_free_buf;
		}
		entry->tspec.ts_info[1] |= ((peer->uapsd & BIT(slsi_frame_priority_to_ac_queue(tsid))) << 2);
	}
	memcpy(&req->tspec, &entry->tspec, sizeof(entry->tspec));
	req_len = sizeof(*req);
	pos = (u8 *)(req + 1);
	entry->ebw = ebw ? 1 : 0;

	if (ebw) {
		ie_len += add_ebw_ie(pos, extra_ie_len, tsid);
		if (ie_len <= 0)
			SLSI_ERR(sdev, "CAC-ADDTS: Failed to add EBW IE\n");
	}

	/* Add tsrs IE in case of ccx enabled bss */
	if (ccx_status == BSS_CCX_ENABLED) {
		num_rates = bss_get_bit_rates(ndev_vif->sta.sta_bss, &rates);
		if (num_rates <= 0)
			rate = 12; /* Default to 6Mbps */
		else {
			for (i = 0; i < num_rates; i++)
				if ((rates[i] > rate) && (rates[i] <= 48))
					rate = rates[i];
			kfree(rates);
		}

		do {
			/* if the nominal rate is equal to minimum_phy_rate
			 * don't add the tsrs_ie
			 */
			if ((rate * TSRS_RATE_PER_UNIT) == req->tspec.minimum_phy_rate)
				break;

			if ((rate * TSRS_RATE_PER_UNIT) > req->tspec.minimum_phy_rate) {
				ie_len += add_tsrs_ie(pos + ie_len, extra_ie_len - ie_len,
						      tsid, &rate, 1);
				if (ie_len <= 0) {
					SLSI_ERR(sdev, "CAC-ADDTS: Failed to add TSRS IE\n");
					r = -1;
					goto exit_free_buf;
				}
			} else { /* only the "<" case is possible */
				SLSI_ERR(sdev, "CAC-ADDTS: BSS rate too low\n");
				r = -1;
				goto exit_free_buf;
			}
		} while (0);
	}

	if (slsi_mlme_send_frame_mgmt(sdev, netdev, buf, (IEEE80211_HEADER_SIZE + req_len + ie_len), FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, 0, 0, 0) != 0) {
		SLSI_ERR(sdev, "CAC-ADDTS: Failed to send ADDTS request\n");
		r = -1;
		goto exit_free_buf;
	}
	entry->dialog_token = req->hdr.dialog_token;

exit_free_buf:
	kfree(buf);
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
	return r;
}

/* Name: cac_send_delts
 * Desc: Build and send the DELTS action frame
 * sdev: pointer to the slsi_dev struct
 * id: the tspec id that is going the DELTS action frame to send for
 * return: 0 (succes), -1 (failure)
 */
static int cac_send_delts(struct slsi_dev *sdev, int id)
{
	struct action_delts_req *req;
	struct cac_tspec        *entry;
	size_t                  req_len;
	u32                     priority, dir;
	int                     rc;
	struct netdev_vif       *ndev_vif;
	struct net_device       *netdev;
	u16                     host_tag = slsi_tx_mgmt_host_tag(sdev);
	struct ieee80211_hdr    *hdr;
	u8                      *buf = NULL;
	u8                      *bssid;
	u8                      r = 0;
	struct slsi_peer        *stapeer;
	u16                     blockack_control_bitmap = 0;

	entry = find_tspec_entry_by_id(id);
	if (entry == NULL) {
		SLSI_ERR(sdev, "CAC-DELTS: Invalid TSPEC ID\n");
		return -1;
	}

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	netdev = get_netdev_for_station(sdev);
	if (netdev == NULL) {
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
		return -1;
	}
	ndev_vif = netdev_priv(netdev);
	if ((ndev_vif == NULL) || (ndev_vif->sta.sta_bss == NULL)) {
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
		return -1;
	}
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	stapeer = slsi_get_peer_from_qs(sdev, netdev, SLSI_STA_PEER_QUEUESET);
	if (WARN_ON(!stapeer)) {
		r = -1;
		goto exit;
	}

	if ((!ndev_vif->activated) || (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) ||
	    (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED)) {
		SLSI_ERR(sdev, "CAC-DELTS: Not connected, can't send DELTS\n");
		r = -1;
		goto exit;
	}

	if (!entry->accepted) {
		SLSI_ERR(sdev, "CAC-DELTS: no TSPEC has been established for tsid=%d\n", entry->id);
		r = -1;
		goto exit;
	}

	bssid = ndev_vif->sta.sta_bss->bssid;
	buf = kmalloc(24 + sizeof(*req), GFP_KERNEL);
	if (buf == NULL) {
		SLSI_ERR(sdev, "CAC-DELTS: Failed to allocate DELTS request\n");
		r = -1;
		goto exit;
	}
	hdr = (struct ieee80211_hdr *)buf;
	hdr->frame_control = IEEE80211_FC(IEEE80211_FTYPE_MGMT, IEEE80211_STYPE_ACTION);
	SLSI_ETHER_COPY(hdr->addr1, bssid);
	SLSI_ETHER_COPY(hdr->addr2, sdev->hw_addr);
	SLSI_ETHER_COPY(hdr->addr3, bssid);
	req = (struct action_delts_req *)(buf + 24);
	req_len = sizeof(*req);
	req->hdr.category = WLAN_CATEGORY_WMM;
	req->hdr.action = WMM_ACTION_CODE_DELTS;
	req->hdr.dialog_token = 0;
	req->hdr.status_code = 0;
	memcpy(&req->tspec, &entry->tspec, sizeof(entry->tspec));

	/* TODO_HARDMAC: If PMF is negotiated over the link, the host shall not
	 * issue this primitive before pairwise keys have been installed in F/W .
	 */
	if (slsi_mlme_send_frame_mgmt(sdev, netdev, buf, (IEEE80211_HEADER_SIZE + req_len), FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, 0, 0, 0) != 0) {
		SLSI_ERR(sdev, "CAC-DELTS: Failed to send DELTS request\n");
		r = -1;
		goto exit_free_buf;
	}

	rc = cac_query_tspec_field(sdev, entry, "user_priority", &priority);
	rc |= cac_query_tspec_field(sdev, entry, "direction", &dir);
	if (rc != 0)
		SLSI_ERR(sdev, "CAC-DELTS: Cannot send DELTS command to driver\n");

	if (slsi_mlme_del_traffic_parameters(sdev, netdev, priority) != 0) {
		SLSI_ERR(sdev, "CAC-DELTS: Failed to send DELTS request\n");
		r = -1;
		goto exit_free_buf;
	}

	/* if the DELTS request is for UP = 4 or 5 then generate a
	 * MLME-BLOCKACK-CONTROL.request so that no BlockAck is negotiated
	 * on AC_VI. And leave AC_BE enabled
	 */
	if (priority == FAPI_PRIORITY_QOS_UP4 || priority == FAPI_PRIORITY_QOS_UP5) {
		/* bits for AC_BE should always be set */
		blockack_control_bitmap |= FAPI_BLOCKACKCONTROL_BLOCKACK_PRIORITY_0;
		blockack_control_bitmap |= FAPI_BLOCKACKCONTROL_BLOCKACK_PRIORITY_3;

		if (slsi_mlme_blockack_control_req(sdev, netdev, blockack_control_bitmap,
						   FAPI_DIRECTION_ANY,
						   ndev_vif->sta.sta_bss->bssid) != 0) {
			SLSI_ERR(sdev, "CAC-DELTS: Failed to send MLME-BLOCKACK-CONTROL request\n");
			goto exit_free_buf;
		}
	}

	entry->accepted = 0; /* DELTS sent successfully */
	sdev->tspec_error_code = 0;
	stapeer->tspec_established &= ~BIT(priority);

	if (ccx_status == BSS_CCX_ENABLED && previous_msdu_lifetime != MAX_TRANSMIT_MSDU_LIFETIME_NOT_VALID)
		if (slsi_send_max_transmit_msdu_lifetime(sdev, netdev, previous_msdu_lifetime) != 0) {
			SLSI_ERR(sdev, "CAC-DELTS: slsi_send_max_msdu_lifetime failed");
			goto exit_free_buf;
		}
exit_free_buf:
	kfree(buf);
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
	return r;
}

/* Name: cac_create_tspec
 * Desc: Create a tspec entry and added it to the tspec list
 * sdev: pointer to the slsi_dev struct
 * id: the id of the tspec that is included in DELTS action frame
 * return: 0 (succes), -1 (failure)
 */
static int cac_create_tspec(struct slsi_dev *sdev, char *args)
{
	struct cac_tspec  *entry;
	int               id;
	u8                tid_auto_done = 0;
	struct netdev_vif *ndev_vif;
	struct net_device *netdev;

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	netdev = get_netdev_for_station(sdev);
	if (netdev == NULL) {
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
		return -1;
	}
	ndev_vif = netdev_priv(netdev);
	if (ndev_vif == NULL) {
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
		return -1;
	}
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if ((!ndev_vif->activated) || (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) ||
	    (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED)) {
		SLSI_ERR(sdev, "CAC-ADDTS: Not connected, can't create TSPEC\n");
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
		return -1;
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);

	if (args == NULL) {
		/* No input for tid, so we use the auto increment*/
		if (tspec_list_next_id <= 7) {
			id = tspec_list_next_id++;
		} else {
			id = 0;
			tspec_list_next_id = 0;
			tspec_list_next_id++;
		}
		tid_auto_done = 1;
	}

	if ((!tid_auto_done) && (strtoint(args, &id) < 0)) {
		/* Invalid input for tid, so we use the auto increment*/
		if (tspec_list_next_id <= 7) {
			id = tspec_list_next_id++;
		} else {
			id = 0;
			tspec_list_next_id = 0;
			tspec_list_next_id++;
		}
	}

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (entry == NULL) {
		SLSI_ERR(sdev, "CAC: Failed to allocate TSPEC\n");
		return -1;
	}

	entry->tspec.eid = WLAN_EID_VENDOR_SPECIFIC;
	entry->tspec.length = sizeof(entry->tspec) - sizeof(entry->tspec.eid) - sizeof(entry->tspec.length);
	CAC_PUT_BE24(entry->tspec.oui, WLAN_OUI_MICROSOFT);
	entry->tspec.oui_type = WLAN_OUI_TYPE_MICROSOFT_WMM;
	entry->tspec.oui_subtype = WMM_OUI_SUBTYPE_TSPEC_ELEMENT;
	entry->tspec.version = WMM_VERSION;
	entry->accepted = 0;
	/* Setting the 7th bit of ts info to 1, as its a fixed reserved bit. */
	entry->tspec.ts_info[0] = 0x80;
	if (id >= TSID_MIN && id <= TSID_MAX) {
		entry->id =  id;
	} else {
		SLSI_ERR(sdev, "CAC: Invalid TSID, must be in range 0-7\n");
		kfree(entry);
		return -1;
	}
	entry->next = tspec_list;
	tspec_list = entry;

	return entry->id;
}

/* Name: cac_delete_tspec
 * Desc: delete a tspec from the list of the tspecs
 * sdev: pointer to the slsi_dev struct
 * id: the id of the tspec that will be deleted
 * return: 0 (succes), -1 (failure)
 */
static int cac_delete_tspec(struct slsi_dev *sdev, int id)
{
	struct cac_tspec *itr;
	struct cac_tspec *prev;

	itr = tspec_list;
	prev = NULL;
	while (itr != NULL) {
		if (itr->id == id) {
			if (prev)
				prev->next = itr->next;
			else
				tspec_list = itr->next;

			if (itr->accepted)
				cac_send_delts(sdev, itr->id);

			kfree(itr);
			return 0;
		}
		prev = itr;
		itr = itr->next;
	}
	SLSI_ERR(sdev, "CAC: Couldn't find TSPEC with ID %u", id);

	return -1;
}

/* Name: cac_config_tspec
 * Desc: Set a field's value of a tspec
 * sdev: pointer to the slsi_dev struct
 * id: the id of the tspec that will be configured
 * field: the field name that will be changed
 * value: the value of the field
 * return: 0 (succes), -1 (failure)
 */
static int cac_config_tspec(struct slsi_dev *sdev, int id, const char *field, u32 value)
{
	struct cac_tspec *entry;
	int              i;
	u32              max = 0xFFFFFFFF;
	u32              tsinfo;
	u8               mask;
	u8               *pos;

	if (field == NULL)
		return -1;

	entry = find_tspec_entry_by_id(id);
	if (entry == NULL) {
		SLSI_ERR(sdev, "CAC: Invalid TSPEC ID\n");
		return -1;
	}
	for (i = 0; i < NUM_TSPEC_FIELDS; i++)
		if (strcasecmp(field, tspec_fields[i].name) == 0)
			break;
	if (i >= NUM_TSPEC_FIELDS) {
		SLSI_ERR(sdev, "CAC: Invalid TSPEC config field\n");
		return -1;
	}
	if (tspec_fields[i].read_only) {
		SLSI_ERR(sdev, "CAC: TSPEC field is read-only\n");
		return -1;
	}
	if (tspec_fields[i].is_tsinfo_field) {
		mask = tspec_fields[i].size;
		if (value > mask) {
			SLSI_ERR(sdev, "CAC: TSPEC config value exceeded maximum\n");
			return -1;
		}
		tsinfo = CAC_GET_LE24(&entry->tspec.ts_info[0]);
		tsinfo &= ~(u32)(mask << tspec_fields[i].offset);
		tsinfo |= (u32)((value & mask) << tspec_fields[i].offset);
		CAC_PUT_LE24(&entry->tspec.ts_info[0], tsinfo);
	} else {
		if (tspec_fields[i].size < 4)
			max = ((1 << (tspec_fields[i].size * 8)) - 1);

		if (value > max) {
			SLSI_ERR(sdev, "CAC: TSPEC config value exceeded maximum\n");
			return -1;
		}

		pos = (u8 *)(&entry->tspec) + tspec_fields[i].offset;
		if (tspec_fields[i].size == 1)
			*pos = (value & 0xFF);
		else if (tspec_fields[i].size == 2)
			CAC_PUT_LE16(pos, value);
		else
			CAC_PUT_LE32(pos, value);
	}

	return 0;
}

/* Name: cac_ctrl_create_tspec
 * Desc: public function to create tspec
 * sdev: pointer to the slsi_dev struct
 * return: tspec id
 */
int cac_ctrl_create_tspec(struct slsi_dev *sdev, char *args)
{
	int id;

	id = cac_create_tspec(sdev, args);
	if (id < 0)
		return -1;

	return id;
}

/* Name: cac_ctrl_delete_tspec
 * Desc: public function to delete tspec
 * sdev: pointer to the slsi_dev struct
 * args:pointer to a buffer that contains the agrs for deleting tspec from the list
 * return: 0 (succes), -1 (failure)
 */
int cac_ctrl_delete_tspec(struct slsi_dev *sdev, char *args)
{
	int id;

	if (strtoint(args, &id) < 0) {
		SLSI_ERR(sdev, "CAC-DELETE-TSPEC: Invalid TSPEC ID\n");
		return -1;
	}

	if (cac_delete_tspec(sdev, id) < 0)
		return -1;

	return 0;
}

/* Name: cac_ctrl_config_tspec
 * Desc: public function to configure a tspec
 * sdev: pointer to the slsi_dev struct
 * args: pointer to a buffer that contains the agrs for tspec configuration
 * return: 0 (succes), -1 (failure)
 */
int cac_ctrl_config_tspec(struct slsi_dev *sdev, char *args)
{
	char *id;
	char *field;
	char *value;
	int  tspec_id;
	u32  val;

	id = args;
	field = strchr(id, ' ');
	if (field == NULL) {
		SLSI_ERR(sdev, "CAC: field string is NULL\n");
		return -1;
	}
	*field++ = '\0';
	value = strchr(field, ' ');
	if (value == NULL) {
		SLSI_ERR(sdev, "CAC: field value is NULL\n");
		return -1;
	}
	*value++ = '\0';

	if (strtoint(id, &tspec_id) < 0) {
		SLSI_ERR(sdev, "CAC: Conversion error for tspecid\n");
		return -1;
	}

	if (strtoint(value, &val) < 0) {
		SLSI_ERR(sdev, "CAC: Conversion error for tspecid value\n");
		return -1;
	}

	if (cac_config_tspec(sdev, tspec_id, field, val) < 0)
		return -1;

	return 0;
}

/* Name: cac_ctrl_send_addts
 * Desc: public function to send ADDTS action frame
 * sdev: pointer to the slsi_dev struct
 * args: buffer that contains the agrs for ADDTS request
 * return: 0 (succes), -1 (failure)
 */
int cac_ctrl_send_addts(struct slsi_dev *sdev, char *args)
{
	char *id_str;
	char *ebw_str;
	int  id;
	int  ebw = 0;

	if (args == NULL)
		return -1;

	id_str = args;
	ebw_str = strchr(id_str, ' ');
	if (ebw_str != NULL) {
		*ebw_str++ = '\0';
		if (!strncmp(ebw_str, "ebw", 3))
			ebw = 1;
	}
	if (strtoint(id_str, &id) < 0) {
		SLSI_ERR(sdev, "CAC: Conversion error for tspecid value\n");
		return -1;
	}
	if (cac_send_addts(sdev, id, ebw) < 0)
		return -1;

	return 0;
}

/* Name: cac_ctrl_send_delts
 * Desc: public function to send DELTS action frame
 * sdev: pointer to the slsi_dev struct
 * args: buffer that contains the agrs for DELTS request
 * return: 0 (succes), -1 (failure)
 */
int cac_ctrl_send_delts(struct slsi_dev *sdev, char *args)
{
	int id;

	if (args == NULL)
		return -1;

	if (strtoint(args, &id) < 0) {
		SLSI_ERR(sdev, "CAC: Invalid TSPEC ID\n");
		return -1;
	}
	if (cac_send_delts(sdev, id) < 0)
		return -1;

	return 0;
}

/* Name: cac_process_delts_req
 * Desc: process a DELTS request
 * sdev: pointer to the slsi_dev struct
 * req: buffer of the DELTS request
 * return: 0 (succes), -1 (failure)
 */
static void cac_process_delts_req(struct slsi_dev *sdev, struct net_device *netdev, struct action_delts_req *req)
{
	struct netdev_vif *ndev_vif = netdev_priv(netdev);
	struct cac_tspec  *itr;
	u32               priority, dir;
	int               rc;
	u16               blockack_control_bitmap = 0;
	struct slsi_peer  *stapeer;

	itr = find_tspec_entry_by_tspec(&req->tspec);
	if (itr == NULL) {
		SLSI_ERR(sdev, "CAC: No matching TSPEC found\n");
		return;
	}

	WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	stapeer = slsi_get_peer_from_qs(sdev, netdev, SLSI_STA_PEER_QUEUESET);
	if (WARN_ON(!stapeer))
		return;

	rc = cac_query_tspec_field(sdev, itr, "user_priority", &priority);
	rc |= cac_query_tspec_field(sdev, itr, "direction", &dir);
	if (rc != 0) {
		SLSI_ERR(sdev, "CAC: Cannot send DELTS command to driver\n");
		return;
	}

	if ((!ndev_vif->activated) || (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) ||
	    (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED) || (ndev_vif->sta.sta_bss == NULL)) {
		SLSI_ERR(sdev, "CAC: Not connected, can't send DELTS\n");
		return;
	}
	if (slsi_mlme_del_traffic_parameters(sdev, netdev, priority) != 0) {
		SLSI_ERR(sdev, "CAC: Failed to send DEL-TRAFFIC_PARAMETERS request\n");
		return;
	}

	/* if the DELTS request is for UP = 4 or 5 then generate a
	 * MLME-BLOCKACK-CONTROL.request so that no BlockAck is negotiated
	 * on AC_VI. And leave AC_BE enabled
	 */
	if (priority == FAPI_PRIORITY_QOS_UP4 || priority == FAPI_PRIORITY_QOS_UP5) {
		/* bits for AC_BE should always be set */
		blockack_control_bitmap |= FAPI_BLOCKACKCONTROL_BLOCKACK_PRIORITY_0;
		blockack_control_bitmap |= FAPI_BLOCKACKCONTROL_BLOCKACK_PRIORITY_3;

		if (slsi_mlme_blockack_control_req(sdev, netdev, blockack_control_bitmap,
						   FAPI_DIRECTION_ANY,
						   ndev_vif->sta.sta_bss->bssid) != 0) {
			SLSI_ERR(sdev, "CAC: Failed to send MLME-BLOCKACK-CONTROL request\n");
			return;
		}
	}

	itr->accepted = 0; /* del_tspec sent successfully */
	stapeer->tspec_established &= ~BIT(priority);
	if (ccx_status == BSS_CCX_ENABLED && previous_msdu_lifetime != MAX_TRANSMIT_MSDU_LIFETIME_NOT_VALID)
		if (slsi_send_max_transmit_msdu_lifetime(sdev, netdev, previous_msdu_lifetime) != 0) {
			SLSI_ERR(sdev, "CAC: slsi_send_max_msdu_lifetime failed");
			return;
		}
}

/* Name: cac_find_edca_ie
 * Desc: Finds the edca IE in the ADDTS response action frame
 * sdev: pointer to the slsi_dev struct
 * ie: buffer of the edca IE
 * tsid: the tsid that is included in the edca IE
 * lifetime: the lifetime value that is included in the edca IE
 * return: 0 (succes), -1 (failure)
 */
static int cac_find_edca_ie(const u8 *ie, size_t ie_len, u8 *tsid, u16 *lifetime)
{
	const u8 *pos = ie;

	if ((ie == NULL) || (ie_len < 9) ||
	    (tsid == NULL) || (lifetime == NULL))
		return -1;

	pos = cfg80211_find_vendor_ie(WLAN_OUI_CISCO, WLAN_OUI_TYPE_CISCO_EDCA, ie, ie_len);
	if (pos && (pos + 9 <= ie + ie_len)) {
		*tsid = pos[6];
		*lifetime = CAC_GET_LE16(&pos[7]);
		return 0;
	}

	return -1;
}

/* Name: cac_process_addts_rsp
 * Desc: parsing of the addts response
 * sdev: pointer to the slsi_dev struct
 * rsp: the buffer of the ADDTS response received
 * ie_len: the length of the buffer
 * return: 0 (succes), -1 (failure)
 */
static void cac_process_addts_rsp(struct slsi_dev *sdev, struct net_device *netdev, struct action_addts_rsp *rsp, const u8 *ie, size_t ie_len)
{
	struct netdev_vif        *ndev_vif = netdev_priv(netdev);
	struct cac_tspec         *itr;
	struct wmm_tspec_element *tspec;
	u32                      priority, dir, psb;
	int                      rc;
	u8                       tsid;
	u16                      msdu_lifetime;
	u16                      blockack_control_bitmap = 0;
	struct slsi_peer         *stapeer;

	WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));
	stapeer = slsi_get_peer_from_qs(sdev, netdev, SLSI_STA_PEER_QUEUESET);
	if (WARN_ON(!stapeer))
		goto exit;

	itr = tspec_list;
	while (itr != NULL) {
		if (itr->dialog_token == rsp->hdr.dialog_token)
			break;
		itr = itr->next;
	}
	if (itr == NULL) {
		SLSI_ERR(sdev, "CAC: No matching TSPEC found\n");
		goto exit;
	}
	if (rsp->hdr.status_code != ADDTS_STATUS_ACCEPTED) {
		SLSI_ERR(sdev, "CAC: TSPEC rejected (status=0x%02X)", rsp->hdr.status_code);
		goto exit;
	}
	rc = cac_query_tspec_field(sdev, itr, "user_priority", &priority);
	rc |= cac_query_tspec_field(sdev, itr, "direction", &dir);
	rc |= cac_query_tspec_field(sdev, itr, "psb", &psb);
	if (rc != 0) {
		SLSI_ERR(sdev, "CAC: Cannot send ADDTS command to driver");
		goto exit;
	}

	if ((ccx_status == BSS_CCX_ENABLED) && cac_find_edca_ie(ie, ie_len, &tsid, &msdu_lifetime) != 0)
		msdu_lifetime = MSDU_LIFETIME_DEFAULT;

	tspec = (struct wmm_tspec_element *)(rsp + 1);

	if ((!ndev_vif->activated) || (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) ||
	    (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED) || (ndev_vif->sta.sta_bss == NULL)) {
		SLSI_ERR(sdev, "CAC: Not connected, INVALID state for ADDTS response\n");
		goto exit;
	}

	switch (priority) {
	/*AC_BK*/
	case FAPI_PRIORITY_QOS_UP1:
		if (stapeer->tspec_established & BIT(FAPI_PRIORITY_QOS_UP2))
			goto exit_with_error;
		break;
	case FAPI_PRIORITY_QOS_UP2:
		if (stapeer->tspec_established & BIT(FAPI_PRIORITY_QOS_UP1))
			goto exit_with_error;
		break;
	/*AC_BE*/
	case FAPI_PRIORITY_QOS_UP0:
		if (stapeer->tspec_established & BIT(FAPI_PRIORITY_QOS_UP3))
			goto exit_with_error;
		break;
	case FAPI_PRIORITY_QOS_UP3:
		if (stapeer->tspec_established & BIT(FAPI_PRIORITY_QOS_UP0))
			goto exit_with_error;
		break;
	/*AC_VI*/
	case FAPI_PRIORITY_QOS_UP4:
		if (stapeer->tspec_established & BIT(FAPI_PRIORITY_QOS_UP5))
			goto exit_with_error;
		break;
	case FAPI_PRIORITY_QOS_UP5:
		if (stapeer->tspec_established & BIT(FAPI_PRIORITY_QOS_UP4))
			goto exit_with_error;
		break;
	/*AC_VO*/
	case FAPI_PRIORITY_QOS_UP6:
		if (stapeer->tspec_established & BIT(FAPI_PRIORITY_QOS_UP7))
			goto exit_with_error;
		break;
	case FAPI_PRIORITY_QOS_UP7:
		if (stapeer->tspec_established & BIT(FAPI_PRIORITY_QOS_UP6))
			goto exit_with_error;
		break;
	/* invalid*/
	default:
		SLSI_ERR(sdev, "CAC: Invalid UP in the request\n");
		goto exit;
	}

	if (slsi_mlme_set_traffic_parameters(sdev, netdev, priority, tspec->medium_time, tspec->minimum_data_rate, ndev_vif->sta.sta_bss->bssid) != 0) {
		SLSI_ERR(sdev, "CAC: Failed to send SET_TRAFFIC_PARAMETERS request\n");
		goto exit;
	}

	/* Currently the firmware autonomously negotiates BlockAck agreement for AC_BE.
	 * It is required for WMM-AC certification to use BlockAck for AC_VI.
	 * So if a TSPEC for AC_VI (UP = 5 0r 4) is successfully negotiated, the host
	 * generates an MLME-BLOCKACK-CONTROL.request, identifying that a BlockAck for the
	 * corresponding Priority (direction set to Any) should be enabled, i.e. the F/W
	 * will accept a downlink requested BlockAck Request, and will try to set-up an
	 * uplink BlockAck Request for that priority (TID).
	 */
	if (priority == FAPI_PRIORITY_QOS_UP4 || priority == FAPI_PRIORITY_QOS_UP5) {
		/* bits for AC_BE should always be set */
		blockack_control_bitmap |= FAPI_BLOCKACKCONTROL_BLOCKACK_PRIORITY_0;
		blockack_control_bitmap |= FAPI_BLOCKACKCONTROL_BLOCKACK_PRIORITY_3;
		if (priority == FAPI_PRIORITY_QOS_UP4)
			blockack_control_bitmap |= FAPI_BLOCKACKCONTROL_BLOCKACK_PRIORITY_4;
		else
			blockack_control_bitmap |= FAPI_BLOCKACKCONTROL_BLOCKACK_PRIORITY_5;

		if (slsi_mlme_blockack_control_req(sdev, netdev, blockack_control_bitmap,
						   FAPI_DIRECTION_ANY,
						   ndev_vif->sta.sta_bss->bssid) != 0) {
			SLSI_ERR(sdev, "CAC: Failed to send MLME-BLOCKACK-CONTROL request\n");
			goto exit;
		}
	}

	/* Add store in MIB the msdu_lifetime value in case of ccx enabled bss */
	if (ccx_status == BSS_CCX_ENABLED) {
		if ((slsi_read_max_transmit_msdu_lifetime(sdev, netdev, &previous_msdu_lifetime)) != 0) {
			previous_msdu_lifetime = MAX_TRANSMIT_MSDU_LIFETIME_NOT_VALID;
			SLSI_ERR(sdev, "CAC: slsi_read_max_msdu_lifetime failed");
			goto exit;
		}

		if (slsi_send_max_transmit_msdu_lifetime(sdev, netdev, msdu_lifetime) != 0) {
			SLSI_ERR(sdev, "CAC: slsi_send_max_msdu_lifetime failed");
			goto exit;
		}
	}

	itr->accepted = 1; /* add_tspec sent successfully */
	sdev->tspec_error_code = 0;
	stapeer->tspec_established |= BIT(priority);
	goto exit;

exit_with_error:
	SLSI_ERR(sdev, "CAC: Cannot set  UP for same AC for which UP is already set\n");

exit:
	return;
}

/* Name: cac_rx_wmm_action
 * Desc: Get the action frame received and call the corresponding process routine
 * sdev: pointer to the slsi_dev struct
 * data: buffer to the action frame received
 * len: the length in bytes of the action frame
 */
void cac_rx_wmm_action(struct slsi_dev *sdev, struct net_device *netdev, struct ieee80211_mgmt *data, size_t len)
{
	struct ieee80211_mgmt   *mgmt = data;
	struct action_addts_rsp *addts;

	if ((sdev == NULL) || (data == NULL) || (netdev == NULL) || (len == 0))
		return;

	if (mgmt->u.action.u.wme_action.action_code == WMM_ACTION_CODE_ADDTS_RESP) {
		addts = (struct action_addts_rsp *)&mgmt->u.action;
		cac_process_addts_rsp(sdev, netdev, addts, mgmt->u.action.u.wme_action.variable, len - sizeof(*addts) + 1);
	} else if (mgmt->u.action.u.wme_action.action_code == WMM_ACTION_CODE_DELTS) {
		cac_process_delts_req(sdev, netdev, (struct action_delts_req *)&mgmt->u.action);
	}
}

/* Name: cac_get_active_tspecs
 * Desc:
 * tspecs: the list of active tspecs
 * return: 0 (succes), -1 (failure)
 */
int cac_get_active_tspecs(struct cac_activated_tspec **tspecs)
{
	struct cac_tspec *itr = tspec_list;
	int              count = 0;
	int              i = 0;

	if (tspecs == NULL)
		return -1;

	while (itr != NULL) {
		if (itr->accepted)
			count++;
		itr = itr->next;
	}
	*tspecs = kmalloc_array((size_t)count, sizeof(struct cac_activated_tspec), GFP_KERNEL);
	itr = tspec_list;
	while (itr != NULL) {
		if (itr->accepted) {
			tspecs[i]->ebw = itr->ebw;
			memcpy(&tspecs[i]->tspec, &itr->tspec, sizeof(itr->tspec));
			i++;
		}
		itr = itr->next;
	}

	return count;
}

/*********************************************************
 * call cac_deactivate_tspecs to deactivate all tspecs
 * when the device is disconnecting
 */
/* Name: cac_deactivate_tspecs
 * Desc:
 * sdev: pointer to the slsi_dev struct
 * return: 0 (succes), -1 (failure)
 */
void cac_deactivate_tspecs(struct slsi_dev *sdev)
{
	struct cac_tspec *itr = tspec_list;
	struct cac_tspec *temp = NULL;

	SLSI_UNUSED_PARAMETER(sdev);

	while (itr != NULL) {
		itr->accepted = 0;
		itr->dialog_token = 0;
		temp = itr;
		itr = itr->next;
		kfree(temp);
	}
	tspec_list = NULL;
}
