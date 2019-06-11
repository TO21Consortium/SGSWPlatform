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

#ifndef EXYNOS_CAMERA_LUT_4E6_H
#define EXYNOS_CAMERA_LUT_4E6_H

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

static int PREVIEW_SIZE_LUT_4E6[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1458      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
#else
      2560      , 1440      ,   /* [bds    ] */
      2560      , 1440      ,   /* [target ] */
#endif
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
#else
      1920      , 1440      ,   /* [bds	 ] */
      1920      , 1440      ,   /* [target ] */
#endif
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2592 + 16), (1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      1936      , 1936      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1072      , 1072      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1072      , 1072      ,   /* [target ] */
#else
      1440      , 1440      ,   /* [bds	 ] *//* w=1080, Increased for 16 pixel align */
      1440      , 1440      ,   /* [target ] */
#endif
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1728      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1616      , 1080      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
#else
      2160      , 1440      ,   /* [bds    ] */
      2160      , 1440      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
#endif
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2432      , 1950      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1344      , 1080      ,   /* [bds	 ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
#else
      1792      , 1440      ,   /* [bds    ] */
      1792      , 1440      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
#endif
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1554      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1792      , 1080      ,   /* [bds	 ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
#else
      2400      , 1440      ,   /* [bds    ] */
      2400      , 1440      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
#endif
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2368      , 1950      ,   /* [bcrop  ] */
#ifdef LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE
      1312      , 1080      ,   /* [bds	 ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
#else
      1760      , 1440      ,   /* [bds    ] */
      1760      , 1440      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
#endif
    }
};

static int DUAL_PREVIEW_SIZE_LUT_4E6[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1458      ,   /* [bcrop  ] */
      2592      , 1458      ,   /* [bds    ] */
      2592      , 1458      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds	 ] */
      2592      , 1944      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2592 + 16), (1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      1936      , 1936      ,   /* [bcrop  ] */
      1936      , 1936      ,   /* [bds	 ] *//* w=1080, Increased for 16 pixel align */
      1936      , 1936      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1728      ,   /* [bcrop  ] */
      2592      , 1728      ,   /* [bds    ] */
      2592      , 1728      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2432      , 1950      ,   /* [bcrop  ] */
      2432      , 1950      ,   /* [bds    ] */
      2432      , 1950      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1554      ,   /* [bcrop  ] */
      2592      , 1554      ,   /* [bds    ] */
      2592      , 1554      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2368      , 1950      ,   /* [bcrop  ] */
      2368      , 1950      ,   /* [bds    ] */
      2368      , 1950      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    }
};

static int PICTURE_SIZE_LUT_4E6[][SIZE_OF_LUT] =
{
    { SIZE_RATIO_16_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1458      ,   /* [bcrop  ] */
      2592      , 1458      ,   /* [bds    ] */
      2592      , 1458      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds    ] */
      2592      , 1944      ,   /* [target ] */
    },
    /*  1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      1936      , 1936      ,   /* [bcrop  ] */
      1936      , 1936      ,   /* [bds    ] */
      1936      , 1936      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1728      ,   /* [bcrop  ] */
      2592      , 1728      ,   /* [bds    ] */
      2592      , 1728      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2432      , 1950      ,   /* [bcrop  ] */
      2432      , 1950      ,   /* [bds    ] */
      2432      , 1950      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2560      , 1536      ,   /* [bcrop  ] */
      2560      , 1536      ,   /* [bds    ] */
      2560      , 1536      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2384      , 1950      ,   /* [bcrop  ] */
      2384      , 1950      ,   /* [bds    ] */
      2384      , 1950      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_4E6[][SIZE_OF_LUT] =
{
    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1458      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      1936      , 1936      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1728      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2432      , 1950      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1554      ,   /* [bcrop  ] */
      2400      , 1440      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2368      , 1950      ,   /* [bcrop  ] */
      1760      , 1440      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    }
};

