/*
**
** Copyright 2014, Samsung Electronics Co. LTD
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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraUtilsModule"
#include <cutils/log.h>

#include "ExynosCameraUtilsModule.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// HACK
//////////////
#define isOwnScc(cameraId) ((cameraId == CAMERA_ID_BACK) ? MAIN_CAMERA_HAS_OWN_SCC : FRONT_CAMERA_HAS_OWN_SCC)

namespace android {

void updateNodeGroupInfoMainPreview(
        int cameraId,
        camera2_node_group *node_group_info_3aa,
        camera2_node_group *node_group_info_isp,
        ExynosRect bayerCropSize,
        __unused ExynosRect bdsSize,
        int previewW, int previewH,
        __unused int pictureW, __unused int pictureH)
{
    ALOGV("Leader before (%d, %d, %d, %d)(%d, %d, %d, %d)(%d %d)",
        node_group_info_3aa->leader.input.cropRegion[0],
        node_group_info_3aa->leader.input.cropRegion[1],
        node_group_info_3aa->leader.input.cropRegion[2],
        node_group_info_3aa->leader.input.cropRegion[3],
        node_group_info_3aa->leader.output.cropRegion[0],
        node_group_info_3aa->leader.output.cropRegion[1],
        node_group_info_3aa->leader.output.cropRegion[2],
        node_group_info_3aa->leader.output.cropRegion[3],
        node_group_info_3aa->leader.request,
        node_group_info_3aa->leader.vid);

    /* Leader : 3AA : BCrop */
    node_group_info_3aa->leader.input.cropRegion[0] = bayerCropSize.x;
    node_group_info_3aa->leader.input.cropRegion[1] = bayerCropSize.y;
    node_group_info_3aa->leader.input.cropRegion[2] = bayerCropSize.w;
    node_group_info_3aa->leader.input.cropRegion[3] = bayerCropSize.h;
    node_group_info_3aa->leader.output.cropRegion[0] = node_group_info_3aa->leader.input.cropRegion[0];
    node_group_info_3aa->leader.output.cropRegion[1] = node_group_info_3aa->leader.input.cropRegion[1];
    node_group_info_3aa->leader.output.cropRegion[2] = node_group_info_3aa->leader.input.cropRegion[2];
    node_group_info_3aa->leader.output.cropRegion[3] = node_group_info_3aa->leader.input.cropRegion[3];

    /* Capture 0 : 3AC -[X] - output cropX, cropY should be Zero */
    node_group_info_3aa->capture[PERFRAME_BACK_3AC_POS].input.cropRegion[0] = 0;
    node_group_info_3aa->capture[PERFRAME_BACK_3AC_POS].input.cropRegion[1] = 0;
    node_group_info_3aa->capture[PERFRAME_BACK_3AC_POS].input.cropRegion[2] = node_group_info_3aa->leader.input.cropRegion[2];
    node_group_info_3aa->capture[PERFRAME_BACK_3AC_POS].input.cropRegion[3] = node_group_info_3aa->leader.input.cropRegion[3];
    node_group_info_3aa->capture[PERFRAME_BACK_3AC_POS].output.cropRegion[0] = 0;
    node_group_info_3aa->capture[PERFRAME_BACK_3AC_POS].output.cropRegion[1] = 0;
    node_group_info_3aa->capture[PERFRAME_BACK_3AC_POS].output.cropRegion[2] = node_group_info_3aa->leader.input.cropRegion[2];
    node_group_info_3aa->capture[PERFRAME_BACK_3AC_POS].output.cropRegion[3] = node_group_info_3aa->leader.input.cropRegion[3];

    /* Capture 1 : 3AP - [BDS] */
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].input.cropRegion[0] = 0;
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].input.cropRegion[1] = 0;
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].input.cropRegion[2] = bayerCropSize.w;
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].input.cropRegion[3] = bayerCropSize.h;
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].output.cropRegion[0] = 0;
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].output.cropRegion[1] = 0;
#if (defined(CAMERA_HAS_OWN_BDS) && (CAMERA_HAS_OWN_BDS))
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].output.cropRegion[2] = (bayerCropSize.w < bdsSize.w) ? bayerCropSize.w : bdsSize.w;
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].output.cropRegion[3] = (bayerCropSize.h < bdsSize.h) ? bayerCropSize.h : bdsSize.h;
#else
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].output.cropRegion[2] = bayerCropSize.w;
    node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].output.cropRegion[3] = bayerCropSize.h;
