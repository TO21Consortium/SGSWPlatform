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

#ifndef EXYNOS_CAMERA_LUT_IMX219_H
#define EXYNOS_CAMERA_LUT_IMX219_H

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

static int PREVIEW_SIZE_LUT_IMX219[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (3264 + 16),(1836 + 10),   /* [sensor ] */
      3280      , 1846      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3264      , 2448      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      2448      , 2448      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3060      , 2448      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3264      , 1958      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_IMX219[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (3264 + 16),(1836 + 10),   /* [sensor ] */
      3280      , 1846      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      3264      , 1836      ,   /* [bds    ] */
      3264      , 1836      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3264      , 2448      ,   /* [bcrop  ] */
      3264      , 2448      ,   /* [bds    ] */
      3264      , 2448      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      2448      , 2448      ,   /* [bcrop  ] */
      2448      , 2448      ,   /* [bds    ] */
      2448      , 2448      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_IMX219[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /*  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (3264 + 16),(1836 + 10),   /* [sensor ] */
      3280      , 1846      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3264      , 2448      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      2448      , 2448      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] *//* w=1080, Reduced for 16 pixel align */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] *//* w=1620, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3060      , 2448      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=1350, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      3264      , 1958      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] *//* w=1800, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3264 + 16),(2448 + 10),   /* [sensor ] */
      3280      , 2458      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX219[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = OFF */

    /*   HD_60  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1624 + 16),( 914 + 10),   /* [sensor ] */
      1640      ,  924      ,   /* [bns    ] */
      1632      ,  918      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX219[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = OFF */

    /*  HD_120  4:3 (Single) */
    { SIZE_RATIO_4_3,
     ( 800 + 16),( 450 + 10),   /* [sensor ] */
       816      ,  460      ,   /* [bns    ] */
       800      ,  450      ,   /* [bcrop  ] */
       800      ,  450      ,   /* [bds    ] */
       800      ,  450      ,   /* [target ] */
    }
};

static int IMX219_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
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
};

static int IMX219_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    { 3840, 2160, SIZE_RATIO_16_9},
    { 1600, 1200, SIZE_RATIO_4_3},
    { 1280,  960, SIZE_RATIO_4_3},
#endif
    { 1056,  864, SIZE_RATIO_11_9},
    {  528,  432, SIZE_RATIO_11_9},
    {  800,  480, SIZE_RATIO_5_3},
    {  672,  448, SIZE_RATIO_3_2},
    {  480,  320, SIZE_RATIO_3_2},
    {  480,  270, SIZE_RATIO_16_9},
};

static int IMX219_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 3264, 2448, SIZE_RATIO_4_3},
    { 3264, 1836, SIZE_RATIO_16_9},
    { 2448, 2448, SIZE_RATIO_1_1},
    { 2048, 1536, SIZE_RATIO_4_3},
    { 2048, 1152, SIZE_RATIO_16_9},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  640,  480, SIZE_RATIO_4_3},
};

static int IMX219_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 3200, 2400, SIZE_RATIO_4_3},
    { 3072, 1728, SIZE_RATIO_16_9},
    { 2988, 2988, SIZE_RATIO_1_1},
    { 2976, 2976, SIZE_RATIO_1_1},
    { 2592, 1944, SIZE_RATIO_4_3},
    { 2592, 1936, SIZE_RATIO_4_3},  /* not exactly matched ratio */
    { 2560, 1920, SIZE_RATIO_4_3},
    { 2448, 2448, SIZE_RATIO_1_1},
    { 2048, 1536, SIZE_RATIO_4_3},
};

static int IMX219_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
    {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int IMX219_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
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
    {  176,  144, SIZE_RATIO_11_9}
};

static int IMX219_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
#ifdef USE_UHD_RECORDING
    { 3840, 2160, SIZE_RATIO_16_9}
#endif
};

static int IMX219_FPS_RANGE_LIST[][2] =
{
    {  20000,  20000},
    {  20000,  24000},
    {  24000,  24000}
};

static int IMX219_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};
#endif

