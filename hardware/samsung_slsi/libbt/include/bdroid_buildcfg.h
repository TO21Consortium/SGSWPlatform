/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BDROID_BUILDCFG_H
#define _BDROID_BUILDCFG_H

#define BTM_DEF_LOCAL_NAME   "SLSI S5E7570"
/**
 * SD-3510 Increase page timeout value when A2dp is streaming
 * During A2DP streaming, BT sometimes fails to connect
 * because of PAGE_TIMEOUT.
 * BTA_DM_PAGE_TIMEOUT is originally defined in bta_dm_cfg.c
 */
#define BTA_DM_PAGE_TIMEOUT 8192

/**
 * SD-996 Increase MAX_L2CAP_CHANNELS
 * L2CAP: MAX_L2CAP_CHANNELS is now defined to 20 in order to be able
 * to connect to 7 HID devices or 5 HID devices and a Stereo Headset
 * which support AV, AVRCP and HFG
 * Originally defined in bt_target.h
 */
#define MAX_L2CAP_CHANNELS          (20)

/**
 * SD-1101 Optimize RX SPP throughput
 * Optimize RX SPP throughput by making sure that
 * the Peer device do not run out of rfcomm credits
 * Originally defined in bt_target.h
 * SSB-5680: Re-optimize for OPP throughput
 */
#define PORT_RX_BUF_LOW_WM          (20)
#define PORT_RX_BUF_HIGH_WM         (40)
#define PORT_RX_BUF_CRITICAL_WM     (45)

/**
 * SD-2893 Optimize MTU used by the RFCOMM socket
 * Optimize the MTU used by the RFCOMM socket by
 * defining a MTU that can be sent in one l2cap packet
 * Originally defined in bta_jv_api.h
 */
#define BTA_JV_DEF_RFC_MTU      (1011)

/*
 * SSB-213 New extension channel needed for Android-defined vendor specific commands.
 * Toggles support for vendor specific extensions such as RPA offloading,
 * feature discovery, multi-adv etc.
 */
#define BLE_VND_INCLUDED        TRUE

/**
 * SSB-4621 Change number of credits allocated for Bluetooth LE data in the Android host
 * Based in investigations initiated by MTK-86 it is recommended to change the number of
 * credits allocated for BLE data from 1 (default) to 2.
 */
#define L2C_DEF_NUM_BLE_BUF_SHARED      2

/**
 * SSB-582 Improve data scheduling to HCI when A2DP is involved
 * Ensure that A2DP data is scheduled in a way that suits the scheduling in Coex and BT-only scenarios
 */
#define L2CAP_HIGH_PRI_MIN_XMIT_QUOTA       13
#define MIN_NUMBER_A2DP_SDUS                3

#endif
