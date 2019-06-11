/****************************************************************************
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd
 *
 ****************************************************************************/
#ifndef BT_VENDOR_SLSI_H__
#define BT_VENDOR_SLSI_H__

#ifndef CONFIG_SAMSUNG_SCSC_WIFIBT
#error "CONFIG_SAMSUNG_SCSC_WIFIBT not defined"
#endif

#include <cutils/log.h>

#include "bt_types.h"
#include "btm_api.h"

#ifndef BTVENDOR_DBG
#define BTVENDOR_DBG FALSE
#endif

#define BTVENDORE(param, ...) { ALOGE(param, ## __VA_ARGS__); }

#if (BTVENDOR_DBG == TRUE)
#define BTVENDORD(param, ...) { ALOGD(param, ## __VA_ARGS__); }
#define BTVENDORI(param, ...) { ALOGI(param, ## __VA_ARGS__); }
#define BTVENDORV(param, ...) { ALOGV(param, ## __VA_ARGS__); }
#define BTVENDORW(param, ...) { ALOGW(param, ## __VA_ARGS__); }
#else
#define BTVENDORD(param, ...) {}
#define BTVENDORI(param, ...) {}
#define BTVENDORV(param, ...) {}
#define BTVENDORW(param, ...) {}
#endif
#define SCSC_UNUSED(x) (void)(x)

#endif
