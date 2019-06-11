/*****************************************************************************
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/
#include "oxygen_ioctl.h"
#include "ioctl.h"
#include "debug.h"
#include "mlme.h"
#include "mgt.h"
#include <net/netlink.h>
#include <linux/netdevice.h>
#include <linux/ieee80211.h>
#include "mib.h"

struct sock *nl_sk;

void oxygen_netlink_recv(struct sk_buff *skb)
{
	SLSI_ERR_NODEV("%s not implemented\n", __func__);
}

int oxygen_netlink_send(int pid, int type, int seq, void *data, size_t size)
{
	struct sk_buff  *skb = NULL;
	struct nlmsghdr *nlh = NULL;

	int             ret = -1;

	if (nl_sk == NULL) {
		SLSI_ERR_NODEV("nl_sk == NULL");
		return -1;
	}

	skb = alloc_skb(NLMSG_SPACE(size), GFP_ATOMIC);

	if (skb == NULL) {
		SLSI_ERR_NODEV("Failed alloc_skb()");
		return -1;
	}

	nlh = nlmsg_put(skb, 0, 0, 0, size, 0);

	if (nlh == NULL) {
		SLSI_ERR_NODEV("Failed nlmsg_put()");
		dev_kfree_skb(skb);
		return -1;
	}

	memcpy(nlmsg_data(nlh), data, size);
	nlh->nlmsg_seq = seq;
	nlh->nlmsg_type = type;

	ret = netlink_unicast(nl_sk, skb, pid, 0);

	SLSI_DBG3_NODEV(SLSI_NETDEV, "pid = %d seq = %d ret = %d.\n", pid, seq, ret);
	return ret;
}

int oxygen_netlink_init(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct netlink_kernel_cfg cfg = {
		.input = oxygen_netlink_recv,
	};
#endif

	if (nl_sk != NULL) {
		SLSI_ERR_NODEV("nl_sk != NULL before call netlink_kernel_create()");
		return -1;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
	nl_sk = netlink_kernel_create(&init_net, NETLINK_OXYGEN,
				      0, oxygen_netlink_recv, NULL, THIS_MODULE);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0))
	nl_sk = netlink_kernel_create(&init_net, NETLINK_OXYGEN, THIS_MODULE, &cfg);
#else
	nl_sk = netlink_kernel_create(&init_net, NETLINK_OXYGEN, &cfg);
#endif

	if (nl_sk == NULL) {
		SLSI_ERR_NODEV("nl_sk == NULL after call netlink_kernel_create()");
		return -1;
	}

	return 0;
}

void oxygen_netlink_release(void)
{
	if (nl_sk != NULL) {
		netlink_kernel_release(nl_sk);
		nl_sk = NULL;
	}
}

/* COMMAND : "DRIVER SETIBSSBEACONOUIDATA 001632 4000" */
int oxygen_set_ibss_beacon_oui_data(struct net_device *dev, char *command, char *buf, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	unsigned char     data[2];
	unsigned char     oui[3];
	unsigned int      offset = 0;
	int               tmp_len = 7;                  /* The length of IBSSBEACONOUI VENDOR_EID(1), Length(1), OUI(3),  DATA(2) */
	int               r = 0;
	int               readbyte = 0;
	u8                *buf_tmp = NULL;

	SLSI_NET_DBG3(dev, SLSI_NETDEV, "func : %s.\n", __func__);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	buf_tmp = kmalloc((size_t)tmp_len, GFP_KERNEL);
	if (buf_tmp == NULL)
		return -ENOMEM;

	readbyte = sscanf(buf, "SETIBSSBEACONOUIDATA %02hhx%02hhx%02hhx %02hhx%02hhx", &oui[0], &oui[1], &oui[2], &data[0], &data[1]);
	if (!readbyte) {
		SLSI_NET_ERR(dev, "sscanf failed\n");
		r = -EINVAL;
		goto exit;
	}

	*(buf_tmp + offset) = WLAN_EID_VENDOR_SPECIFIC;
	offset++;
	*(buf_tmp + offset) = 0x05;
	offset++;
	memcpy(buf_tmp + offset, oui, 3);
	offset += 3;
	memcpy(buf_tmp + offset, data, 2);
	offset += 2;

	slsi_cache_ies(buf_tmp, offset, &ndev_vif->ibss.cache_oui_ie, &ndev_vif->ibss.oui_ie_len);
	slsi_ibss_prepare_add_info_ies(ndev_vif);
	r = slsi_mlme_add_info_elements(sdev, dev, (FAPI_PURPOSE_BEACON | FAPI_PURPOSE_PROBE_RESPONSE),
					ndev_vif->ibss.add_info_ies,
					ndev_vif->ibss.add_info_ies_len);

	slsi_clear_cached_ies(&ndev_vif->ibss.add_info_ies, &ndev_vif->ibss.add_info_ies_len);

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "oui[0] : %02x, oui[1] : %02x, oui[2] : %02x, data : %02x%02x.\n", oui[0], oui[1], oui[2], data[0], data[1]);

