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

#ifndef EXYNOS_CAMERA_LUT_6D1_H
#define EXYNOS_CAMERA_LUT_6D1_H

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
    Sensor Margin Height = 16
-----------------------------*/

static int PREVIEW_SIZE_LUT_6D1[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1920      , 1440      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1440      , 1440      ,   /* [bcrop  ] */
      1440      , 1440      ,   /* [bds    ] */
      1072      , 1072      ,   /* [target ] *//* w=1080, Reduced for 16 pixel align */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2160      , 1440      ,   /* [bcrop  ] */
      2160      , 1440      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1792      , 1440      ,   /* [bcrop  ] */
      1792      , 1440      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2400      , 1440      ,   /* [bcrop  ] */
      2400      , 1440      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1760      , 1440      ,   /* [bcrop  ] */
      1760      , 1440      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    }
};

static int PICTURE_SIZE_LUT_6D1[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1920      , 1440      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },
    /* 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1440      , 1440      ,   /* [bcrop  ] */
      1440      , 1440      ,   /* [bds    ] */
      1440      , 1440      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_6D1[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#endif
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1920      , 1440      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
#else
      1920      , 1440      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
#endif
    },
    /* 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1440      , 1440      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING
      1072      , 1072      ,   /* [bds    ] */
      1072      , 1072      ,   /* [target ] *//* w=1080, Reduced for 16 pixel align */
#else
      1440      , 1440      ,   /* [bds    ] */
      1072      , 1072      ,   /* [target ] *//* w=1080, Reduced for 16 pixel align */
#endif
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2160      , 1440      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING
      1616      , 1080      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
#else
      2160      , 1440      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
#endif
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1792      , 1440      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING
      1344      , 1080      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
#else
      1792      , 1440      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
#endif
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2400      , 1440      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING
      1792      , 1080      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
#else
      2400      , 1440      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
#endif
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1760      , 1440      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING
      1312      , 1080      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
#else
      1760      , 1440      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
#endif
    }
};

#ifdef ENABLE_8MP_FULL_FRAME
static int VIDEO_SIZE_LUT_6D1_8MP_FULL[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },
    /* 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
    }
};
#endif

static int DUAL_PREVIEW_SIZE_LUT_6D1[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Dual) */
    { SIZE_RATIO_16_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Dual) */
    { SIZE_RATIO_4_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1920      , 1440      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Dual) */
    { SIZE_RATIO_1_1,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1440      , 1440      ,   /* [bcrop  ] */
      1072      , 1072      ,   /* [bds    ] */
      1072      , 1072      ,   /* [target ] *//* w=1080, Reduced for 16 pixel align */
    },
    /* 3:2 (Dual) */
    { SIZE_RATIO_3_2,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2160      , 1440      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Dual) */
    { SIZE_RATIO_5_4,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1800      , 1440      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Dual) */
    { SIZE_RATIO_5_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2400      , 1440      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Dual) */
    { SIZE_RATIO_11_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1760      , 1440      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int DUAL_VIDEO_SIZE_LUT_6D1[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Dual) */
    { SIZE_RATIO_16_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2560      , 1440      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Dual) */
    { SIZE_RATIO_4_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1920      , 1440      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Dual) */
    { SIZE_RATIO_1_1,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1440      , 1440      ,   /* [bcrop  ] */
      1072      , 1072      ,   /* [bds    ] */
      1072      , 1072      ,   /* [target ] *//* w=1080, Reduced for 16 pixel align */
    },
    /* 3:2 (Dual) */
    { SIZE_RATIO_3_2,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2160      , 1440      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Dual) */
    { SIZE_RATIO_5_4,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1800      , 1440      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Dual) */
    { SIZE_RATIO_5_3,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      2400      , 1440      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Dual) */
    { SIZE_RATIO_11_9,
     (2560 + 16),(1440 + 16),   /* [sensor ] */
      2576      , 1456      ,   /* [bns    ] */
      1760      , 1440      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int S5K6D1_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
#endif
    { 1280,  720, SIZE_RATIO_16_9},
    { 1056,  704, SIZE_RATIO_3_2},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  720, SIZE_RATIO_1_1},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
#ifdef ENABLE_8MP_FULL_FRAME
    {  256,  144, SIZE_RATIO_16_9},
#endif
    {  176,  144, SIZE_RATIO_11_9}
};

static int S5K6D1_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if !(defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080))
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
#endif
    { 2560, 1440, SIZE_RATIO_16_9},
    { 1056,  864, SIZE_RATIO_11_9},
    { 1024,  768, SIZE_RATIO_4_3},
    {  800,  600, SIZE_RATIO_4_3},
    {  800,  480, SIZE_RATIO_5_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  480, SIZE_RATIO_3_2},
    {  672,  448, SIZE_RATIO_3_2},
    {  528,  432, SIZE_RATIO_11_9},
    {  480,  320, SIZE_RATIO_3_2},
    {  480,  270, SIZE_RATIO_16_9},
};

static int S5K6D1_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2560, 1440, SIZE_RATIO_16_9},
    { 2048, 1536, SIZE_RATIO_4_3},
    { 1920, 1440, SIZE_RATIO_4_3},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1440, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  640,  480, SIZE_RATIO_4_3},
};

static int S5K6D1_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1600, 1200, SIZE_RATIO_4_3},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1280,  960, SIZE_RATIO_4_3},
    { 1024,  768, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
    {  800,  600, SIZE_RATIO_4_3},
    {  800,  480, SIZE_RATIO_5_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  480, SIZE_RATIO_3_2},
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_11_9},
    {  480,  320, SIZE_RATIO_3_2},
    {  320,  240, SIZE_RATIO_4_3},
    {  320,  180, SIZE_RATIO_16_9},
};

static int S5K6D1_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
    {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int S5K6D1_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  480,  320, SIZE_RATIO_3_2},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
#ifdef ENABLE_8MP_FULL_FRAME
    {  256,  144, SIZE_RATIO_16_9}
#else
    {  176,  144, SIZE_RATIO_11_9}
#endif
};

static int S5K6D1_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
#ifdef USE_WQHD_RECORDING
    { 2560, 1440, SIZE_RATIO_16_9},
#endif
};

static int S5K6D1_FPS_RANGE_LIST[][2] =
{
    {   5000,   5000},
    {   7000,   7000},
    {  15000,  15000},
    {  24000,  24000},
    {   4000,  30000},
    {   8000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int S5K6D1_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  25000,  30000},
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};
#endif
