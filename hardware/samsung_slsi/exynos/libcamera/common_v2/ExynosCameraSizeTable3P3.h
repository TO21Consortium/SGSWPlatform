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

#ifndef EXYNOS_CAMERA_LUT_3P3_H
#define EXYNOS_CAMERA_LUT_3P3_H

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
    Sensor Margin Height = 10
-----------------------------*/

static int PREVIEW_SIZE_LUT_3P3_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),   /* [sensor ] */
      4624      , 2602      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_3P3_BNS_DUAL[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5 for 16:9, 2.0 for 4:3 and 1:1
       BDS       : NO */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),   /* [sensor ] */
      4624      , 2602      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

/*
 * This is not BNS, BDS (just name is BNS)
 * To keep source code. just let the name be.
 */
static int PREVIEW_SIZE_LUT_3P3[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),   /* [sensor ] */
      4624      , 2602      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3072      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3686      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 2764      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4224      , 3456      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_3P3_FULL_OTF[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),//(5312 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 2602      ,//5328      , 3000      ,   /* [bns    ] */
      4616      , 2600      ,//5312      , 2988      ,   /* [bcrop  ] */
      4608      , 2592      ,//5312      , 2988      ,   /* [bds    ] */
      1920      , 1080      ,//1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),//(3984 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 3466      ,//4000      , 3000      ,   /* [bns    ] */
      4616      , 3464      ,//3984      , 2988      ,   /* [bcrop  ] */
      4608      , 3456      ,//3984      , 2988      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (3456 + 16),(3456 + 10),//(3984 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 3466      ,//4000      , 3000      ,   /* [bns    ] */
      3464      , 3464      ,//2988      , 2988      ,   /* [bcrop  ] */
      3456      , 3456      ,//2988      , 2988      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
};


static int PICTURE_SIZE_LUT_3P3[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),   /* [sensor ] */
      4624      , 2602      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      4608      , 2592      ,   /* [bds    ] */
      4608      , 2592      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      3456      , 3456      ,   /* [bds    ] */
      3456      , 3456      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      3264      , 2176      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3060      , 2448      ,   /* [bcrop  ] */
      3056      , 2448      ,   /* [bds    ] *//* w=3060, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3264      , 1958      ,   /* [bcrop  ] */
      3264      , 1958      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_3P3_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),   /* [sensor ] */
      4624      , 2602      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_3P3[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

     /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),   /* [sensor ] */
      4624      , 2602      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2236      , 1490      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2000      , 1500      ,   /* [bns    ] */
      1862      , 1490      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      2664      , 1500      ,   /* [bns    ] */
      2484      , 1490      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4224      , 3456      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_3P3_FULL_OTF[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /*  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),//(5312 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 2602      ,//5328      , 3000      ,   /* [bns    ] */
      4616      , 2600      ,//5312      , 2988      ,   /* [bcrop  ] */
      4608      , 2592      ,//5312      , 2988      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(2592 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4616      , 3464      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      640      , 480      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (3456 + 16),(3456 + 10),   /* [sensor ] */
      4626      , 3466      ,   /* [bns    ] */
      3464      , 3464      ,   /* [bcrop  ] */
      3456      , 3456      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3P3_BNS[][SIZE_OF_LUT] =
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

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3P3_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*   HD_120  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1136 + 16),( 632 + 16),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1152      ,  648      ,   /* [bns    ] */
      1148      ,  644      ,   /* [bcrop  ] */
      1136      ,  638      ,   /* [bds    ] */
      1136      ,  638      ,   /* [target ] */
    }
};

static int S5K3P3_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
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
//    {  176,  144, SIZE_RATIO_11_9}
};

static int S5K3P3_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
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

static int S5K3P3_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4608, 3456, SIZE_RATIO_4_3},
    { 4608, 2592, SIZE_RATIO_16_9},
    { 3264, 2448, SIZE_RATIO_4_3},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 2976, 2976, SIZE_RATIO_1_1},
    { 2048, 1152, SIZE_RATIO_16_9},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  640,  480, SIZE_RATIO_4_3},
};

static int S5K3P3_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
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

static int S5K3P3_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
/* TODO : will be supported after enable S/W scaler correctly */
//  {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int S5K3P3_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
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

static int S5K3P3_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
#ifdef USE_UHD_RECORDING
    { 3840, 2160, SIZE_RATIO_16_9}
#endif
};

static int S5K3P3_FPS_RANGE_LIST[][2] =
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

static int S5K3P3_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};
#endif
