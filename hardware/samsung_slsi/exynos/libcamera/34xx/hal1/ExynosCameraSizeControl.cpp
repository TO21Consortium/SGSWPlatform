/*
 **
 ** Copyright 2015, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraSizeControl"
#include <cutils/log.h>

#include "ExynosCameraSizeControl.h"

namespace android {

void updateNodeGroupInfo(
        __unused int pipeId,
        __unused ExynosCamera1Parameters *params,
        __unused camera2_node_group *node_group_info)
{
    ALOGE("ERR(%s[%d]):This is invalid function call in 34xx. Use ExynosCameraUtilsModule's APIs", __FUNCTION__, __LINE__);
}

void setLeaderSizeToNodeGroupInfo(
        camera2_node_group *node_group_info,
        int cropX, int cropY,
        int width, int height)
{
    node_group_info->leader.input.cropRegion[0] = cropX;
    node_group_info->leader.input.cropRegion[1] = cropY;
    node_group_info->leader.input.cropRegion[2] = width;
    node_group_info->leader.input.cropRegion[3] = height;

    node_group_info->leader.output.cropRegion[0] = 0;
    node_group_info->leader.output.cropRegion[1] = 0;
    node_group_info->leader.output.cropRegion[2] = width;
    node_group_info->leader.output.cropRegion[3] = height;
}

void setCaptureSizeToNodeGroupInfo(
        camera2_node_group *node_group_info,
        uint32_t perframePosition,
        int width, int height)
{
    node_group_info->capture[perframePosition].input.cropRegion[0] = 0;
    node_group_info->capture[perframePosition].input.cropRegion[1] = 0;
    node_group_info->capture[perframePosition].input.cropRegion[2] = width;
    node_group_info->capture[perframePosition].input.cropRegion[3] = height;

    node_group_info->capture[perframePosition].output.cropRegion[0] = 0;
    node_group_info->capture[perframePosition].output.cropRegion[1] = 0;
    node_group_info->capture[perframePosition].output.cropRegion[2] = width;
    node_group_info->capture[perframePosition].output.cropRegion[3] = height;
}

void setCaptureCropNScaleSizeToNodeGroupInfo(
        camera2_node_group *node_group_info,
        uint32_t perframePosition,
        int cropX, int cropY,
        int cropWidth, int cropHeight,
        int targetWidth, int targetHeight)
{
    node_group_info->capture[perframePosition].input.cropRegion[0] = cropX;
    node_group_info->capture[perframePosition].input.cropRegion[1] = cropY;
    node_group_info->capture[perframePosition].input.cropRegion[2] = cropWidth;
    node_group_info->capture[perframePosition].input.cropRegion[3] = cropHeight;

    node_group_info->capture[perframePosition].output.cropRegion[0] = 0;
    node_group_info->capture[perframePosition].output.cropRegion[1] = 0;
    node_group_info->capture[perframePosition].output.cropRegion[2] = targetWidth;
    node_group_info->capture[perframePosition].output.cropRegion[3] = targetHeight;
}

bool useSizeControlApi(void)
{
    bool use = false;
#ifdef USE_SIZE_CONTROL_API
    use = USE_SIZE_CONTROL_API;
#else
    use = false;
    ALOGV("INFO(%s[%d]):Use Legacy Utils Module API", __FUNCTION__, __LINE__);
#endif
    return use;
}

}; /* namespace android */
