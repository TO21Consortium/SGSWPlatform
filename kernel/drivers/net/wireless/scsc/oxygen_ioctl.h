/******************************************************************************
 *
 * Copyright (c) 2014 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 *****************************************************************************/

#ifndef __IOCTL_OXYGEN_H__
#define __IOCTL_OXYGEN_H__

#include <linux/if.h>
#include <linux/netdevice.h>

#define NETLINK_OXYGEN 30
#define MAX_COMMAND_LEN 30
enum ibss_event_type {
	IBSS_EVENT_TXFAIL
};

enum rmc_event_type {
	RMC_EVENT_NONE,
	RMC_EVENT_LEADER_CHECK_FAIL
};

#ifdef CONFIG_COMPAT
typedef struct _compat_android_wifi_priv_cmd {
	compat_caddr_t buf;
	int used_len;
	int total_len;
} compat_android_wifi_priv_cmd;
#endif /* CONFIG_COMPAT */

int oxygen_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
int oxygen_netlink_init(void);
void oxygen_netlink_release(void);
int oxygen_netlink_send(int pid, int type, int seq, void *data, size_t size);

int oxygen_set_ibss_beacon_oui_data(struct net_device *dev, char *command, char *buf, int buf_len);
int oxygen_get_ibss_peer_info(struct net_device *dev, char *command, char *buf, int buf_len, bool info_all);
int oxygen_set_ibss_ampdu(struct net_device *dev, char *command, char *buf, int buf_len);
int oxygen_set_ibss_antenna_mode(struct net_device *dev, char *command, char *buf, int buf_len);
int oxygen_set_rmc_enable(struct net_device *dev, char *command, char *buf, int buf_len);
int oxygen_set_rmc_tx_rate(struct net_device *dev, char *command, char *buf, int buf_len);
int oxygen_set_rmc_action_period(struct net_device *dev, char *command, char *buf, int buf_len);
int oxygen_set_rmc_leader(struct net_device *dev, char *command, char *buf, int buf_len);
int oxygen_set_ibss_tx_fail_event(struct net_device *dev, char *command, char *buf, int buf_len);
int oxygen_set_ibss_route_table(struct net_device *dev, char *command, char *buf, int buf_len);

#endif /*  __IOCTL_OXYGEN.H__ */
