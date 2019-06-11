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

#ifndef EXYNOS_CAMERA_LUT_3L8_H
#define EXYNOS_CAMERA_LUT_3L8_H

#include "ExynosCameraConfig.h"

/* -------------------------
    SIZE_RATIO_16_9 = 0,
    SIZE_RATIO_4_3,
    SIZE_RATIO_1_1,
    SIZE_RATIO_3_2,
    SIZE_RATIO_5_4,
    SIZE_RATIO_5_3,
    SIZE_RATIO_11_9,
    SIZE_RATIO_END
----------------------------
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
-----------------------------
    Sensor Margin Width  = 0,
    Sensor Margin Height = 0
-----------------------------*/

static int PREVIEW_SIZE_LUT_3L8[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 2322      ,   /* [bcrop  ] */
      4128      , 2322      ,   /* [bds    ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [target ] */
#endif
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 3096      ,   /* [bcrop  ] */
      4128      , 3096      ,   /* [bds    ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1440      , 1080      ,   /* [target ] */
#else
      1920      , 1440      ,   /* [target ] */
#endif
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      2976      , 2976      ,   /* [bcrop  ] */
      2976      , 2976      ,   /* [bds    ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1088      , 1088      ,   /* [target ] */
#else
      1440      , 1440      ,   /* [target ] */
#endif
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4208      , 2804      ,   /* [bcrop  ] */
      4208      , 2804      ,   /* [bds    ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
#else
      2160      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      3888      , 3120      ,   /* [bcrop  ] */
      3888      , 3120      ,   /* [bds    ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1344      , 1080      ,   /* [target ] */ /* w=1350, Reduced for 16 pixel align */
#else
      1792      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4208      , 2512      ,   /* [bcrop  ] */
      4208      , 2512      ,   /* [bds    ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
#else
      2400      , 1440      ,   /* [target ] */
#endif
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      3808      , 3120      ,   /* [bcrop  ] */
      3808      , 3120      ,   /* [bds    ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
#else
      1760      , 1440      ,   /* [target ] */
#endif
    }
};

static int PREVIEW_SIZE_LUT_3L8_BNS_15[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2752      , 1548      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
#endif
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2752      , 2064      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
#else
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
#endif
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2080      , 2080      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
#else
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
#endif
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2800      , 1868      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
#else
      2160      , 1440      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2800      , 1684      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
#else
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2800      , 1684      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
#else
      2400      , 1440      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
#endif
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2544      , 2080      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
#else
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
#endif
    }
};

static int PREVIEW_SIZE_LUT_3L8_BNS_20[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      2104      , 1184      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      2104      , 1560      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
#else
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
#endif
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      1560      , 1560      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
#else
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
#endif
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      2104      , 1402      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      1952      , 1560      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
#else
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      2104      , 1264      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      1920      , 1560      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
#else
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
#endif
    }
};

static int PREVIEW_SIZE_LUT_3L8_BDS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 2322      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
#endif
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 3096      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
#else
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
#endif
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      2976      , 2976      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1488      , 1488      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
#else
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
#endif
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4208      , 2804      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
#else
      2160      , 1440      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      3888      , 3120      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
#else
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4208      , 2512      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
#else
      2400      , 1440      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
#endif
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      3808      , 3120      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
#else
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
#endif
    }
};

static int PICTURE_SIZE_LUT_3L8[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 2322      ,   /* [bcrop  ] */
      4128      , 2322      ,   /* [bds    ] */
      4128      , 2322      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 3096      ,   /* [bcrop  ] */
      4128      , 3096      ,   /* [bds    ] */
      4128      , 3096      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      2976      , 2976      ,   /* [bcrop  ] */
      2976      , 2976      ,   /* [bds    ] */
      2976      , 2976      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_3L8[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 2322      ,   /* [bcrop  ] */
      4128      , 2322      ,   /* [bds    ] */
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING)
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [target ] */
#endif
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 3096      ,   /* [bcrop  ] */
      4128      , 3096      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      2976      , 2976      ,   /* [bcrop  ] */
      2976      , 2976      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] *//* w=1080, Increased for 16 pixel align */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4208      , 2804      ,   /* [bcrop  ] */
      4208      , 2804      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      3888      , 3120      ,   /* [bcrop  ] */
      3888      , 3120      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4208      , 2512      ,   /* [bcrop  ] */
      4208      , 2512      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      3808      , 3120      ,   /* [bcrop  ] */
      3808      , 3120      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    }
};

static int VIDEO_SIZE_LUT_3L8_BNS_15[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2752      , 1548      ,   /* [bcrop  ] */
#if defined(USE_BDS_RECORDING)
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING)
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
#endif /* LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING */
#else
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING)
      2752      , 1548      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#else
      2752      , 1548      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
#endif /* LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING */
#endif /* USE_BDS_RECORDING */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2752      , 2064      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2080      , 2080      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2800      , 1868      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2608      , 2808      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2800      , 1684      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2800      , 2080      ,   /* [bns    ] */
      2544      , 2080      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_3L8_BNS_20[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      2104      , 1184      ,   /* [bcrop  ] */
#if defined(USE_BDS_RECORDING)
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
#else
      2104      , 1184      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#endif /* USE_BDS_RECORDING */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      2104      , 1560      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      1560      , 1560      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      2104      , 1402      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      1952      , 1560      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      2104      , 1264      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      2104      , 1560      ,   /* [bns    ] */
      1920      , 1560      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_3L8_BDS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 2322      ,   /* [bcrop  ] */
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING)
      2656      , 1494      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      2560      , 1440      ,   /* [target ] */