exit:
	kfree(buf_tmp);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

/* COMMAND : “DRIVER GETIBSSPEERINFOALL”
 * RETURN DATA : “3 cc:3a:61:b0:c4:95 433 -38 cc:3a:61:b0:c4:96 150 -56 cc:3a:61:b0:c4:96 6 -78”
 */
int oxygen_get_ibss_peer_info(struct net_device *dev, char *command, char *buf, int buf_len, bool info_all)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	struct slsi_peer  *peer = NULL;
	int               bytes_written  = 0;
	int               qs_count = 0;
	int               current_peer_count = 0;
	int               i;
	int               r = 0;
	u8                macaddr[6];

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "enter\n");

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	qs_count = ndev_vif->peer_sta_records;
	memset(buf, 0, buf_len);

	if (!info_all)
		sscanf(buf, "GETIBSSPEERINFO %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
		       &macaddr[0], &macaddr[1], &macaddr[2], &macaddr[3], &macaddr[4], &macaddr[5]);

	if (qs_count != 0) {
		if (info_all)
			bytes_written += snprintf(&buf[bytes_written], sizeof(buf[bytes_written]), "%d", qs_count);

		for (i = 0; i < SLSI_ADHOC_PEER_CONNECTIONS_MAX; i++) {
			peer = slsi_get_peer_from_qs(sdev, dev, i);

			if (peer == NULL)
				continue;

			current_peer_count++;

			if (info_all) {
				/* write MAC addr */
				bytes_written += sprintf(&buf[bytes_written], " %pM ", peer->address);

				r = slsi_mlme_ibss_get_sinfo_mib(sdev, dev, peer);

				if (r != 0) {
					SLSI_NET_ERR(dev, "failed to read txrate and signal :%d\n", r);
				} else {
					/* write Link Rate */
					bytes_written += sprintf(&buf[bytes_written], "%hu ", peer->txrate);

					/* write RSSI */
					bytes_written += sprintf(&buf[bytes_written], "%hhd", peer->signal);
					SLSI_NET_DBG3(dev, SLSI_NETDEV, "%d txrate = %hu. signal = %hhd.\n",
						      i, peer->txrate, peer->signal);
				}
			} else if (memcmp(&macaddr, peer->address, sizeof(macaddr)) == 0) {
				SLSI_NET_DBG3(dev, SLSI_NETDEV, "Peer address is matched with %d %pM.\n", i, peer->address);
				r = slsi_mlme_ibss_get_sinfo_mib(sdev, dev, peer);

				if (r != 0) {
					SLSI_NET_ERR(dev, "failed to read txrate and signal :%d\n", r);
				} else {
					/* write Link Rate */
					bytes_written += sprintf(&buf[bytes_written], "%hu ", peer->txrate);

					/* write RSSI */
					bytes_written += sprintf(&buf[bytes_written], "%hhd", peer->signal);
				}
				break; /* Exit the loop */
			}

			if (current_peer_count == qs_count)
				break;
		}
	} else {
		SLSI_NET_DBG3(dev, SLSI_NETDEV, "No peer attached.\n");
	}

	bytes_written += sprintf(&buf[bytes_written], "%s", "\0");

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return bytes_written;
}

