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

#ifndef EXYNOS_CAMERA_DOF_LUT_4H8_H
#define EXYNOS_CAMERA_DOF_LUT_4H8_H

#include "ExynosCameraFusionInclude.h"

const DOF_LUT DOF_LUT_4H8[] =
{
    DOF_LUT(10000,  0.003,  DEFAULT_DISTANCE_FAR, 3586),
    DOF_LUT( 5000,  0.007,  47686,                2641),
    DOF_LUT( 4000,  0.008,  14070,                2333),
    DOF_LUT( 3000,  0.011,   6469,                1954),
    DOF_LUT( 2000,  0.016,   3110,                1475),
    DOF_LUT( 1900,  0.017,   2874,                1420),
    DOF_LUT( 1800,  0.018,   2651,                1363),
    DOF_LUT( 1700,  0.019,   2439,                1305),
    DOF_LUT( 1600,  0.020,   2238,                1246),
    DOF_LUT( 1500,  0.022,   2047,                1184),
    DOF_LUT( 1400,  0.023,   1865,                1121),
    DOF_LUT( 1300,  0.025,   1691,                1056),
    DOF_LUT( 1200,  0.027,   1525,                 989),
    DOF_LUT( 1100,  0.030,   1367,                 921),
    DOF_LUT( 1000,  0.033,   1216,                 850),
    DOF_LUT(  900,  0.036,   1071,                 776),
    DOF_LUT(  800,  0.041,    932,                 701),
    DOF_LUT(  700,  0.047,    799,                 623),
    DOF_LUT(  600,  0.055,    671,                 543),
    DOF_LUT(  500,  0.066,    548,                 460),
    DOF_LUT(  450,  0.073,    488,                 417),
    DOF_LUT(  400,  0.082,    430,                 374),
    DOF_LUT(  350,  0.094,    373,                 330),
    DOF_LUT(  300,  0.110,    316,                 285),
    DOF_LUT(  250,  0.133,    261,                 240),
    DOF_LUT(  200,  0.167,    207,                 193),
    DOF_LUT(  150,  0.225,    154,                 146),
    DOF_LUT(  140,  0.242,    143,                 137),
    DOF_LUT(  130,  0.261,    133,                 127),
    DOF_LUT(  120,  0.284,    122,                 118),
    DOF_LUT(  110,  0.312,    112,                 108),
    DOF_LUT(  100,  0.345,    102,                  98),
    DOF_LUT(   90,  0.385,     91,                  89),
    DOF_LUT(   80,  0.437,     81,                  79),
    DOF_LUT(   70,  0.505,     71,                  69),
    DOF_LUT(   60,  0.598,     61,                  59),
    DOF_LUT(   50,  0.733,     50,                  50),
};

struct DOF_4H8 : public DOF
{
    DOF_4H8() {
        lut = DOF_LUT_4H8;
        lutCnt = sizeof(DOF_LUT_4H8) / sizeof(DOF_LUT);

        lensShiftOn12M = 0.003f;
        lensShiftOn01M = 0.082f; // this is 0.4M's value.
    }
};

#endif // EXYNOS_CAMERA_DOF_LUT_4H8_H
