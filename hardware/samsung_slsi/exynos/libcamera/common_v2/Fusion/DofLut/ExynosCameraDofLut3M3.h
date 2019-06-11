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

#ifndef EXYNOS_CAMERA_DOF_LUT_3M3_H
#define EXYNOS_CAMERA_DOF_LUT_3M3_H

#include "ExynosCameraFusionInclude.h"

const DOF_LUT DOF_LUT_3M3[] =
{
    DOF_LUT(10000,  0.001,  DEFAULT_DISTANCE_FAR, 2302),
    DOF_LUT( 5000,  0.003,  DEFAULT_DISTANCE_FAR, 1872),
    DOF_LUT( 4000,  0.003,  DEFAULT_DISTANCE_FAR, 1712),
    DOF_LUT( 3000,  0.004,  DEFAULT_DISTANCE_FAR, 1499),
    DOF_LUT( 2000,  0.007,  6031,                 1200),
    DOF_LUT( 1900,  0.007,  5203,                 1163),
    DOF_LUT( 1800,  0.007,  4515,                 1125),
    DOF_LUT( 1700,  0.008,  3933,                 1085),
    DOF_LUT( 1600,  0.008,  3435,                 1044),
    DOF_LUT( 1500,  0.009,  3004,                 1000),
    DOF_LUT( 1400,  0.010,  2627,                  955),
    DOF_LUT( 1300,  0.010,  2295,                  907),
    DOF_LUT( 1200,  0.011,  2000,                  858),
    DOF_LUT( 1100,  0.012,  1736,                  805),
    DOF_LUT( 1000,  0.013,  1499,                  751),
    DOF_LUT(  900,  0.015,  1285,                  693),
    DOF_LUT(  800,  0.017,  1090,                  632),
    DOF_LUT(  700,  0.019,   912,                  568),
    DOF_LUT(  600,  0.022,   749,                  501),
    DOF_LUT(  500,  0.027,   599,                  429),
    DOF_LUT(  450,  0.030,   528,                  392),
    DOF_LUT(  400,  0.034,   461,                  354),
    DOF_LUT(  350,  0.039,   395,                  314),
    DOF_LUT(  300,  0.045,   333,                  273),
    DOF_LUT(  250,  0.054,   272,                  231),
    DOF_LUT(  200,  0.068,   214,                  188),
    DOF_LUT(  150,  0.091,   158,                  143),
    DOF_LUT(  140,  0.098,   147,                  134),
    DOF_LUT(  130,  0.106,   136,                  125),
    DOF_LUT(  120,  0.115,   125,                  116),
    DOF_LUT(  110,  0.126,   114,                  106),
    DOF_LUT(  100,  0.139,   103,                   97),
    DOF_LUT(   90,  0.155,    93,                   88),
    DOF_LUT(   80,  0.175,    82,                   78),
    DOF_LUT(   70,  0.202,    72,                   69),
    DOF_LUT(   60,  0.237,    61,                   59),
    DOF_LUT(   50,  0.289,    51,                   49),
};

struct DOF_3M3 : public DOF
{
    DOF_3M3() {
        lut = DOF_LUT_3M3;
        lutCnt = sizeof(DOF_LUT_3M3) / sizeof(DOF_LUT);

        lensShiftOn12M = 0.001f;
        lensShiftOn01M = 0.034f; // this is 0.4M's value.
    }
};

#endif // EXYNOS_CAMERA_DOF_LUT_3M3_H