#endif
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4128      , 3096      ,   /* [bcrop  ] */
      1984      , 1488      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      2976      , 2976      ,   /* [bcrop  ] */
      1488      , 1488      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4208      , 2804      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      3888      , 3120      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      4208      , 2512      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4208 + 0 ),(3120 + 0) ,   /* [sensor ] */
      4208      , 3120      ,   /* [bns    ] */
      3808      , 3120      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_HIGH_SPEED_3L8_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* FHD_60 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2648 + 16),(1488 + 12),   /* [sensor ] *//* Sensor binning ratio = 2 */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* HD_120 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1312 + 16),( 736 + 12),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1328      ,  748      ,   /* [bns    ] */
      1312      ,  738      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    },
    /* WVGA_300 5:3 (Single) */
    { SIZE_RATIO_16_9,
     ( 808 + 16),( 484 + 12),   /* [sensor ] *//* Sensor binning ratio = 6 */
       824      ,  496      ,   /* [bns    ] */
       810      ,  486      ,   /* [bcrop  ] */
       800      ,  480      ,   /* [bds    ] */
       800      ,  480      ,   /* [target ] */
    }
};

static int VTCALL_SIZE_LUT_3L8_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
     (2648 + 16),(1488 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (VT_Call) */
    { SIZE_RATIO_4_3,
     (2648 + 16),(1488 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      1986      , 1490      ,   /* [bcrop  ] */
       960      ,  720      ,   /* [bds    ] */
       960      ,  720      ,   /* [target ] */
    },
    /* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
     (2648 + 16),(1488 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      1488      , 1488      ,   /* [bcrop  ] */
       720      ,  720      ,   /* [bds    ] */
       720      ,  720      ,   /* [target ] */
    },
    /* 11:9 (VT_Call) */
    { SIZE_RATIO_11_9,
     (2648 + 16),(1488 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
       352      ,  288      ,   /* [bds    ] */
       352      ,  288      ,   /* [target ] */
    }
};

static int S5K3L8_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE)
#else
    { 2560, 1440, SIZE_RATIO_16_9},
    { 1920, 1440, SIZE_RATIO_4_3},
    { 1440, 1440, SIZE_RATIO_1_1},
#endif
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
#endif
    { 1280,  720, SIZE_RATIO_16_9},
    { 1056,  704, SIZE_RATIO_3_2},
    { 1024,  768, SIZE_RATIO_4_3},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  720, SIZE_RATIO_1_1},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  256,  144, SIZE_RATIO_16_9}, /* for CAMERA2_API_SUPPORT */
    {  176,  144, SIZE_RATIO_11_9}
};

static int S5K3L8_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if !(defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080))
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
#endif
    { 3840, 2160, SIZE_RATIO_16_9},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 2560, 1440, SIZE_RATIO_16_9},
    { 1600, 1200, SIZE_RATIO_4_3},
    { 1280,  960, SIZE_RATIO_4_3},
    { 1056,  864, SIZE_RATIO_11_9},
    {  528,  432, SIZE_RATIO_11_9},
    {  800,  480, SIZE_RATIO_5_3},
    {  672,  448, SIZE_RATIO_3_2},
    {  480,  320, SIZE_RATIO_3_2},
    {  480,  270, SIZE_RATIO_16_9},
};

static int S5K3L8_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4128, 3096, SIZE_RATIO_4_3},
    { 4128, 2322, SIZE_RATIO_16_9},
    { 3984, 2988, SIZE_RATIO_4_3},
    { 3264, 2448, SIZE_RATIO_4_3},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 2976, 2976, SIZE_RATIO_1_1},
    { 2048, 1152, SIZE_RATIO_16_9},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  640,  480, SIZE_RATIO_4_3},
};

static int S5K3L8_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4096, 3072, SIZE_RATIO_4_3},
    { 4096, 2304, SIZE_RATIO_16_9},
    { 3840, 2160, SIZE_RATIO_16_9},
    { 3200, 2400, SIZE_RATIO_4_3},
    { 3072, 1728, SIZE_RATIO_16_9},
    { 2988, 2988, SIZE_RATIO_1_1},
    { 2656, 1494, SIZE_RATIO_16_9}, /* use S-note */
    { 2592, 1944, SIZE_RATIO_4_3},
    { 2592, 1936, SIZE_RATIO_4_3},  /* not exactly matched ratio */
    { 2560, 1920, SIZE_RATIO_4_3},
    { 2448, 2448, SIZE_RATIO_1_1},
    { 2048, 1536, SIZE_RATIO_4_3},
    {  720,  720, SIZE_RATIO_1_1}, /* dummy size for binning mode */
    {  352,  288, SIZE_RATIO_11_9}, /* dummy size for binning mode */
};

static int S5K3L8_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
/* TODO : will be supported after enable S/W scaler correctly */
//  {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int S5K3L8_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  480,  320, SIZE_RATIO_3_2},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  256,  144, SIZE_RATIO_16_9}, /* for CAMERA2_API_SUPPORT */
    {  176,  144, SIZE_RATIO_11_9},
};

static int S5K3L8_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
#ifdef USE_UHD_RECORDING
    { 3840, 2160, SIZE_RATIO_16_9},
#endif
#ifdef USE_WQHD_RECORDING
    { 2560, 1440, SIZE_RATIO_16_9},
#endif
};

static int S5K3L8_FPS_RANGE_LIST[][2] =
{
    //{   5000,   5000},
    //{   7000,   7000},
    {  15000,  15000},
    //{  24000,  24000},
    //{   4000,  30000},
    //{  10000,  30000},
    //{  15000,  30000},
    //{  30000,  30000},
};

static int S5K3L8_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  10000,  24000},
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};
#endif
