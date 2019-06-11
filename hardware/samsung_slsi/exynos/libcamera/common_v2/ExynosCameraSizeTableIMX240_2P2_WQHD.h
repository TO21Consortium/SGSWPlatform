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

#ifndef EXYNOS_CAMERA_LUT_IMX240_2P2_H
#define EXYNOS_CAMERA_LUT_IMX240_2P2_H

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
    Sensor Margin Width  = 16,
    Sensor Margin Height = 12
-----------------------------*/

static int PREVIEW_SIZE_LUT_IMX240_2P2_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1986      , 1490      ,   /* [bcrop  ] */
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
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      1504      , 1500      ,   /* [bns    ] */
      1488      , 1488      ,   /* [bcrop  ] */
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
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1344      , 1080      ,   /* [bds	  ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
#else
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
#else
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
#endif
    }
};

static int PREVIEW_SIZE_LUT_IMX240_2P2[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      2656      , 1494      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
#endif
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1984      , 1488      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
#else
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
#endif
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
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
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1344      , 1080      ,   /* [bds	  ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
#else
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
#endif
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
#else
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
#endif
    }
};

static int DUAL_PREVIEW_SIZE_LUT_IMX240_2P2_FHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1986      , 1490      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      1504      , 1500      ,   /* [bns    ] */
      1490      , 1490      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_IMX240_2P2[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      5312      , 2988      ,   /* [bds    ] */
      5312      , 2988      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      3984      , 2988      ,   /* [bds    ] */
      3984      , 2988      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      2988      , 2988      ,   /* [bds    ] */
      2988      , 2988      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      4480      , 2988      ,   /* [bds    ] */
      4480      , 2988      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      3728      , 2988      ,   /* [bds    ] */
      3728      , 2988      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      4976      , 2988      ,   /* [bds    ] */
      4976      , 2988      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      3648      , 2988      ,   /* [bds    ] */
      3648      , 2988      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_IMX240_2P2_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3536      , 1988      ,   /* [bcrop  ] */
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
      3536      , 1988      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#else
      3536      , 1988      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
#endif /* LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING */
#endif /* USE_BDS_RECORDING */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2650      , 1988      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      2004      , 2000      ,   /* [bns    ] */
      1988      , 1988      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      2982      , 1988      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2486      , 1988      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3314      , 1988      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2430      , 1988      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_IMX240_2P2[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1984      , 1488      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      1488      , 1488      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080		,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080		,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080		,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      1312      , 1080      ,	/* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080		,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX240_2P2_BNS[][SIZE_OF_LUT] =
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
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_2P2_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* HD_120 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1312 + 16),( 736 + 12),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1328      ,  748      ,   /* [bns    ] */
      1312      ,  738      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    },
    /* HD_120 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (1312 + 16),( 736 + 12),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1328      ,  748      ,   /* [bns    ] */
       960      ,  720      ,   /* [bcrop  ] */
       960      ,  720      ,   /* [bds    ] */
       960      ,  720      ,   /* [target ] */
    }
};

static int VTCALL_SIZE_LUT_IMX240_2P2_BNS[][SIZE_OF_LUT] =
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

static int YUV_SIZE_LUT_IMX240_2P2[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      5312      , 2988      ,   /* [bds    ] */
      5312      , 2988      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      3984      , 2988      ,   /* [bds    ] */
      3984      , 2988      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      2988      , 2988      ,   /* [bds    ] */
      2988      , 2988      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      4480      , 2988      ,   /* [bds    ] */
      4480      , 2988      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      3728      , 2988      ,   /* [bds    ] */
      3728      , 2988      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      4976      , 2988      ,   /* [bds    ] */
      4976      , 2988      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      3648      , 2988      ,   /* [bds    ] */
      3648      , 2988      ,   /* [target ] */
    }
};

static int YUV_SIZE_LUT_IMX240_2P2_BDS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      2160      , 1440      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      2400      , 1440      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};


static int IMX240_2P2_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
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

static int IMX240_2P2_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
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

static int IMX240_2P2_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 5312, 2988, SIZE_RATIO_16_9},
    { 3984, 2988, SIZE_RATIO_4_3},
    { 3264, 2448, SIZE_RATIO_4_3},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 2976, 2976, SIZE_RATIO_1_1},
    { 2048, 1152, SIZE_RATIO_16_9},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  256,  144, SIZE_RATIO_16_9},
};

