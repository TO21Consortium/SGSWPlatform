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

#ifndef EXYNOS_CAMERA_LUT_2P8_H
#define EXYNOS_CAMERA_LUT_2P8_H

//#include "ExynosCameraConfig.h"

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

static int PREVIEW_SIZE_LUT_2P8[][SIZE_OF_LUT] =
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
      2560      , 1440      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      3984      , 2988      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      2988      , 2988      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      4480      , 2988      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      3728      , 2988      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      4976      , 2988      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      3648      , 2988      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_2P8_BDS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
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
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_2P8_BDS_BNS15[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3536      , 1988      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2650      , 1988      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      2004      , 2000      ,   /* [bns    ] */
      1988      , 1988      ,   /* [bcrop  ] */
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      2982      , 1988      ,   /* [bcrop  ] */
      2160      , 1440      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2486      , 1988      ,   /* [bcrop  ] */
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3314      , 1988      ,   /* [bcrop  ] */
      2400      , 1440      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2430      , 1988      ,   /* [bcrop  ] */
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_2P8_BDS_BNS20_WQHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1986      , 1490      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      1504      , 1500      ,   /* [bns    ] */
      1490      , 1490      ,   /* [bcrop  ] */
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      2160      , 1440      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      2400      , 1440      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_2P8_BDS_BNS20_FHD[][SIZE_OF_LUT] =
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
      1616      , 1080      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    }
};

static int PICTURE_SIZE_LUT_2P8[][SIZE_OF_LUT] =
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
    }
};

static int VIDEO_SIZE_LUT_2P8_WQHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      5312      , 2988      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      2560      , 1440      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      3984      , 2988      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      2988      , 2988      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      4480      , 2988      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      3728      , 2988      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      4976      , 2988      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      3648      , 2988      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P8_BDS_WQHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      2656      , 1494      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      2560      , 1440      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1992      , 1494      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      1494      , 1494      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      2240      , 1494      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      1864      , 1494      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      2488      , 1494      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      1824      , 1494      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P8_BDS_DIS_WQHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      2656      , 1494      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      2560      , 1440      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1992      , 1494      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      1494      , 1494      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_2P8_BDS_BNS15_WQHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3536      , 1988      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      2560      , 1440      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2650      , 1988      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      2004      , 2000      ,   /* [bns    ] */
      1988      , 1988      ,   /* [bcrop  ] */
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      2982      , 1988      ,   /* [bcrop  ] */
      2160      , 1440      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2486      , 1988      ,   /* [bcrop  ] */
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3314      , 1988      ,   /* [bcrop  ] */
      2400      , 1440      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2430      , 1988      ,   /* [bcrop  ] */
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P8_BDS_BNS20_WQHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2656      , 1494      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      2560      , 1440      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1992      , 1494      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      1504      , 1500      ,   /* [bns    ] */
      1490      , 1490      ,   /* [bcrop  ] */
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      2160      , 1440      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      2400      , 1440      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P8_FHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      5312      , 2988      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      3984      , 2988      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      2988      , 2988      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] *//* w=1080, Increased for 16 pixel align */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      4480      , 2988      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      3728      , 2988      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      4976      , 2988      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      3648      , 2988      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    }
};

static int VIDEO_SIZE_LUT_2P8_BDS_FHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3648      , 2988      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P8_BDS_DIS_FHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      2656      , 1494      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1992      , 1494      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      1494      , 1494      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_2P8_BDS_BNS15_FHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3536      , 1988      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
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
      1088      , 1088      ,   /* [bds    ] *//* w=1088, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },

    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      2982      , 1988      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },

    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2486      , 1988      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },

    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3314      , 1988      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },

    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2430      , 1988      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P8_BDS_BNS15_DIS_FHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3536      , 1988      ,   /* [bcrop  ] */
      2656      , 1494      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2650      , 1988      ,   /* [bcrop  ] */
      1992      , 1494      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      2004      , 2000      ,   /* [bns    ] */
      1988      , 1988      ,   /* [bcrop  ] */
      1494      , 1494      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_2P8_BDS_BNS20_FHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
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
      1088      , 1088      ,   /* [bds    ] *//* w=1088, Increased for 16 pixel align */
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

static int VIDEO_SIZE_LUT_2P8_BDS_BNS20_DIS_FHD[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2656      , 1494      ,   /* [bcrop  ] */
      2656      , 1494      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },

    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1992      , 1494      ,   /* [bcrop  ] */
      1992      , 1494      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },

    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      1504      , 1500      ,   /* [bns    ] */
      1494      , 1494      ,   /* [bcrop  ] */
      1494      , 1494      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P8[][SIZE_OF_LUT] =
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
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P8[][SIZE_OF_LUT] =
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

static int VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2P8[][SIZE_OF_LUT] =
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

static int VIDEO_SIZE_LUT_HIGH_SPEED_2P8[][SIZE_OF_LUT] =
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
    { SIZE_RATIO_5_3,
     ( 808 + 16),( 484 + 12),   /* [sensor ] *//* Sensor binning ratio = 6 */
       824      ,  496      ,   /* [bns    ] */
       810      ,  486      ,   /* [bcrop  ] */
       800      ,  480      ,   /* [bds    ] */
       800      ,  480      ,   /* [target ] */
    }
};

static int YUV_SIZE_LUT_2P8[][SIZE_OF_LUT] =
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