#endif
    /* Leader : ISP */
    node_group_info_isp->leader.input.cropRegion[0] = 0;
    node_group_info_isp->leader.input.cropRegion[1] = 0;
    node_group_info_isp->leader.input.cropRegion[2] = node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].output.cropRegion[2];
    node_group_info_isp->leader.input.cropRegion[3] = node_group_info_3aa->capture[PERFRAME_BACK_3AP_POS].output.cropRegion[3];
    node_group_info_isp->leader.output.cropRegion[0] = 0;
    node_group_info_isp->leader.output.cropRegion[1] = 0;
    node_group_info_isp->leader.output.cropRegion[2] = node_group_info_isp->leader.input.cropRegion[2];
    node_group_info_isp->leader.output.cropRegion[3] = node_group_info_isp->leader.input.cropRegion[3];

    /* Capture : ISPP */
    node_group_info_isp->capture[PERFRAME_BACK_ISPP_POS].input.cropRegion[0] = node_group_info_isp->leader.output.cropRegion[0];
    node_group_info_isp->capture[PERFRAME_BACK_ISPP_POS].input.cropRegion[1] = node_group_info_isp->leader.output.cropRegion[1];
    node_group_info_isp->capture[PERFRAME_BACK_ISPP_POS].input.cropRegion[2] = node_group_info_isp->leader.output.cropRegion[2];
    node_group_info_isp->capture[PERFRAME_BACK_ISPP_POS].input.cropRegion[3] = node_group_info_isp->leader.output.cropRegion[3];

    node_group_info_isp->capture[PERFRAME_BACK_ISPP_POS].output.cropRegion[0] = node_group_info_isp->leader.output.cropRegion[0];
    node_group_info_isp->capture[PERFRAME_BACK_ISPP_POS].output.cropRegion[1] = node_group_info_isp->leader.output.cropRegion[1];
    node_group_info_isp->capture[PERFRAME_BACK_ISPP_POS].output.cropRegion[2] = node_group_info_isp->leader.output.cropRegion[2];
    node_group_info_isp->capture[PERFRAME_BACK_ISPP_POS].output.cropRegion[3] = node_group_info_isp->leader.output.cropRegion[3];

    /* Capture 0 : SCP - [scaling] */
    if (isOwnScc(cameraId) == true) {
        /* HACK: When Driver do not support SCP scaling */
        node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[0] = 0;
        node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[1] = 0;
        node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[2] = previewW;
        node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[3] = previewH;
    } else {
        node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[0] = 0;
        node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[1] = 0;
        node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[2] = node_group_info_isp->leader.output.cropRegion[2];
        node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[3] = node_group_info_isp->leader.output.cropRegion[3];
    }
    node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].output.cropRegion[0] = 0;
    node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].output.cropRegion[1] = 0;
    node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].output.cropRegion[2] =
                         (node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[2] < (unsigned)previewW ? node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[2] : previewW);
    node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].output.cropRegion[3] =
                         (node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[3] < (unsigned)previewH ? node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[3] : previewH);

    /*
     * HACK
     * in OTF case, we need to set perframe size on 3AA.
     * This set 3aa perframe size by isp perframe size.
     * The reason of hack is that worry about modify and code sync.
     */
    for (int i = 0; i < 4; i++) {
        node_group_info_3aa->capture[PERFRAME_BACK_SCP_POS].input. cropRegion[i] = node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].input. cropRegion[i];
        node_group_info_3aa->capture[PERFRAME_BACK_SCP_POS].output.cropRegion[i] = node_group_info_isp->capture[PERFRAME_BACK_SCP_POS].output.cropRegion[i];
    }

    ALOGV("Leader after (%d, %d, %d, %d)(%d, %d, %d, %d)(%d %d)",
        node_group_info_3aa->leader.input.cropRegion[0],
        node_group_info_3aa->leader.input.cropRegion[1],
        node_group_info_3aa->leader.input.cropRegion[2],
        node_group_info_3aa->leader.input.cropRegion[3],
        node_group_info_3aa->leader.output.cropRegion[0],
        node_group_info_3aa->leader.output.cropRegion[1],
        node_group_info_3aa->leader.output.cropRegion[2],
        node_group_info_3aa->leader.output.cropRegion[3],
        node_group_info_3aa->leader.request,
        node_group_info_3aa->leader.vid);
}