#if defined(ENABLE_8MP_FULL_FRAME) || defined(ENABLE_13MP_FULL_FRAME)
static int VIDEO_SIZE_LUT_4E6_FULL[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },
    /* 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1920      , 1440      ,   /* [bds    ] */
      1920      , 1440      ,   /* [target ] */
    }
};
#endif

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_4E6[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*   HD_60  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1624 + 16),( 914 + 10),   /* [sensor ] *//* Sensor binning ratio = 2 */
      1640      ,  924      ,   /* [bns    ] */
      1632      ,  918      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4E6[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*  HD_120  4:3 (Single) */
    { SIZE_RATIO_4_3,
     ( 636 + 16),( 478 + 10),   /* [sensor ] *//* Sensor binning ratio = 4 */
       652      ,  488      ,   /* [bns    ] */
       624      ,  468      ,   /* [bcrop  ] */
       624      ,  468      ,   /* [bds    ] */
       624      ,  468      ,   /* [target ] */
    }
};

static int YUV_SIZE_LUT_4E6[][SIZE_OF_LUT] =
{
    { SIZE_RATIO_16_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1458      ,   /* [bcrop  ] */
      2592      , 1458      ,   /* [bds    ] */
      2592      , 1458      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds    ] */
      2592      , 1944      ,   /* [target ] */
    },
    /*  1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      1936      , 1936      ,   /* [bcrop  ] */
      1936      , 1936      ,   /* [bds    ] */
      1936      , 1936      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1728      ,   /* [bcrop  ] */
      2592      , 1728      ,   /* [bds    ] */
      2592      , 1728      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2432      , 1950      ,   /* [bcrop  ] */
      2432      , 1950      ,   /* [bds    ] */
      2432      , 1950      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2560      , 1536      ,   /* [bcrop  ] */
      2560      , 1536      ,   /* [bds    ] */
      2560      , 1536      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2592 + 16), (1950 + 10),  /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2384      , 1950      ,   /* [bcrop  ] */
      2384      , 1950      ,   /* [bds    ] */
      2384      , 1950      ,   /* [target ] */
    }
};

static int VTCALL_SIZE_LUT_4E6[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
     (1288 + 16),(968 + 12),   /* [sensor ] */
      1304      ,  980     ,   /* [bns    ] */
      1280      ,  720     ,   /* [bcrop  ] */
      1280      ,  720     ,   /* [bds    ] */
      1280      ,  720     ,   /* [target ] */
    },
    /* 4:3 (VT_Call) */
    /* Bcrop size 1152*864 -> 1280*960, for flicker algorithm */
    { SIZE_RATIO_4_3,
    (1288 + 16),(968 + 12),   /* [sensor ] */
     1304      ,  980     ,   /* [bns	 ] */
     1280      ,  960     ,   /* [bcrop  ] */
      960      ,  720     ,   /* [bds    ] */
      960      ,  720     ,   /* [target ] */
    },
    /* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
    (1288 + 16),(968 + 12),   /* [sensor ] */
     1304      ,  980	  ,   /* [bns	 ] */
      976      ,  976     ,   /* [bcrop  ] */
      720      ,  720     ,   /* [bds    ] */
      720      ,  720     ,   /* [target ] */
    },
    /* 11:9 (VT_Call) */
    /* Bcrop size 1056*864 -> 1168*960, for flicker algorithm */
    { SIZE_RATIO_11_9,
    (1288 + 16),(968 + 12),   /* [sensor ] */
     1304      ,  980     ,   /* [bns	 ] */
     1168      ,  960     ,   /* [bcrop  ] */
      352      ,  288     ,   /* [bds    ] */
      352      ,  288     ,   /* [target ] */
    }
};

static int DUAL_VIDEO_SIZE_LUT_4E6[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Dual) */
    { SIZE_RATIO_16_9,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1458      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Dual) */
    { SIZE_RATIO_1_1,
     (2592 + 16),(1950 + 10),   /* [sensor ] */
      2608      , 1960      ,   /* [bns    ] */
      1936      , 1936      ,   /* [bcrop  ] */
      1088      , 1088      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    }
};

static int S5K4E6_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE)
#else
    { 2560, 1440, SIZE_RATIO_16_9},
    { 1920, 1440, SIZE_RATIO_4_3},
#endif
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1088, 1088, SIZE_RATIO_1_1},
    { 1072, 1072, SIZE_RATIO_1_1},
#endif
    { 1280,  720, SIZE_RATIO_16_9},
    { 1056,  704, SIZE_RATIO_3_2},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9}, /* for CAMERA2_API_SUPPORT */
    {  736,  736, SIZE_RATIO_1_1},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  256,  144, SIZE_RATIO_16_9}, /* for CAMERA2_API_SUPPORT */
    {  176,  144, SIZE_RATIO_11_9},
};

