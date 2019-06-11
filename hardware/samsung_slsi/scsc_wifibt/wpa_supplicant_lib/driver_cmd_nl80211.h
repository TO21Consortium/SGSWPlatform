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
#ifndef _DRIVER_CMD_NL80211_H_
#define _DRIVER_CMD_NL80211_H_

#include "driver_nl80211.h"

#ifdef ANDROID

/* Prototype taken from the driver_nl80211.c file so that the
 * wpa_supplicant NL send and rec message handling functions can
 * be used. Only extern on Android build.
 */
int send_and_recv_msgs(struct wpa_driver_nl80211_data *drv, struct nl_msg *msg,
		       int (*valid_handler)(struct nl_msg *, void *),
		       void *valid_data);

#else
#error Platform/Target not supported
#endif
#endif