void updateNodeGroupInfoReprocessing(
        int cameraId,
        camera2_node_group *node_group_info_3aa,
        camera2_node_group *node_group_info_isp,
        ExynosRect bayerCropSizePreview,
        ExynosRect bayerCropSizePicture,
        ExynosRect bdsSize,
        int pictureW, int pictureH,
        bool pureBayerReprocessing,
        bool flag3aaIspOtf)
{
        int perFramePos = 0;
    ALOGV("Leader before (%d, %d, %d, %d)(%d, %d, %d, %d)(%d %d)",
        node_group_info_3aa->leader.input.cropRegion[0],
        node_group_info_3aa->leader.input.cropRegion[1],
        node_group_info_3aa->leader.input.cropRegion[2],
        node_group_info_3aa->leader.input.cropRegion[3],
        node_group_info_3aa->leader.output.cropRegion[0],
        node_group_info_3aa->leader.output.cropRegion[1],
        node_group_info_3aa->leader.output.cropRegion[2],
        node_group_info_3aa->leader.output.cropRegion[3],
        node_group_info_3aa->leader.request,
        node_group_info_3aa->leader.vid);

    if (pureBayerReprocessing == true) {
        /* Leader : 3AA */
        node_group_info_3aa->leader.input.cropRegion[0] = bayerCropSizePicture.x;
        node_group_info_3aa->leader.input.cropRegion[1] = bayerCropSizePicture.y;
        node_group_info_3aa->leader.input.cropRegion[2] = bayerCropSizePicture.w;
        node_group_info_3aa->leader.input.cropRegion[3] = bayerCropSizePicture.h;
        node_group_info_3aa->leader.output.cropRegion[0] = node_group_info_3aa->leader.input.cropRegion[0];
        node_group_info_3aa->leader.output.cropRegion[1] = node_group_info_3aa->leader.input.cropRegion[1];
        node_group_info_3aa->leader.output.cropRegion[2] = node_group_info_3aa->leader.input.cropRegion[2];
        node_group_info_3aa->leader.output.cropRegion[3] = node_group_info_3aa->leader.input.cropRegion[3];

        perFramePos = PERFRAME_REPROCESSING_3AP_POS;

        /* Capture 1 : 3AC - [BDS] */
        node_group_info_3aa->capture[perFramePos].input.cropRegion[0] = 0;
        node_group_info_3aa->capture[perFramePos].input.cropRegion[1] = 0;
        node_group_info_3aa->capture[perFramePos].input.cropRegion[2] = bayerCropSizePicture.w;
        node_group_info_3aa->capture[perFramePos].input.cropRegion[3] = bayerCropSizePicture.h;
        node_group_info_3aa->capture[perFramePos].output.cropRegion[0] = 0;
        node_group_info_3aa->capture[perFramePos].output.cropRegion[1] = 0;
        node_group_info_3aa->capture[perFramePos].output.cropRegion[2] = (bayerCropSizePicture.w < bdsSize.w) ? bayerCropSizePicture.w : bdsSize.w;
        node_group_info_3aa->capture[perFramePos].output.cropRegion[3] = (bayerCropSizePicture.h < bdsSize.h) ? bayerCropSizePicture.h : bdsSize.h;

        /* Leader : ISP */
        node_group_info_isp->leader.input.cropRegion[0] = 0;
        node_group_info_isp->leader.input.cropRegion[1] = 0;
        node_group_info_isp->leader.input.cropRegion[2] = node_group_info_3aa->capture[perFramePos].output.cropRegion[2];
        node_group_info_isp->leader.input.cropRegion[3] = node_group_info_3aa->capture[perFramePos].output.cropRegion[3];
        node_group_info_isp->leader.output.cropRegion[0] = 0;
        node_group_info_isp->leader.output.cropRegion[1] = 0;
        node_group_info_isp->leader.output.cropRegion[2] = node_group_info_isp->leader.input.cropRegion[2];
        node_group_info_isp->leader.output.cropRegion[3] = node_group_info_isp->leader.input.cropRegion[3];

        /* Capture 1 : SCC */
        node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[0] = node_group_info_isp->leader.output.cropRegion[0];
        node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[1] = node_group_info_isp->leader.output.cropRegion[1];
        node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2] = node_group_info_isp->leader.output.cropRegion[2];
        node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3] = node_group_info_isp->leader.output.cropRegion[3];

        if (isOwnScc(cameraId) == true) {
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[0] = 0;
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[1] = 0;
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[2] = pictureW;
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[3] = pictureH;
        } else {
            /* ISPC does not support scaling */
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[0] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[0];
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[1] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[1];
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[2] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2];
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[3] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3];
        }

        /*
         * HACK
         * in OTF case, we need to set perframe size on 3AA.
         * just set 3aa_isp otf size by isp perframe size.
         * The reason of hack is
         * worry about modify and code sync.
         */
        if (flag3aaIspOtf == true) {
            /* Capture 1 : ISPC */
            node_group_info_3aa->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[0] = node_group_info_isp->leader.output.cropRegion[0];
            node_group_info_3aa->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[1] = node_group_info_isp->leader.output.cropRegion[1];
            node_group_info_3aa->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2] = node_group_info_isp->leader.output.cropRegion[2];
            node_group_info_3aa->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3] = node_group_info_isp->leader.output.cropRegion[3];

            node_group_info_3aa->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[0] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[0];
            node_group_info_3aa->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[1] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[1];
            node_group_info_3aa->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[2] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[2];
            node_group_info_3aa->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[3] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[3];
        }
    } else {
        /* Leader : ISP */
        node_group_info_isp->leader.input.cropRegion[0] = 0;
        node_group_info_isp->leader.input.cropRegion[1] = 0;
        node_group_info_isp->leader.input.cropRegion[2] = bayerCropSizePreview.w;
        node_group_info_isp->leader.input.cropRegion[3] = bayerCropSizePreview.h;
        node_group_info_isp->leader.output.cropRegion[0] = 0;
        node_group_info_isp->leader.output.cropRegion[1] = 0;
        node_group_info_isp->leader.output.cropRegion[2] = node_group_info_isp->leader.input.cropRegion[2];
        node_group_info_isp->leader.output.cropRegion[3] = node_group_info_isp->leader.input.cropRegion[3];

        /* Capture 1 : SCC */
        node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[0] = (bayerCropSizePreview.w - bayerCropSizePicture.w) / 2;
        node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[1] = (bayerCropSizePreview.h - bayerCropSizePicture.h) / 2;
        node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2] = bayerCropSizePicture.w;
        node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3] = bayerCropSizePicture.h;

        if (isOwnScc(cameraId) == true) {
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[0] = 0;
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[1] = 0;
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[2] = pictureW;
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[3] = pictureH;
        } else {
            /* ISPC does not support scaling */
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[0] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[0];
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[1] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[1];
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[2] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2];
            node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[3] = node_group_info_isp->capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3];
        }
    }

    ALOGV("Leader after (%d, %d, %d, %d)(%d, %d, %d, %d)(%d %d)",
        node_group_info_3aa->leader.input.cropRegion[0],
        node_group_info_3aa->leader.input.cropRegion[1],
        node_group_info_3aa->leader.input.cropRegion[2],
        node_group_info_3aa->leader.input.cropRegion[3],
        node_group_info_3aa->leader.output.cropRegion[0],
        node_group_info_3aa->leader.output.cropRegion[1],
        node_group_info_3aa->leader.output.cropRegion[2],
        node_group_info_3aa->leader.output.cropRegion[3],
        node_group_info_3aa->leader.request,
        node_group_info_3aa->leader.vid);
}

