/*
**
** Copyright 2013, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef EXYNOS_CAMERA_SIZE_TABLE_H
#define EXYNOS_CAMERA_SIZE_TABLE_H
#include <cutils/log.h>
#include <utils/String8.h>
#include "ExynosCameraConfig.h"

namespace android {

#define SIZE_OF_LUT         11
#define SIZE_OF_RESOLUTION  3

enum EXYNOS_CAMERA_SIZE_RATIO_ID {
    SIZE_RATIO_16_9 = 0,
    SIZE_RATIO_4_3,
    SIZE_RATIO_1_1,
    SIZE_RATIO_3_2,
    SIZE_RATIO_5_4,
    SIZE_RATIO_5_3,
    SIZE_RATIO_11_9,
    SIZE_RATIO_END
};

enum SIZE_LUT_INDEX {
    RATIO_ID,
    SENSOR_W   = 1,
    SENSOR_H,
    BNS_W,
    BNS_H,
    BCROP_W,
    BCROP_H,
    BDS_W,
    BDS_H,
    TARGET_W,
    TARGET_H,
    SIZE_LUT_INDEX_END
};

/* LSI Sensor */
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_2560_1440)
#include "ExynosCameraSizeTable2P2_WQHD.h"
#include "ExynosCameraSizeTable2P8_WQHD.h"
#include "ExynosCameraSizeTable2T2_WQHD.h"
#else
#include "ExynosCameraSizeTable2P2_FHD.h"
#include "ExynosCameraSizeTable2T2_WQHD.h"
#include "ExynosCameraSizeTableIMX240_2P2_WQHD.h"
#include "ExynosCameraSizeTable2P8_WQHD.h"
#endif
#include "ExynosCameraSizeTable2P2_12M.h"
#include "ExynosCameraSizeTable2P3.h"
#if defined(USE_REAR_FULL_OTF)
#include "ExynosCameraSizeTable3L2_FULL_OTF.h"
#else
#include "ExynosCameraSizeTable3L2.h"
#endif
#include "ExynosCameraSizeTable3L8.h"
#include "ExynosCameraSizeTable3M2.h"
#include "ExynosCameraSizeTable3M3.h"
#include "ExynosCameraSizeTable3H5.h"
#include "ExynosCameraSizeTable3H7.h"
#if defined(USE_REAR_FULL_OTF)
#include "ExynosCameraSizeTable3P3_FULL_OTF.h"
#else
#include "ExynosCameraSizeTable3P3.h"
#endif
#include "ExynosCameraSizeTable4H5.h"
#include "ExynosCameraSizeTable5E2.h"
#if defined(USE_FRONT_VFLIP_SENSOR)
#include "ExynosCameraSizeTable5E3_VFLIP.h"
#elif defined(USE_FRONT_FULL_OTF)
#include "ExynosCameraSizeTable5E3_FULL_OTF.h"
#else
#include "ExynosCameraSizeTable5E3.h"
#endif
#include "ExynosCameraSizeTable5E8.h"
#include "ExynosCameraSizeTable6A3.h"
#include "ExynosCameraSizeTable6B2.h"
#include "ExynosCameraSizeTable8B1.h"
#include "ExynosCameraSizeTable6D1.h"
#include "ExynosCameraSizeTable4E6.h"

/* Sony Sensor */
#include "ExynosCameraSizeTableIMX134.h"
#include "ExynosCameraSizeTableIMX135.h"
#include "ExynosCameraSizeTableIMX175.h"
#if 0
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_2560_1440)
#include "ExynosCameraSizeTableIMX240_WQHD.h"
#include "ExynosCameraSizeTableIMX228_WQHD.h"
#else
#include "ExynosCameraSizeTableIMX240_FHD.h"
#endif
#endif
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_2560_1440)
#include "ExynosCameraSizeTableIMX240_WQHD.h"
#include "ExynosCameraSizeTableIMX240_2P2_WQHD.h"
#else
#include "ExynosCameraSizeTableIMX240_FHD.h"
#include "ExynosCameraSizeTableIMX240_2P2_FHD.h"
#endif
#include "ExynosCameraSizeTableIMX228_WQHD.h"
#include "ExynosCameraSizeTableIMX260_2L1_WQHD.h"
#include "ExynosCameraSizeTableIMX219.h"

/*Siliconfile Sensor */
#include "ExynosCameraSizeTableSR261.h"
#include "ExynosCameraSizeTableSR259.h"
#include "ExynosCameraSizeTableSR544.h"

/*OV Sensor */
#include "ExynosCameraSizeTableOV5670.h"

}; /* namespace android */
#endif