static int VTCALL_SIZE_LUT_2P8[][SIZE_OF_LUT] =
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

static int LIVE_BROADCAST_SIZE_LUT_2P8[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 */
    { SIZE_RATIO_16_9,
     (2648 + 16),(1488 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 */
    { SIZE_RATIO_4_3,
     (2648 + 16),(1488 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      1986      , 1490      ,   /* [bcrop  ] */
       960      ,  720      ,   /* [bds    ] */
       960      ,  720      ,   /* [target ] */
    },
    /* 1:1 */
    { SIZE_RATIO_1_1,
     (2648 + 16),(1488 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      1488      , 1488      ,   /* [bcrop  ] */
       720      ,  720      ,   /* [bds    ] */
       720      ,  720      ,   /* [target ] */
    },
    /* 11:9 */
    { SIZE_RATIO_11_9,
     (2648 + 16),(1488 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
       352      ,  288      ,   /* [bds    ] */
       352      ,  288      ,   /* [target ] */
    }
};

static int S5K2P8_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_2560_1440)
    { 2560, 1440, SIZE_RATIO_16_9},
    { 1920, 1440, SIZE_RATIO_4_3},
    { 1440, 1440, SIZE_RATIO_1_1},
#endif
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    { 1056,  704, SIZE_RATIO_3_2},
    { 1024,  768, SIZE_RATIO_4_3},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  720, SIZE_RATIO_1_1},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
/* HACK: Exynos8890 EVT0 is not support */
#if !defined (JUNGFRAU_EVT0)
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
/* HACK: Exynos7870 is not support */
#if !defined (SMDK7870)
    {  256,  144, SIZE_RATIO_16_9}, /* for CAMERA2_API_SUPPORT */
    {  176,  144, SIZE_RATIO_11_9}
#endif
#endif
};

static int S5K2P8_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_2560_1440)
    { 3840, 2160, SIZE_RATIO_16_9},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 2560, 1440, SIZE_RATIO_16_9},
    { 1600, 1200, SIZE_RATIO_4_3},
#endif
    { 1280,  960, SIZE_RATIO_4_3},
    { 1056,  864, SIZE_RATIO_11_9},
    {  960,  960, SIZE_RATIO_1_1},  /* For clip movie */
    {  960,  540, SIZE_RATIO_16_9}, /* for GearVR*/
    {  800,  600, SIZE_RATIO_4_3},  /* for GearVR */
    {  800,  480, SIZE_RATIO_5_3},
    {  672,  448, SIZE_RATIO_3_2},
    {  640,  360, SIZE_RATIO_16_9}, /* for SWIS & GearVR*/
    {  528,  432, SIZE_RATIO_11_9},
/* HACK: Exynos8890 EVT0 is not support */
#if !defined (JUNGFRAU_EVT0)
    {  480,  320, SIZE_RATIO_3_2},
    {  480,  270, SIZE_RATIO_16_9},
#endif
};

static int S5K2P8_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
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
};

static int S5K2P8_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
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

static int S5K2P8_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
/* TODO : will be supported after enable S/W scaler correctly */
    /* {  320,  240, SIZE_RATIO_4_3}, */
    {    0,    0, SIZE_RATIO_1_1}
};

static int S5K2P8_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
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

static int S5K2P8_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 3840, 2160, SIZE_RATIO_16_9},
    { 2560, 1440, SIZE_RATIO_16_9},
    {  960,  960, SIZE_RATIO_1_1},  /* For clip movie */
    {  864,  480, SIZE_RATIO_16_9}, /* for PLB mode */
    {  432,  240, SIZE_RATIO_16_9}, /* for PLB mode */
};

static int S5K2P8_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1280,  720, SIZE_RATIO_16_9},
};

static int S5K2P8_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000, 120000},
    { 120000, 120000},
};

static int S5K2P8_FPS_RANGE_LIST[][2] =
{
    /*
    {   5000,   5000},
    {   7000,   7000},
    */
    {  15000,  15000},
    {  24000,  24000},
    /* {   4000,  30000}, */
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int S5K2P8_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  10000,  24000},
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};

/* For HAL3 */
static int S5K2P8_YUV_LIST[][SIZE_OF_RESOLUTION] =
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

static camera_metadata_rational UNIT_MATRIX_2P8_3X3[] =
{
    {128, 128}, {0, 128}, {0, 128},
    {0, 128}, {128, 128}, {0, 128},
    {0, 128}, {0, 128}, {128, 128}
};

static camera_metadata_rational COLOR_MATRIX1_2P8_3X3[] = {
    {800, 1024}, {-172, 1024}, {-110, 1024},
    {-463, 1024}, {1305, 1024}, {146, 1024},
    {-119, 1024}, {286, 1024}, {552, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_2P8_3X3[] = {
    {1758, 1024}, {-1014, 1024}, {-161, 1024},
    {-129, 1024}, {1119, 1024}, {134, 1024},
    {-13, 1024}, {225, 1024}, {604, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_2P8_3X3[] = {
    {682, 1024}, {182, 1024}, {120, 1024},
    {244, 1024}, {902, 1024}, {-122, 1024},
    {14, 1024}, {-316, 1024}, {1142, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_2P8_3X3[] = {
    {450, 1024}, {307, 1024}, {227, 1024},
    {8, 1024}, {1049, 1024}, {-33, 1024},
    {-7, 1024}, {-968, 1024}, {1815, 1024}
};
#endif
