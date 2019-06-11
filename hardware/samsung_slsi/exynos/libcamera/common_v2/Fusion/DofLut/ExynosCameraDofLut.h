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

#ifndef EXYNOS_CAMERA_DOF_LUT_H
#define EXYNOS_CAMERA_DOF_LUT_H

#include "ExynosCameraFusionInclude.h"

using namespace android;

#define DEFAULT_DISTANCE_FAR (100000.0f) /* 100m == 10000cm == 100000mm */
#define DEFAULT_DISTANCE_NEAR     (0.0f)

struct DOF_LUT {
    int     distance;  // mm
    float   lensShift; // mm
    float   farField;  // mm
    float   nearField; // mm

    DOF_LUT(int   distance,
            float lensShift,
            float farField,
            float nearField) :
            distance (distance),
            lensShift(lensShift),
            farField (farField),
            nearField(nearField)
    {}

    DOF_LUT() {
        distance  = 0;
        lensShift = 0.0f;
        farField  = 0.0f;
        nearField = 0.0f;
    };
};

struct DOF {
    const DOF_LUT *lut;
    int            lutCnt;

    float          lensShiftOn12M; // mm
    float          lensShiftOn01M; // mm

    DOF()
    {
        lut = NULL;
        lutCnt = 0;

        lensShiftOn12M = 0.0f;
        lensShiftOn01M = 0.0f;
    };
};

#endif // EXYNOS_CAMERA_DOF_LUT_H