void updateNodeGroupInfoFront(
        int cameraId,
        camera2_node_group *node_group_info_3aa,
        camera2_node_group *node_group_info_isp,
        ExynosRect bayerCropSize,
        __unused ExynosRect bdsSize,
        int previewW, int previewH,
        __unused int pictureW, __unused int pictureH)
{

    /* Leader : 3AA : BCrop */
    node_group_info_3aa->leader.input.cropRegion[0] = bayerCropSize.x;
    node_group_info_3aa->leader.input.cropRegion[1] = bayerCropSize.y;
    node_group_info_3aa->leader.input.cropRegion[2] = bayerCropSize.w;
    node_group_info_3aa->leader.input.cropRegion[3] = bayerCropSize.h;
    node_group_info_3aa->leader.output.cropRegion[0] = node_group_info_3aa->leader.input.cropRegion[0];
    node_group_info_3aa->leader.output.cropRegion[1] = node_group_info_3aa->leader.input.cropRegion[1];
    node_group_info_3aa->leader.output.cropRegion[2] = node_group_info_3aa->leader.input.cropRegion[2];
    node_group_info_3aa->leader.output.cropRegion[3] = node_group_info_3aa->leader.input.cropRegion[3];

    /* Capture 0 :3AP : BDS */
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].input.cropRegion[0] = 0;
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].input.cropRegion[1] = 0;
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].input.cropRegion[2] = node_group_info_3aa->leader.output.cropRegion[2];
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].input.cropRegion[3] = node_group_info_3aa->leader.output.cropRegion[3];
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].output.cropRegion[0] = 0;
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].output.cropRegion[1] = 0;
#if (defined(CAMERA_HAS_OWN_BDS) && (CAMERA_HAS_OWN_BDS))
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].output.cropRegion[2] = (bayerCropSize.w < bdsSize.w)? bayerCropSize.w : bdsSize.w;
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].output.cropRegion[3] = (bayerCropSize.h < bdsSize.h)? bayerCropSize.h : bdsSize.h;
#else
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].output.cropRegion[2] = bayerCropSize.w;
    node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].output.cropRegion[3] = bayerCropSize.h;
