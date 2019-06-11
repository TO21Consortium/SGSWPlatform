/*
**
**copyright 2013, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_LUT_2P2_H
#define EXYNOS_CAMERA_LUT_2P2_H

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

static int PREVIEW_SIZE_LUT_2P2_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1986      , 1490      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      1504      , 1500      ,   /* [bns    ] */
      1488      , 1488      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Incread for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_2P2_BNS_DUAL[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5 for 16:9, 2.0 for 4:3 and 1:1
       BDS       : NO */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3536      , 1988      ,   /* [bcrop  ] */
      3536      , 1988      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1984      , 1448      ,   /* [bcrop  ] */
      1984      , 1448      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1488      , 1488      ,   /* [bcrop  ] */
      1488      , 1488      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      2236      , 1490      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1862      , 1490      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      2484      , 1490      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1822      , 1490      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    }
};

/*
 * This is not BNS, BDS (just name is BNS)
 * To keep source code. just let the name be.
 */
static int PREVIEW_SIZE_LUT_2P2[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Incread for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3684      , 2988      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_2P2_FULL_OTF[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      5312      , 2988      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      3984      , 2988      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single) */
    { SIZE_RATIO_1_1,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      2988      , 2988      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4496      , 2988      ,   /* [bcrop  ] */
      4496      , 2988      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      3728      , 2988      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      4976      , 2988      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3684      , 2988      ,   /* [bcrop  ] */
      3684      , 2988      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    }
};


static int PICTURE_SIZE_LUT_2P2[][SIZE_OF_LUT] =
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
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      3984      , 2988      ,   /* [bds    ] */
      3984      , 2988      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      2988      , 2988      ,   /* [bds    ] */
      2988      , 2988      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P2_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1080p */

    /*  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3536      , 1988      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2650      , 1988      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      2004      , 2000      ,   /* [bns    ] */
      1988      , 1988      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      2982      , 1988      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2486      , 1988      ,   /* [bcrop  ] */
      1344      , 1088      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      3552      , 2000      ,   /* [bns    ] */
      3314      , 1988      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      2664      , 2000      ,   /* [bns    ] */
      2430      , 1988      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P2[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /*  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2992 + 16),(2988 + 12),   /* [sensor ] */
      3008      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4480      , 2988      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 16),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3684      , 2988      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixe align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P2_FULL_OTF[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /*  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      5312      , 2988      ,   /* [bcrop  ] */
      5312      , 2988      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3984      , 2988      ,   /* [bcrop  ] */
      3984      , 2988      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      2988      , 2988      ,   /* [bcrop  ] */
      2988      , 2988      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single) */
    { SIZE_RATIO_3_2,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4496      , 2988      ,   /* [bcrop  ] */
      4496      , 2988      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single) */
    { SIZE_RATIO_5_4,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3728      , 2988      ,   /* [bcrop  ] */
      3728      , 2988      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single) */
    { SIZE_RATIO_5_3,
     (5312 + 16),(2988 + 12),   /* [sensor ] */
      5328      , 3000      ,   /* [bns    ] */
      4976      , 2988      ,   /* [bcrop  ] */
      4976      , 2988      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single) */
    { SIZE_RATIO_11_9,
     (3984 + 16),(2988 + 12),   /* [sensor ] */
      4000      , 3000      ,   /* [bns    ] */
      3684      , 2988      ,   /* [bcrop  ] */
      3684      , 2988      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P2_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*  FHD_60  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2648 + 16),(1488 + 12),   /* [sensor ] *//* Sensor binning ratio = 2 */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P2_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*   HD_120  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1312 + 16),( 736 + 12),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1328      ,  748      ,   /* [bns    ] */
      1312      ,  738      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_HIGH_SPEED_2P2_BNS_FULL_OTF[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = OFF */

    /*  FHD_60  16:9 () */
    { SIZE_RATIO_16_9,
     (2648 + 16),(1488 + 12),   /* [sensor ] *//* Sensor binning ratio = 2 */
      2664      , 1500      ,   /* [bns    ] */
      2648      , 1490      ,   /* [bcrop  ] */
      2648      , 1490      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*   HD_120  16:9 (Fast AE) */
    { SIZE_RATIO_16_9,
     (1312 + 16),( 736 + 12),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1328      ,  748      ,   /* [bns    ] */
      1312      ,  738      ,   /* [bcrop  ] */
      1312      ,  738      ,   /* [bns    ] */
      1280      ,  720      ,   /* [target ] */
    },
};

static int S5K2P2_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
#endif
    { 1280,  720, SIZE_RATIO_16_9},
    { 1056,  704, SIZE_RATIO_3_2},
    { 1024,  768, SIZE_RATIO_4_3},
    {  960,  720, SIZE_RATIO_4_3},
    {  720,  720, SIZE_RATIO_1_1},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  176,  144, SIZE_RATIO_11_9}
};

static int S5K2P2_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if !(defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080))
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
#endif
    { 3840, 2160, SIZE_RATIO_16_9},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 1600, 1200, SIZE_RATIO_4_3},
    { 1280,  960, SIZE_RATIO_4_3},
    { 1056,  864, SIZE_RATIO_11_9},
    {  528,  432, SIZE_RATIO_11_9},
    {  800,  480, SIZE_RATIO_5_3},
    {  672,  448, SIZE_RATIO_3_2},
    {  480,  320, SIZE_RATIO_3_2},
    {  480,  270, SIZE_RATIO_16_9},
};

static int S5K2P2_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
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

static int S5K2P2_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4608, 2592, SIZE_RATIO_16_9},
    { 4128, 3096, SIZE_RATIO_4_3},
    { 4128, 2322, SIZE_RATIO_16_9},
    { 4096, 3072, SIZE_RATIO_4_3},
    { 4096, 2304, SIZE_RATIO_16_9},
    { 3840, 2160, SIZE_RATIO_16_9},
    { 3456, 2592, SIZE_RATIO_4_3},
    { 3200, 2400, SIZE_RATIO_4_3},
    { 3072, 1728, SIZE_RATIO_16_9},
    { 2988, 2988, SIZE_RATIO_1_1},
    { 2592, 2592, SIZE_RATIO_1_1},
    { 2592, 1944, SIZE_RATIO_4_3},
    { 2592, 1936, SIZE_RATIO_4_3},  /* not exactly matched ratio */
    { 2560, 1920, SIZE_RATIO_4_3},
    { 2448, 2448, SIZE_RATIO_1_1},
    { 2048, 1536, SIZE_RATIO_4_3},
};

static int S5K2P2_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
/* TODO : will be supported after enable S/W scaler correctly */
//  {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int S5K2P2_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  480,  320, SIZE_RATIO_3_2},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  176,  144, SIZE_RATIO_11_9}
};

static int S5K2P2_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
#ifdef USE_UHD_RECORDING
    { 3840, 2160, SIZE_RATIO_16_9}
#endif
};

static int S5K2P2_FPS_RANGE_LIST[][2] =
{
    {   5000,   5000},
    {   7000,   7000},
    {  15000,  15000},
    {  24000,  24000},
    {   4000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int S5K2P2_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};
#endif
