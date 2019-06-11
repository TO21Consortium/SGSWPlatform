/*
 * Copyright 2012, Samsung Electronics Co. LTD
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

#include "fimg2d_board.h"

static int boost_size[][2] = { {768, 924}, {1152, 1436}, {1536,1948} };

int FimgApiCheckBoostup(Fimg *fimg, Fimg *prev_fimg)
{
    bool found = false;
    int i;

    for (i = 0; i < sizeof(boost_size) / sizeof(boost_size[0]); i++) {
        if ((fimg->srcW == boost_size[i][0]) && (fimg->srcH == boost_size[i][1])) {
            found = true;
            fimg->level = 0;
            break;
        }
    }

    if (!found)
        return false;

    if (fimg->alpha != 0xff)
        return false;

    if ((fimg->srcAddr != prev_fimg->srcAddr) ||
        (fimg->srcX != prev_fimg->srcX) ||
        (fimg->srcY != prev_fimg->srcY) ||
        (fimg->srcW != prev_fimg->srcW) ||
        (fimg->srcH != prev_fimg->srcH) ||
        (fimg->srcFWStride != prev_fimg->srcFWStride) ||
        (fimg->srcFH != prev_fimg->srcFH) ||
        (fimg->srcColorFormat != prev_fimg->srcColorFormat))
        return false;

    if ((fimg->dstAddr != prev_fimg->dstAddr) ||
        (fimg->dstX != prev_fimg->dstX) ||
        (fimg->dstY != prev_fimg->dstY) ||
        (fimg->dstW != prev_fimg->dstW) ||
        (fimg->dstH != prev_fimg->dstH) ||
        (fimg->dstFWStride != prev_fimg->dstFWStride) ||
        (fimg->dstFH != prev_fimg->dstFH) ||
        (fimg->dstColorFormat != prev_fimg->dstColorFormat))
        return false;

    if ((fimg->clipT != prev_fimg->clipT) ||
        (fimg->clipB != prev_fimg->clipB) ||
        (fimg->clipL != prev_fimg->clipL) ||
        (fimg->clipR != prev_fimg->clipR))
        return false;

    if ((fimg->fillcolor != prev_fimg->fillcolor) ||
        (fimg->rotate != prev_fimg->rotate) ||
        (fimg->alpha != prev_fimg->alpha) ||
        (fimg->xfermode != prev_fimg->xfermode) ||
        (fimg->isDither != prev_fimg->isDither) ||
        (fimg->isFilter != prev_fimg->isFilter) ||
        (fimg->matrixType != prev_fimg->matrixType))
        return false;
    return true;
}