static int S5K4E6_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE)
    { 2560, 1440, SIZE_RATIO_16_9},
#endif
#if !(defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080))
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
#endif
    { 2240, 1680, SIZE_RATIO_4_3},  /* For Easy 360 */
    { 1600, 1200, SIZE_RATIO_4_3},
    { 1280,  960, SIZE_RATIO_4_3},
    { 1056,  864, SIZE_RATIO_11_9},
    {  960,  960, SIZE_RATIO_1_1},  /* for Clip Movie */
    {  640,  360, SIZE_RATIO_16_9},  /* for SWIS */
    {  720,  720, SIZE_RATIO_1_1},
    {  528,  432, SIZE_RATIO_11_9},
    {  800,  480, SIZE_RATIO_5_3},
    {  672,  448, SIZE_RATIO_3_2},
    {  480,  320, SIZE_RATIO_3_2},
    {  480,  270, SIZE_RATIO_16_9},
};

static int S5K4E6_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2592, 1458, SIZE_RATIO_16_9},
    { 2592, 1944, SIZE_RATIO_4_3},
    { 2560, 1440, SIZE_RATIO_16_9},
    { 2048, 1536, SIZE_RATIO_4_3},
    { 1936, 1936, SIZE_RATIO_1_1},
    { 1920, 1440, SIZE_RATIO_4_3},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1440, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  256,  144, SIZE_RATIO_16_9},
};

static int S5K4E6_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1600, 1200, SIZE_RATIO_4_3},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1280,  960, SIZE_RATIO_4_3},
    { 1024,  768, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
    {  800,  600, SIZE_RATIO_4_3},
    {  800,  480, SIZE_RATIO_5_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  720, SIZE_RATIO_1_1},
    {  720,  480, SIZE_RATIO_3_2},
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  480,  320, SIZE_RATIO_3_2},
    {  352,  288, SIZE_RATIO_11_9}, /* dummy size for binning mode */
    {  320,  240, SIZE_RATIO_4_3},
    {  320,  180, SIZE_RATIO_16_9},
};

static int S5K4E6_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
    {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int S5K4E6_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1440, SIZE_RATIO_1_1},
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
    {  176,  144, SIZE_RATIO_11_9}
};

static int S5K4E6_HIDDEN_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
#ifdef USE_WQHD_RECORDING
    { 2560, 1440, SIZE_RATIO_16_9},
#endif
    {  960,  960, SIZE_RATIO_1_1},  /* for Clip Movie */
    {  864,  480, SIZE_RATIO_16_9}, /* for PLB mode */
    {  432,  240, SIZE_RATIO_16_9}, /* for PLB mode */
};

static int S5K4E6_FPS_RANGE_LIST[][2] =
{
//    {   5000,   5000},
//    {   7000,   7000},
    {  15000,  15000},
    {  24000,  24000},
//    {   4000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int S5K4E6_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  10000,  24000},
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};

/* For HAL3 */
static int S5K4E6_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2592, 1944, SIZE_RATIO_4_3},
    { 2592, 1458, SIZE_RATIO_16_9},
    { 2560, 1440, SIZE_RATIO_16_9},
    { 2048, 1536, SIZE_RATIO_4_3},
    { 1936, 1936, SIZE_RATIO_1_1},
    { 1920, 1440, SIZE_RATIO_4_3},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1440, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  256,  144, SIZE_RATIO_16_9},
    {  176,  144, SIZE_RATIO_11_9},
};

/* availble Jpeg size (only for  HAL_PIXEL_FORMAT_BLOB) */
static int S5K4E6_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2592, 1944, SIZE_RATIO_4_3},
    { 2592, 1458, SIZE_RATIO_16_9},
    { 2560, 1440, SIZE_RATIO_16_9},
    { 2048, 1536, SIZE_RATIO_4_3},
    { 1936, 1936, SIZE_RATIO_1_1},
    { 1920, 1440, SIZE_RATIO_4_3},
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1440, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
};

static camera_metadata_rational UNIT_MATRIX_4E6_3X3[] =
{
    {128, 128}, {0, 128}, {0, 128},
    {0, 128}, {128, 128}, {0, 128},
    {0, 128}, {0, 128}, {128, 128}
};
#endif