#endif

    /* Leader : ISP */
    node_group_info_isp->leader.input.cropRegion[0] = 0;
    node_group_info_isp->leader.input.cropRegion[1] = 0;
    node_group_info_isp->leader.input.cropRegion[2] = node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].output.cropRegion[2];
    node_group_info_isp->leader.input.cropRegion[3] = node_group_info_3aa->capture[PERFRAME_FRONT_3AP_POS].output.cropRegion[3];
    node_group_info_isp->leader.output.cropRegion[0] = 0;
    node_group_info_isp->leader.output.cropRegion[1] = 0;
    node_group_info_isp->leader.output.cropRegion[2] = node_group_info_isp->leader.input.cropRegion[2];
    node_group_info_isp->leader.output.cropRegion[3] = node_group_info_isp->leader.input.cropRegion[3];

    /* Capture : ISPP */
    node_group_info_isp->capture[PERFRAME_FRONT_ISPP_POS].input.cropRegion[0] = node_group_info_isp->leader.output.cropRegion[0];
    node_group_info_isp->capture[PERFRAME_FRONT_ISPP_POS].input.cropRegion[1] = node_group_info_isp->leader.output.cropRegion[1];
    node_group_info_isp->capture[PERFRAME_FRONT_ISPP_POS].input.cropRegion[2] = node_group_info_isp->leader.output.cropRegion[2];
    node_group_info_isp->capture[PERFRAME_FRONT_ISPP_POS].input.cropRegion[3] = node_group_info_isp->leader.output.cropRegion[3];

    node_group_info_isp->capture[PERFRAME_FRONT_ISPP_POS].output.cropRegion[0] = node_group_info_isp->leader.output.cropRegion[0];
    node_group_info_isp->capture[PERFRAME_FRONT_ISPP_POS].output.cropRegion[1] = node_group_info_isp->leader.output.cropRegion[1];
    node_group_info_isp->capture[PERFRAME_FRONT_ISPP_POS].output.cropRegion[2] = node_group_info_isp->leader.output.cropRegion[2];
    node_group_info_isp->capture[PERFRAME_FRONT_ISPP_POS].output.cropRegion[3] = node_group_info_isp->leader.output.cropRegion[3];

    /* Capture 0 : SCC */
    node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].input.cropRegion[0] = node_group_info_isp->leader.output.cropRegion[0];
    node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].input.cropRegion[1] = node_group_info_isp->leader.output.cropRegion[1];
    node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].input.cropRegion[2] = node_group_info_isp->leader.output.cropRegion[2];
    node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].input.cropRegion[3] = node_group_info_isp->leader.output.cropRegion[3];

    if (isOwnScc(cameraId) == true) {
        node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[0] = 0;
        node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[1] = 0;
        node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[2] = previewW;
        node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[3] = previewH;
    } else {
        /* ISPC does not support scaling */
        node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[0] = node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].input.cropRegion[0];
        node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[1] = node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].input.cropRegion[1];
        node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[2] = node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].input.cropRegion[2];
        node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[3] = node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].input.cropRegion[3];
    }

    /* Capture 1 : SCP */
    node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[0] = node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[0];
    node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[1] = node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[1];
    node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[2] = node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[2];
    node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[3] = node_group_info_isp->capture[PERFRAME_FRONT_SCC_POS].output.cropRegion[3];

    if (isOwnScc(cameraId) == true) {
        /* HACK: When Driver do not support SCP scaling */
        node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[0] = node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[0];
        node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[1] = node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[1];
        node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[2] = node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[2];
        node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[3] = node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[3];
    } else {
        node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[0] = 0;
        node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[1] = 0;
        node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[2] =
                            (node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[2] < (unsigned)previewW ? node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[2] : previewW);
        node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[3] =
                            (node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[3] < (unsigned)previewH ? node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input.cropRegion[3] : previewH);
    }

    /*
     * HACK
     * in OTF case, we need to set perframe size on 3AA
     * just set 3aa perframe size by isp perframe size.
     * The reason of hack is
     * worry about modify and code sync.
     */
    for (int i = 0; i < 4; i++) {
        node_group_info_3aa->capture[PERFRAME_FRONT_SCP_POS].input. cropRegion[i] = node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].input. cropRegion[i];
        node_group_info_3aa->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[i] = node_group_info_isp->capture[PERFRAME_FRONT_SCP_POS].output.cropRegion[i];
    }
}