/* COMMAND: “DRIVER SETIBSSAMPDU 1 0 0 1” */
int oxygen_set_ibss_ampdu(struct net_device *dev, char *command, char *buf, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               ac[4];
	int               up_in = 0, up_enb, up_dis;
	int               ret = 0;
	int               readbyte = 0;

	readbyte = sscanf(buf, "SETIBSSAMPDU %1d %1d %1d %1d"
	       , &ac[SLSI_80211_AC_VO], &ac[SLSI_80211_AC_VI], &ac[SLSI_80211_AC_BE], &ac[SLSI_80211_AC_BK]);
	if (!readbyte) {
		SLSI_NET_ERR(dev, "sscanf failed\n");
		return -EINVAL;
	}

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "IBSS AMPDU: VO=%d VI=%d BE=%d BK=%d\n"
		      , ac[SLSI_80211_AC_VO], ac[SLSI_80211_AC_VI], ac[SLSI_80211_AC_BE], ac[SLSI_80211_AC_BK]);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (WARN_ON(!ndev_vif->activated)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Inactive vif\n");
		ret = -EINVAL;
		goto exit;
	}

	if (WARN_ON(ndev_vif->vif_type != FAPI_VIFTYPE_ADHOC)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Invalid vif type\n");
		ret = -EINVAL;
		goto exit;
	}

	if (ac[SLSI_80211_AC_VO])
		up_in |= BIT(FAPI_PRIORITY_QOS_UP7) | BIT(FAPI_PRIORITY_QOS_UP6);
	if (ac[SLSI_80211_AC_VI])
		up_in |= BIT(FAPI_PRIORITY_QOS_UP5) | BIT(FAPI_PRIORITY_QOS_UP4);
	if (ac[SLSI_80211_AC_BE])
		up_in |= BIT(FAPI_PRIORITY_QOS_UP3) | BIT(FAPI_PRIORITY_QOS_UP0);
	if (ac[SLSI_80211_AC_BK])
		up_in |= BIT(FAPI_PRIORITY_QOS_UP2) | BIT(FAPI_PRIORITY_QOS_UP1);

	up_enb = (ndev_vif->ibss.ba_tx_userpri ^ up_in) & up_in;
	up_dis = (ndev_vif->ibss.ba_tx_userpri ^ up_in) & ~up_in;

	if (up_dis) {
		ret = slsi_ibss_disable_blockack(sdev, dev, up_dis);
		if (ret)
			SLSI_ERR(sdev, "Failed to disable IBSS blockack(%u): up_dis=0x%02X\n", ret, up_dis);
	}

	if (up_enb) {
		ret = slsi_ibss_enable_blockack(sdev, dev, NULL, up_enb);
		if (ret)
			SLSI_ERR(sdev, "Failed to enable IBSS blockack(%u): up_enb=0x%02X\n", ret, up_enb);
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

/* COMMAND: “DRIVER SETIBSSANTENNAMODE 1 1” */
int oxygen_set_ibss_antenna_mode(struct net_device *dev, char *command, char *buf, int buf_len)
{
	int txchain, rxchain;
	int readbyte = 0;

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "func : %s enter\n", __func__);
	readbyte = sscanf(buf, "SETIBSSANTENNAMODE %1d %1d", &txchain, &rxchain);
	if (!readbyte) {
		SLSI_NET_ERR(dev, "sscanf failed\n");
		return -EINVAL;
	}

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "CHAIN tx: %d rx: %d\n", txchain, rxchain);
	return -EOPNOTSUPP;
}

