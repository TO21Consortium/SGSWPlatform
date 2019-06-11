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

#ifndef EXYNOS_CAMERA_LUT_2P2_12M_H
#define EXYNOS_CAMERA_LUT_2P2_12M_H

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
    CAC_W,
    CAC_H,
    BDS_W,
    BDS_H,
    TARGET_W,
    TARGET_H,
-----------------------------
    Sensor Margin Width  = 16,
    Sensor Margin Height = 12
-----------------------------*/

static int PREVIEW_SIZE_LUT_2P2_12M_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 2.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      2312      , 1302      ,   /* [bns    ] */
      2304      , 1296      ,   /* [cac    ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      1736      , 1302      ,   /* [bns    ] */
      1728      , 1296      ,   /* [cac    ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2592 + 16),(2592 + 12),   /* [sensor ] */
      1304      , 1302      ,   /* [bns    ] */
      1296      , 1296      ,   /* [cac    ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*	3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      2312      , 1302      ,   /* [bns    ] */
      1944      , 1296      ,   /* [cac    ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080		,   /* [target ] */
    },
    /*	5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      1736      , 1302      ,   /* [bns    ] */
      1620      , 1296      ,   /* [cac    ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080		,   /* [target ] */
    },
    /*	5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      2312      , 1302      ,   /* [bns    ] */
      2160      , 1296      ,   /* [cac    ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080		,   /* [target ] */
    },
    /*	11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      1736      , 1302      ,   /* [bns    ] */
      1584      , 1296      ,   /* [cac    ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080		,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_2P2_12M[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      4624      , 2604      ,   /* [bns    ] */
      4608      , 2592      ,   /* [cac    ] */
      4608      , 2592      ,   /* [bds    ] */
      4608      , 2592      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      3472      , 2604      ,   /* [bns    ] */
      3456      , 2592      ,   /* [cac    ] */
      3456      , 2592      ,   /* [bds    ] */
      3456      , 2592      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2592 + 16),(2592 + 12),   /* [sensor ] */
      2608      , 2604      ,   /* [bns    ] */
      2592      , 2592      ,   /* [cac    ] */
      2592      , 2592      ,   /* [bds    ] */
      2592      , 2592      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P2_12M_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1080p */

    /*	16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      3080      , 1736      ,   /* [bns    ] */
      3072      , 1728      ,   /* [cac    ] */
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },
    /*	4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      2312      , 1736      ,   /* [bns    ] */
      2304      , 1728      ,   /* [cac    ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*	1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2592 + 16),(2592 + 12),   /* [sensor ] */
      1736      , 1736      ,   /* [bns    ] */
      1728      , 1728      ,   /* [cac    ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*	3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      3080      , 1736      ,   /* [bns    ] */
      2592      , 1728      ,   /* [cac    ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*	5:4 (Single) */
    { SIZE_RATIO_5_4,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      2312      , 1736      ,   /* [bns    ] */
      2160      , 1728      ,   /* [cac    ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*	5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      3080      , 1736      ,   /* [bns    ] */
      2880      , 1728      ,   /* [cac    ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*	11:9 (Single) */
    { SIZE_RATIO_11_9,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      2312      , 1736      ,   /* [bns    ] */
      2112      , 1728      ,   /* [cac    ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P2_12M[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /*  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      4624      , 2604      ,   /* [bns    ] */
      4608      , 2592      ,   /* [cac    ] */
      1920      , 1080      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      3472      , 2604      ,   /* [bns    ] */
      3456      , 2592      ,   /* [cac    ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2592 + 16),(2592 + 12),   /* [sensor ] */
      2608      , 2604      ,   /* [bns    ] */
      2592      , 2592      ,   /* [cac    ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*	3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      4624      , 2604      ,   /* [bns    ] */
      3888      , 2592      ,   /* [cac    ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080		,   /* [target ] */
    },
    /*	5:4 (Single) */
    { SIZE_RATIO_5_4,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      3472      , 2604      ,   /* [bns    ] */
      3240      , 2592      ,   /* [cac    ] */
      1344      , 1080      ,	/* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080		,   /* [target ] */
    },
    /*	5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(2592 + 12),   /* [sensor ] */
      4624      , 2604      ,   /* [bns    ] */
      4320      , 2592      ,   /* [cac    ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080		,   /* [target ] */
    },
    /*	11:9 (Single) */
    { SIZE_RATIO_11_9,
     (3456 + 16),(2592 + 12),   /* [sensor ] */
      3472      , 2604      ,   /* [bns    ] */
      3168      , 2592      ,   /* [cac    ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080		,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P2_12M_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*	FHD_60	16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2296 + 16),(1288 + 12),   /* [sensor ] *//* Sensor binning ratio = 2 */
      2312      , 1300      ,   /* [bns    ] */
      2304      , 1296      ,   /* [cac    ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P2_12M_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*	 HD_120  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1140 + 16),( 638 + 12),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1156      ,  650      ,   /* [bns    ] */
      1152      ,  648      ,   /* [cac    ] */
      1152      ,  648      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    }
};

