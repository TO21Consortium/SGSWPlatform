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

static int PREVIEW_SIZE_LUT_3P3_FULL_OTF[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(2592 + 10),//(5312 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 2602      ,//5328      , 3000      ,   /* [bns    ] */
      4608      , 2592      ,//5312      , 2988      ,   /* [bcrop  ] */
      4608      , 2592      ,//5312      , 2988      ,   /* [bds    ] */
      1920      , 1080      ,//1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),//(3984 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 3466      ,//4000      , 3000      ,   /* [bns    ] */
      4608      , 3456      ,//3984      , 2988      ,   /* [bcrop  ] */
      4608      , 3456      ,//3984      , 2988      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),//(3984 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 3466      ,//4000      , 3000      ,   /* [bns    ] */
      3456      , 3456      ,//2988      , 2988      ,   /* [bcrop  ] */
      3456      , 3456      ,//2988      , 2988      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4128      , 2752      ,   /* [bcrop  ] */
      4128      , 2752      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3872      , 3096      ,   /* [bcrop  ] */
      3872      , 3096      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4128      , 2476      ,   /* [bcrop  ] */
      4128      , 2476      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3784      , 3096      ,   /* [bcrop  ] */
      3784      , 3096      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_3P3_FULL_OTF_HAL3[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 16),(3456 + 10),//(5312 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 3466      ,//5328      , 3000      ,   /* [bns    ] */
      4608      , 2592      ,//5312      , 2988      ,   /* [bcrop  ] */
      4608      , 2592      ,//5312      , 2988      ,   /* [bds    ] */
      1920      , 1080      ,//1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),//(3984 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 3466      ,//4000      , 3000      ,   /* [bns    ] */
      4608      , 3456      ,//3984      , 2988      ,   /* [bcrop  ] */
      4608      , 3456      ,//3984      , 2988      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),//(3984 + 16),(2988 + 12),   /* [sensor ] */
      4624      , 3466      ,//4000      , 3000      ,   /* [bns    ] */
      3456      , 3456      ,//2988      , 2988      ,   /* [bcrop  ] */
      3456      , 3456      ,//2988      , 2988      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4128      , 2752      ,   /* [bcrop  ] */
      4128      , 2752      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3872      , 3096      ,   /* [bcrop  ] */
      3872      , 3096      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4128      , 2476      ,   /* [bcrop  ] */
      4128      , 2476      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3784      , 3096      ,   /* [bcrop  ] */
      3784      , 3096      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
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
      4128      , 2752      ,   /* [bcrop  ] */
      4128      , 2752      ,   /* [bds    ] */
      4128      , 2752      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3872      , 3096      ,   /* [bcrop  ] */
      3872      , 3096      ,   /* [bds    ] */
      3872      , 3096      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4128      , 2476      ,   /* [bcrop  ] */
      4128      , 2476      ,   /* [bds    ] */
      4128      , 2476      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3784      , 3096      ,   /* [bcrop  ] */
      3784      , 3096      ,   /* [bds    ] */
      3776      , 3096      ,   /* [target ] */
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
      2000      , 1500      ,   /* [bns    ] */
      1822      , 1490      ,   /* [bcrop  ] */
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
      4608      , 2592      ,//5312      , 2988      ,   /* [bcrop  ] */
      4608      , 2592      ,//5312      , 2988      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      640      , 480      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4626      , 3466      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      3456      , 3456      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_3_2,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4128      , 2752      ,   /* [bcrop  ] */
      4128      , 2752      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3872      , 3096      ,   /* [bcrop  ] */
      3872      , 3096      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4128      , 2476      ,   /* [bcrop  ] */
      4128      , 2476      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3784      , 3096      ,   /* [bcrop  ] */
      3784      , 3096      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3P3_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*  FHD_60  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2296 + 16),(1290 + 10),   /* [sensor ] *//* Sensor binning ratio = 2 */
      2312      , 1300      ,   /* [bns    ] */
      2296      , 1290      ,   /* [bcrop  ] */
      2296      , 1290      ,   /* [bds    ] */
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
     (1136 + 16),( 638 + 10),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1152      ,  648      ,   /* [bns    ] */
      1136      ,  638      ,   /* [bcrop  ] */
      1136      ,  638      ,   /* [bds    ] */
      1136      ,  638      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_HIGH_SPEED_3P3[][SIZE_OF_LUT] =
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
     (1264 + 16),( 708 + 12),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1280      ,  720      ,   /* [bns    ] */
      1280      ,  720      ,   /* [bcrop  ] */
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

static int YUV_SIZE_LUT_3P3[][SIZE_OF_LUT] =
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
      4128      , 2752      ,   /* [bcrop  ] */
      4128      , 2752      ,   /* [bds    ] */
      4128      , 2752      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3872      , 3096      ,   /* [bcrop  ] */
      3872      , 3096      ,   /* [bds    ] */
      3872      , 3096      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      4128      , 2476      ,   /* [bcrop  ] */
      4128      , 2476      ,   /* [bds    ] */
      4128      , 2476      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 16),(3456 + 10),   /* [sensor ] */
      4624      , 3466      ,   /* [bns    ] */
      3784      , 3096      ,   /* [bcrop  ] */
      3784      , 3096      ,   /* [bds    ] */
      3776      , 3096      ,   /* [target ] */
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
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  256,  144, SIZE_RATIO_16_9},
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
    { 1136,  638, SIZE_RATIO_16_9},
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

static int S5K3P3_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1280,  720, SIZE_RATIO_16_9},
};

static int S5K3P3_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000, 120000},
    { 120000, 120000},
};

static int S5K3P3_FPS_RANGE_LIST[][2] =
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

static int S5K3P3_HIDDEN_FPS_RANGE_LIST[][2] =
{

    {  10000,  24000},
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
};

static camera_metadata_rational UNIT_MATRIX_3P3_3X3[] =
{
    {128, 128}, {0, 128}, {0, 128},
    {0, 128}, {128, 128}, {0, 128},
    {0, 128}, {0, 128}, {128, 128}
};

static camera_metadata_rational COLOR_MATRIX1_3P3_3X3[] = {
    {1094, 1024}, {-306, 1024}, {-146, 1024},
    {-442, 1024}, {1388, 1024}, {52, 1024},
    {-104, 1024}, {250, 1024}, {600, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_3P3_3X3[] = {
    {2263, 1024}, {-1364, 1024}, {-145, 1024},
    {-194, 1024}, {1257, 1024}, {-56, 1024},
    {-24, 1024}, {187, 1024}, {618, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_3P3_3X3[] = {
    {612, 1024}, {233, 1024}, {139, 1024},
    {199, 1024}, {831, 1024}, {-6, 1024},
    {15, 1024}, {-224, 1024}, {1049, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_3P3_3X3[] = {
    {441, 1024}, {317, 1024}, {226, 1024},
    {29, 1024}, {908, 1024}, {87, 1024},
    {9, 1024}, {-655, 1024}, {1486, 1024}
};

#endif