/* COMMAND: "DRIVER SETRMCENABLE (DATA[0 or 1])" */
int oxygen_set_rmc_enable(struct net_device *dev, char *command, char *buf, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               rmc;
	int               ret = 0;
	int               readbyte = 0;

	readbyte = sscanf(buf, "SETRMCENABLE %1d", &rmc);
	if (!readbyte) {
		SLSI_NET_ERR(dev, "sscanf failed\n");
		return -EINVAL;
	}

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "RMC enable: %d\n", rmc);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (WARN_ON(!ndev_vif->activated)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Inactive vif\n");
		ret = -EINVAL;
		goto exit;
	}

	if (WARN_ON(ndev_vif->vif_type != FAPI_VIFTYPE_ADHOC)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Invalid vif type\n");
		ret = -EINVAL;
		goto exit;
	}

	switch (rmc) {
	case 1:
		ret = slsi_mlme_enable_ibss_rmc(sdev, dev, (const u8 *)SLSI_IBSS_RMC_LEADER);
		if (ret)
			SLSI_ERR(sdev, "Failed RMC_ENABLE: %d\n", ret);
		break;
	case 0:
		ret = slsi_mlme_disable_ibss_rmc(sdev, dev);
		if (ret)
			SLSI_ERR(sdev, "Failed RMC_DISABLE: %d\n", ret);
		else
			ndev_vif->ibss.rmc_txrate = SLSI_IBSS_RMC_DEFAULT_TX_RATE;
		break;
	default:
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Invalid input RMC enble/disable\n");
		ret = -EINVAL;
		break;
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

/* COMMAND: "DRIVER SETRMCTXRATE (DATA[0,6,9,12,18,24,36,48,54,65 or 72])" */
int oxygen_set_rmc_tx_rate(struct net_device *dev, char *command, char *buf, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int               rate;
	int               ret = 0;
	int               readbyte = 0;

	readbyte = sscanf(buf, "SETRMCTXRATE %2d", &rate);
	if (!readbyte) {
		SLSI_NET_ERR(dev, "sscanf failed\n");
		return -EINVAL;
	}

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "RMC Tx rate: %d\n", rate);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (WARN_ON(!ndev_vif->activated)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Inactive vif\n");
		ret = -EINVAL;
		goto exit;
	}

	if (WARN_ON(ndev_vif->vif_type != FAPI_VIFTYPE_ADHOC)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Invalid vif type\n");
		ret = -EINVAL;
		goto exit;
	}

	switch (rate) {
	/* TODO: Auto rate is not supported in Condor initial implementation. */
	case 0:
		ret = -EOPNOTSUPP;
		break;

	/* Non-HT rate */
	case 6:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_NON_HT_6MBPS;
		break;
	case 9:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_NON_HT_9MBPS;
		break;
	case 12:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_NON_HT_12MBPS;
		break;
	case 18:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_NON_HT_18MBPS;
		break;
	case 24:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_NON_HT_24MBPS;
		break;
	case 36:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_NON_HT_36MBPS;
		break;
	case 48:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_NON_HT_48MBPS;
		break;
	case 54:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_NON_HT_54MBPS;
		break;

	/* HT rate with 20MHz, SGI enabled, LDPC=0, STBC=0 */
	case 65:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_HT_65MBPS;
		break;
	case 72:
		ndev_vif->ibss.rmc_txrate = SLSI_TX_RATE_HT_72MBPS;
		break;

	default:
		SLSI_NET_DBG1(dev, SLSI_NETDEV, "Invalid RMC Tx rate\n");
		ret = -EINVAL;
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

/* COMMAND: "DRIVER SETRMCACTIONPERIOD (DATA[100 ~ 1000])" */
int oxygen_set_rmc_action_period(struct net_device *dev, char *command, char *buf, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               period;
	const int         period_min = 100, period_max = 1000;
	int               ret = 0;
	int               readbyte = 0;

	readbyte = sscanf(buf, "SETRMCACTIONPERIOD %4d", &period);
	if (!readbyte) {
		SLSI_NET_ERR(dev, "sscanf failed\n");
		return -EINVAL;
	}

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "RMC action period: %d\n", period);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (WARN_ON(!ndev_vif->activated)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Inactive vif\n");
		ret = -EINVAL;
		goto exit;
	}

	if (WARN_ON(ndev_vif->vif_type != FAPI_VIFTYPE_ADHOC)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Invalid vif type\n");
		ret = -EINVAL;
		goto exit;
	}

	if (period < period_min || period > period_max) {
		SLSI_NET_DBG1(dev, SLSI_NETDEV, "Invalid period: %d-%d\n", period_min, period_max);
		ret = -EINVAL;
		goto exit;
	}

	ret = slsi_set_uint_mib(sdev, dev, SLSI_PSID_UNIFI_RMC_ACTION_PERIOD, period);
	if (ret)
		SLSI_ERR(sdev, "Failed SETRMCACTIONPERIOD: %d\n", ret);

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

/* COMMAND: "DRIVER SETRMCLEADER (DATA)"
 * RETURN DATA : 0(success), -1(failed)
 */
int oxygen_set_rmc_leader(struct net_device *dev, char *command, char *buf, int buf_len)
{
	u8  addr[ETH_ALEN] = { 0 };
	int ret = 0;
	int readbyte = 0;

	readbyte = sscanf(buf, "SETRMCLEADER %02X:%02X:%02X:%02X:%02X:%02X"
	       , (u32 *)&addr[0], (u32 *)&addr[1], (u32 *)&addr[2], (u32 *)&addr[3], (u32 *)&addr[4], (u32 *)&addr[5]);
	if (!readbyte) {
		SLSI_NET_ERR(dev, "sscanf failed\n");
		return -EINVAL;
	}

	SLSI_NET_DBG1(dev, SLSI_NETDEV, "RMC leader: %pM\n", addr);

	/* TODO: Manual Leader Selection is not supported in Condor instead of Auto Selection. */
	if (!is_zero_ether_addr(addr)) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV, "Not permitted manual RMC leader select\n");
		ret = -EPERM;
	}

	return ret;
}