static int IMX240_2P2_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4128, 3096, SIZE_RATIO_4_3},
    { 4128, 2322, SIZE_RATIO_16_9},
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

static int IMX240_2P2_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
/* TODO : will be supported after enable S/W scaler correctly */
//  {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int IMX240_2P2_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
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

static int IMX240_2P2_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
#ifdef USE_UHD_RECORDING
    { 3840, 2160, SIZE_RATIO_16_9},
#endif
#ifdef USE_WQHD_RECORDING
    { 2560, 1440, SIZE_RATIO_16_9},
#endif
};

static int IMX240_2P2_FPS_RANGE_LIST[][2] =
{
    //{   5000,   5000},
    //{   7000,   7000},
    {  15000,  15000},
    {  24000,  24000},
    //{   4000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int IMX240_2P2_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  10000,  24000},
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};

/* For HAL3 */
static int IMX240_2P2_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    { 5312, 2988, SIZE_RATIO_16_9},
    { 3984, 2988, SIZE_RATIO_4_3},
    { 2976, 2976, SIZE_RATIO_1_1},
    { 3264, 2448, SIZE_RATIO_4_3},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 2560, 1440, SIZE_RATIO_16_9},
    { 1920, 1440, SIZE_RATIO_4_3},
    { 1440, 1440, SIZE_RATIO_1_1},
    { 2048, 1152, SIZE_RATIO_16_9},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  720,  720, SIZE_RATIO_1_1},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  512,  384, SIZE_RATIO_4_3},
    {  480,  320, SIZE_RATIO_3_2},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  256,  144, SIZE_RATIO_16_9},
//  {  176,  144, SIZE_RATIO_11_9}, /* Too small to create thumbnail */
};

/* For HAL3 */
static int IMX240_2P2_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1280,  720, SIZE_RATIO_16_9},
};

/* For HAL3 */
static int IMX240_2P2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000, 120000},
    { 120000, 120000},
};

static camera_metadata_rational UNIT_MATRIX_IMX240_2P2_3X3[] =
{
    {128, 128}, {0, 128}, {0, 128},
    {0, 128}, {128, 128}, {0, 128},
    {0, 128}, {0, 128}, {128, 128}
};

static camera_metadata_rational COLOR_MATRIX1_IMX240_3X3[] = {
    {800, 1024}, {-172, 1024}, {-110, 1024},
    {-463, 1024}, {1305, 1024}, {146, 1024},
    {-119, 1024}, {286, 1024}, {552, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_IMX240_3X3[] = {
    {1758, 1024}, {-1014, 1024}, {-161, 1024},
    {-129, 1024}, {1119, 1024}, {134, 1024},
    {-13, 1024}, {225, 1024}, {604, 1024}
};

static camera_metadata_rational COLOR_MATRIX1_2P2_3X3[] = {
    {1094, 1024}, {-306, 1024}, {-146, 1024},
    {-442, 1024}, {1388, 1024}, {52, 1024},
    {-104, 1024}, {250, 1024}, {600, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_2P2_3X3[] = {
    {2263, 1024}, {-1364, 1024}, {-145, 1024},
    {-194, 1024}, {1257, 1024}, {-56, 1024},
    {-24, 1024}, {187, 1024}, {618, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_IMX240_3X3[] = {
    {682, 1024}, {182, 1024}, {120, 1024},
    {244, 1024}, {902, 1024}, {-122, 1024},
    {14, 1024}, {-316, 1024}, {1142, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_IMX240_3X3[] = {
    {450, 1024}, {307, 1024}, {227, 1024},
    {8, 1024}, {1049, 1024}, {-33, 1024},
    {-7, 1024}, {-968, 1024}, {1815, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_2P2_3X3[] = {
    {612, 1024}, {233, 1024}, {139, 1024},
    {199, 1024}, {831, 1024}, {-6, 1024},
    {15, 1024}, {-224, 1024}, {1049, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_2P2_3X3[] = {
    {441, 1024}, {317, 1024}, {226, 1024},
    {29, 1024}, {908, 1024}, {87, 1024},
    {9, 1024}, {-655, 1024}, {1486, 1024}
};
#endif