void ExynosCameraNodeGroup::updateNodeGroupInfo(
        int cameraId,
        camera2_node_group *node_group_info_3aa,
        camera2_node_group *node_group_info_isp,
        ExynosRect bayerCropSize,
        ExynosRect bdsSize,
        int previewW, int previewH,
        int pictureW, int pictureH)
{
    if (cameraId == CAMERA_ID_BACK) {
        updateNodeGroupInfoMainPreview(
            cameraId,
            node_group_info_3aa,
            node_group_info_isp,
            bayerCropSize,
            bdsSize,
            previewW, previewH,
            pictureW, pictureH);
    } else {
        updateNodeGroupInfoFront(
            cameraId,
            node_group_info_3aa,
            node_group_info_isp,
            bayerCropSize,
            bdsSize,
            previewW, previewH,
            pictureW, pictureH);
    }

    // m_dump("3AA", cameraId, node_group_info_3aa);
    // m_dump("ISP", cameraId, node_group_info_isp);
}

void ExynosCameraNodeGroup::updateNodeGroupInfo(
        int cameraId,
        camera2_node_group *node_group_info_3aa,
        camera2_node_group *node_group_info_isp,
        ExynosRect bayerCropSizePreview,
        ExynosRect bayerCropSizePicture,
        ExynosRect bdsSize,
        int pictureW, int pictureH,
        bool pureBayerReprocessing,
        bool flag3aaIspOtf)
{
    updateNodeGroupInfoReprocessing(
        cameraId,
        node_group_info_3aa,
        node_group_info_isp,
        bayerCropSizePreview,
        bayerCropSizePicture,
        bdsSize,
        pictureW, pictureH,
        pureBayerReprocessing,
        flag3aaIspOtf);

    // m_dump("3AA", cameraId, node_group_info_3aa);
    // m_dump("ISP", cameraId, node_group_info_isp);
}

void ExynosCameraNodeGroup::m_dump(const char *name, int cameraId, camera2_node_group *node_group_info)
{

    ALOGD("[CAM_ID(%d)][%s]-DEBUG(%s[%d]):node_group_info->leader(in : %d, %d, %d, %d) -> (out : %d, %d, %d, %d)(request : %d, vid : %d)",
        cameraId,
        name,
        __FUNCTION__, __LINE__,
        node_group_info->leader.input.cropRegion[0],
        node_group_info->leader.input.cropRegion[1],
        node_group_info->leader.input.cropRegion[2],
        node_group_info->leader.input.cropRegion[3],
        node_group_info->leader.output.cropRegion[0],
        node_group_info->leader.output.cropRegion[1],
        node_group_info->leader.output.cropRegion[2],
        node_group_info->leader.output.cropRegion[3],
        node_group_info->leader.request,
        node_group_info->leader.vid);

    for (int i = 0; i < CAPTURE_NODE_MAX; i++) {
        ALOGD("[CAM_ID(%d)][%s]-DEBUG(%s[%d]):node_group_info->capture[%d](in : %d, %d, %d, %d) -> (out : %d, %d, %d, %d)(request : %d, vid : %d)",
        cameraId,
        name,
        __FUNCTION__, __LINE__,
        i,
        node_group_info->capture[i].input.cropRegion[0],
        node_group_info->capture[i].input.cropRegion[1],
        node_group_info->capture[i].input.cropRegion[2],
        node_group_info->capture[i].input.cropRegion[3],
        node_group_info->capture[i].output.cropRegion[0],
        node_group_info->capture[i].output.cropRegion[1],
        node_group_info->capture[i].output.cropRegion[2],
        node_group_info->capture[i].output.cropRegion[3],
        node_group_info->capture[i].request,
        node_group_info->capture[i].vid);
    }
}