static int VTCALL_SIZE_LUT_2P2_12M_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
     (2296 + 16),(1288 + 12),   /* [sensor ] */
      2312      , 1300      ,   /* [bns    ] */
      2304      , 1296      ,   /* [cac    ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
	/* 4:3 (VT_Call) */
    { SIZE_RATIO_4_3,
     (2296 + 16),(1288 + 12),   /* [sensor ] */
      2312      , 1300      ,   /* [bns    ] */
      1728      , 1296      ,   /* [cac    ] */
       960      ,  720      ,   /* [bds    ] */
       960      ,  720		,   /* [target ] */
    },
	/* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
     (2296 + 16),(1288 + 12),   /* [sensor ] */
      2312      , 1300      ,   /* [bns    ] */
      1296      , 1296      ,   /* [cac    ] */
       720      ,  720      ,   /* [bds    ] */
       720      ,  720		,   /* [target ] */
    },
	/* 11:9 (VT_Call) */
    { SIZE_RATIO_11_9,
     (2296 + 16),(1288 + 12),   /* [sensor ] */
      2312      , 1300      ,   /* [bns    ] */
      1584      , 1296      ,   /* [cac    ] */
       352      ,  288      ,   /* [bds    ] */
       352      ,  288		,   /* [target ] */
    }
};

static int S5K2P2_12M_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
#endif
    { 1280,  720, SIZE_RATIO_16_9},
    { 1056,  704, SIZE_RATIO_3_2},
    {  960,  720, SIZE_RATIO_4_3},
    {  720,  720, SIZE_RATIO_1_1},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  176,  144, SIZE_RATIO_11_9}
};

static int S5K2P2_12M_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
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
    { 1152,  648, SIZE_RATIO_16_9},
    {  528,  432, SIZE_RATIO_11_9},
    {  800,  480, SIZE_RATIO_5_3},
    {  672,  448, SIZE_RATIO_3_2},
    {  480,  320, SIZE_RATIO_3_2},
    {  480,  270, SIZE_RATIO_16_9},
};

static int S5K2P2_12M_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4608, 2592, SIZE_RATIO_16_9},
    { 3456, 2592, SIZE_RATIO_4_3},
    { 3264, 2448, SIZE_RATIO_4_3},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 2592, 2592, SIZE_RATIO_1_1},
    { 2048, 1536, SIZE_RATIO_4_3},
    { 2048, 1152, SIZE_RATIO_16_9},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  640,  480, SIZE_RATIO_4_3},
};

static int S5K2P2_12M_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4128, 3096, SIZE_RATIO_4_3},
    { 4128, 2322, SIZE_RATIO_16_9},
    { 4096, 3072, SIZE_RATIO_4_3},
    { 4096, 2304, SIZE_RATIO_16_9},
    { 3840, 2160, SIZE_RATIO_16_9},
    { 3200, 2400, SIZE_RATIO_4_3},
    { 3072, 1728, SIZE_RATIO_16_9},
    { 2988, 2988, SIZE_RATIO_1_1},
    { 2592, 1944, SIZE_RATIO_4_3},
    { 2592, 1936, SIZE_RATIO_4_3},  /* not exactly matched ratio */
    { 2560, 1920, SIZE_RATIO_4_3},
    { 2448, 2448, SIZE_RATIO_1_1},
    {  720,  720, SIZE_RATIO_1_1}, /* dummy size for binning mode */
    {  352,  288, SIZE_RATIO_11_9}, /* dummy size for binning mode */
};

static int S5K2P2_12M_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
    {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int S5K2P2_12M_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
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

static int S5K2P2_12M_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
#ifdef USE_UHD_RECORDING
    { 3840, 2160, SIZE_RATIO_16_9}
#endif
};

static int S5K2P2_12M_FPS_RANGE_LIST[][2] =
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

static int S5K2P2_12M_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};
#endif
