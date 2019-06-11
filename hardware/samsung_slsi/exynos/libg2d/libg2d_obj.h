 /*
 * Copyright (C) 2013 The Android Open Source Project
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      libg2d_obj.h
 * \brief     header file for G2D library
 * \author    Eunseok Choi (es10.choi@samsung.com)
 * \date      2013/09/21
 *
 * <b>Revision History: </b>
 * - 2013.09.21 : Eunseok Choi (eunseok.choi@samsung.com) \n
 *   Create
 *
 */
#ifndef __LIBG2D_H__
#define __LIBG2D_H__

#include "exynos_blender.h"
#include "exynos_blender_obj.h"

#define G2D_DEV_NODE    "/dev/video"
#define G2D_NODE(x)     (55 + x)

class CBlender;

class CFimg2d : public CBlender {
public:
    enum { G2D_NUM_OF_PLANES = 2 };

    CFimg2d(BL_DEVID devid, bool nonblock = false);
    ~CFimg2d();

    int DoStart();
    int DoStop();
    int Deactivate(bool deact);

private:
    void Initialize(BL_DEVID devid, bool nonblock);
    void ResetPort(BL_PORT port);
    void DebugParam();

    int SetCtrl();
    int SetFormat();
    int ReqBufs();
    int QBuf();
    int DQBuf();
    int DQBuf(BL_PORT port);
    int StreamOn();
};

#endif // __LIBG2D_H__