/* COMMAND: "enable : DRIVER SETIBSSTXFAILEVENT (VALUE) (PID)"
 * COMMAND: "disable : DRIVER SETIBSSTXFAILEVENT 0"
 */
int oxygen_set_ibss_tx_fail_event(struct net_device *dev, char *command, char *buf, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	unsigned short    failure_count;
	int               pid;
	int               r = 0;
	int               readbyte = 0;

	SLSI_NET_DBG3(dev, SLSI_NETDEV, "enter\n");
	readbyte = sscanf(buf, "SETIBSSTXFAILEVENT %5hd %6d", &failure_count, &pid);
	if (!readbyte) {
		SLSI_NET_ERR(dev, "sscanf failed\n");
		return -EINVAL;
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_DBG3(dev, SLSI_NETDEV, "fail count : %d pid : %d.\n", failure_count, (failure_count == 0 ? 0 : pid));

	if (slsi_set_uint_mib(sdev, dev, SLSI_PSID_UNIFI_TX_FAILURE_THRESHOLD, (int)failure_count) != 0) {
		SLSI_NET_ERR(dev, "fail to set the mib SLSI_PSID_UNIFI_TX_FAILURE_THRESHOLD.\n");
		r = -EIO;
	}

	if (r < 0 || failure_count == 0)
		ndev_vif->ibss.olsrd_pid = 0;
	else
		ndev_vif->ibss.olsrd_pid = pid;

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

s32 oxygen_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct android_wifi_priv_cmd priv_cmd;
	int                          ret = 0;
	char                         command[MAX_COMMAND_LEN];
	int                          i;

#ifdef CONFIG_COMPAT
	if (is_compat_task()) {
		compat_android_wifi_priv_cmd compat_priv_cmd;

		if (copy_from_user(&compat_priv_cmd, rq->ifr_data, sizeof(compat_android_wifi_priv_cmd)))
			return -EFAULT;
		priv_cmd.buf = compat_ptr(compat_priv_cmd.buf);
		priv_cmd.used_len = compat_priv_cmd.used_len;
		priv_cmd.total_len = compat_priv_cmd.total_len;
	} else {
#endif /* CONFIG_COMPAT */
		if (copy_from_user((void *)&priv_cmd, (void *)rq->ifr_data, sizeof(struct android_wifi_priv_cmd)))
			return -EFAULT;
#ifdef CONFIG_COMPAT
	}
#endif /* CONFIG_COMPAT */

	/* parse command. */
	memset(command, 0, sizeof(command));
	i = 0;
	while (i < MAX_COMMAND_LEN) {
		if (priv_cmd.buf[i] == ' ' || priv_cmd.buf[i] == '\0')
			break;
		command[i] = priv_cmd.buf[i];
		i++;
	}

	/* execute command. */
	if (strncmp(command, "SETIBSSBEACONOUIDATA", 20) == 0) {
		ret = oxygen_set_ibss_beacon_oui_data(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strncmp(command, "GETIBSSPEERINFOALL", 18) == 0) {
		ret = oxygen_get_ibss_peer_info(dev, command, priv_cmd.buf, priv_cmd.total_len, true);
	} else if (strncmp(command, "GETIBSSPEERINFO", 15) == 0) {
		ret = oxygen_get_ibss_peer_info(dev, command, priv_cmd.buf, priv_cmd.total_len, false);
	} else if (strncmp(command, "SETIBSSAMPDU", 13) == 0) {
		ret = oxygen_set_ibss_ampdu(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strncmp(command, "SETIBSSANTENNAMODE", 19) == 0) {
		ret = oxygen_set_ibss_antenna_mode(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strncmp(command, "SETRMCENABLE", 12) == 0) {
		ret = oxygen_set_rmc_enable(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strncmp(command, "SETRMCTXRATE", 12) == 0) {
		ret = oxygen_set_rmc_tx_rate(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strncmp(command, "SETRMCACTIONPERIOD", 19) == 0) {
		ret = oxygen_set_rmc_action_period(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strncmp(command, "SETRMCLEADER", 12) == 0) {
		ret = oxygen_set_rmc_leader(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strncmp(command, "SETIBSSTXFAILEVENT", 18) == 0) {
		ret = oxygen_set_ibss_tx_fail_event(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else {
		SLSI_NET_ERR(dev, "INVALID command : %.*s", priv_cmd.total_len, priv_cmd.buf);
		ret  = -EINVAL;
	}

	return ret;
}