void ExynosCameraNodeGroup3AA::updateNodeGroupInfo(
        int cameraId,
        camera2_node_group *node_group_info,
        ExynosRect bayerCropSize,
        ExynosRect bdsSize,
        int previewW, int previewH,
        int pictureW, int pictureH)
{
    camera2_node_group node_group_info_isp;
    memset(&node_group_info_isp, 0x0, sizeof(camera2_node_group));

    if (cameraId == CAMERA_ID_BACK) {
        updateNodeGroupInfoMainPreview(
            cameraId,
            node_group_info,
            &node_group_info_isp,
            bayerCropSize,
            bdsSize,
            previewW, previewH,
            pictureW, pictureH);
    } else {
        updateNodeGroupInfoFront(
            cameraId,
            node_group_info,
            &node_group_info_isp,
            bayerCropSize,
            bdsSize,
            previewW, previewH,
            pictureW, pictureH);
    }

    // m_dump("3AA", cameraId, node_group_info);
}

void ExynosCameraNodeGroupISP::updateNodeGroupInfo(
        int cameraId,
        camera2_node_group *node_group_info,
        ExynosRect bayerCropSize,
        ExynosRect bdsSize,
        int previewW, int previewH,
        int pictureW, int pictureH,
        bool dis)
{
    camera2_node_group node_group_info_3aa;
    memset(&node_group_info_3aa, 0x0, sizeof(camera2_node_group));

    int ispW = previewW;
    int ispH = previewH;

    if (dis == true) {
        ispW = bdsSize.w;
        ispH = bdsSize.h;
    }

    if (cameraId == CAMERA_ID_BACK) {
        updateNodeGroupInfoMainPreview(
            cameraId,
            &node_group_info_3aa,
            node_group_info,
            bayerCropSize,
            bdsSize,
            ispW, ispH,
            pictureW, pictureH);
    } else {
        updateNodeGroupInfoFront(
            cameraId,
            &node_group_info_3aa,
            node_group_info,
            bayerCropSize,
            bdsSize,
            ispW, ispH,
            pictureW, pictureH);
    }

    // m_dump("ISP", cameraId, node_group_info);
}

void ExynosCameraNodeGroupDIS::updateNodeGroupInfo(
        int cameraId,
        camera2_node_group *node_group_info,
        ExynosRect bayerCropSize,
        ExynosRect bdsSize,
        int previewW, int previewH,
        int pictureW, int pictureH,
        bool dis)
{
    camera2_node_group node_group_info_3aa;
    memset(&node_group_info_3aa, 0x0, sizeof(camera2_node_group));

    if (cameraId == CAMERA_ID_BACK) {
        updateNodeGroupInfoMainPreview(
            cameraId,
            &node_group_info_3aa,
            node_group_info,
            bayerCropSize,
            bdsSize,
            previewW, previewH,
            pictureW, pictureH);

    } else {
        updateNodeGroupInfoFront(
            cameraId,
            &node_group_info_3aa,
            node_group_info,
            bayerCropSize,
            bdsSize,
            previewW, previewH,
            pictureW, pictureH);
    }

    /*
     * make DIS output smaller than output.
     * (DIS output = DIS input / HW_VDIS_RATIO)
     */
    if (dis == true) {
        node_group_info->leader.output.cropRegion[2] = ALIGN_UP((int)(node_group_info->leader.input.cropRegion[2] / HW_VDIS_W_RATIO), 2);
        node_group_info->leader.output.cropRegion[3] = ALIGN_UP((int)(node_group_info->leader.input.cropRegion[3] / HW_VDIS_H_RATIO), 2);
    }

    /*
     * In case of DIS,
     * DIS's output crop size must be same SCP's input crop size.
     * because SCP's input image comes from DIS output filtering.
     * (SCP input = DIS output)
     */
    for (int i = 0; i < 4; i++)
        node_group_info->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[i] = node_group_info->leader.output.cropRegion[i];


    // m_dump("DIS", cameraId, node_group_info);
}

}; /* namespace android */
