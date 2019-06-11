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

#ifndef EXYNOS_CAMERA_PIPE_FLITE_H
#define EXYNOS_CAMERA_PIPE_FLITE_H

#include "ExynosCameraPipe.h"

namespace android {

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;

#define FLITE_CNTS  (FIMC_IS_VIDEO_SS3_NUM - FIMC_IS_VIDEO_SS0_NUM + 1)

class ExynosCameraPipeFlite : protected virtual ExynosCameraPipe {
public:
    ExynosCameraPipeFlite()
    {
        m_init();
    }

    ExynosCameraPipeFlite(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums)  : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        m_init();
    }

    virtual ~ExynosCameraPipeFlite();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);
    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds = NULL);

    virtual status_t        sensorStream(bool on);

protected:
    virtual bool            m_mainThreadFunc(void);
    virtual status_t        m_getBuffer(void);
    virtual status_t        m_setPipeInfo(camera_pipe_info_t *pipeInfos);

private:
    void                    m_init(void);

private:
    /*
     * Multi-singleton :
     * User must call createInstance to get PipeLite obj.
     * because 4 flites must be open once.
     *
     */
    static ExynosCameraNode *m_createNode(int cameraId, int nodeNum)
    {
        Mutex::Autolock lock(g_nodeInstanceMutex);

        status_t ret = NO_ERROR;

        int index = nodeNum - FIMC_IS_VIDEO_SS0_NUM + 1;

        if (index < 0 || FLITE_CNTS <= index)
        {
            ALOGE("[CAM_ID(%d)]-ERR(%s[%d]):invalid Index(%d) fail", cameraId, __FUNCTION__, __LINE__, index);
            return NULL;
        }

        if (g_node[index] == NULL) {
            if (g_nodeRefCount[index] != 0) {
                ALOGW("[CAM_ID(%d)]-WARN(%s[%d]):invalid g_nodeRefCount[%d](%d). so, set 0",
                    cameraId, __FUNCTION__, __LINE__, index, g_nodeRefCount[index]);

                g_nodeRefCount[index] = 0;
            }

            g_node[index] = new ExynosCameraNode();

            ALOGD("[CAM_ID(%d)]-DEBUG(%s[%d]): new g_node[%d]",
                cameraId, __FUNCTION__, __LINE__, index);

            ret = g_node[index]->create("FLITE", cameraId);
            if (ret < 0) {
                ALOGE("[CAM_ID(%d)]-ERR(%s[%d]): create(FLITE) fail, ret(%d)",
                    cameraId, __FUNCTION__, __LINE__, ret);

                SAFE_DELETE(g_node[index]);
                return NULL;
            }

            ret = g_node[index]->open(nodeNum);
            if (ret < 0) {
                ALOGE("[CAM_ID(%d)]-ERR(%s[%d]): open(%d) fail, ret(%d)",
                    cameraId, __FUNCTION__, __LINE__, nodeNum, ret);

                SAFE_DELETE(g_node[index]);
                return NULL;
            }

            ALOGD("[CAM_ID(%d)]-DEBUG(%s[%d]):Node(%d) opened",
                cameraId, __FUNCTION__, __LINE__, nodeNum);
        } else {
            ALOGD("[CAM_ID(%d)]-DEBUG(%s[%d]): skip new g_nodeRefCount[%d] : (%d)",
                cameraId, __FUNCTION__, __LINE__, index, g_nodeRefCount[index]);
        }

        /* when calll this, increase ref */
        g_nodeRefCount[index]++;

        return g_node[index];
    }

    /*
     * Multi-singleton :
     * User must call DestoryInstance to delete PipeLite obj.
     * This will destroy obj, when only g_nodeRefCount == 0.
     */
    static void  m_destroyNode(int cameraId, ExynosCameraNode * obj)
    {
        if (obj == NULL) {
            ALOGE("[CAM_ID(%d)]-ERR(%s[%d]):obj == NULL. so fail",
                cameraId, __FUNCTION__, __LINE__);
            return;
        }

        Mutex::Autolock lock(g_nodeInstanceMutex);

        for (int index = 0; index < FLITE_CNTS; index++)
        {
            if (obj == g_node[index]) {
                if (1 < g_nodeRefCount[index]) {
                    ALOGD("[CAM_ID(%d)]-DEBUG(%s[%d]): skip delete g_nodeRefCount[%d] : (%d)",
                        cameraId, __FUNCTION__, __LINE__, index, g_nodeRefCount[index]);

                    g_nodeRefCount[index]--;
                } else if (g_nodeRefCount[index] == 1) {
                    ALOGD("[CAM_ID(%d)]-DEBUG(%s[%d]): delete g_nodeRefCount[%d] : (%d)",
                        cameraId, __FUNCTION__, __LINE__, index, g_nodeRefCount[index]);

                    if (g_node[index]->close() != NO_ERROR) {
                        ALOGE("[CAM_ID(%d)]-DEBUG(%s[%d]): close fail",
                            cameraId, __FUNCTION__, __LINE__);
                    }

                    SAFE_DELETE(g_node[index]);
                    g_nodeRefCount[index]--;
                } else { /* g_nodeRefCount[index] < 1) */
                    ALOGW("[CAM_ID(%d)]-WARN(%s[%d]):invalid g_nodeRefCount[%d](%d). so, set 0",
                        cameraId, __FUNCTION__, __LINE__, index, g_nodeRefCount[index]);

                    g_nodeRefCount[index] = 0;
                }
            }
        }
    }

    /* Override */
    status_t m_updateMetadataToFrame(void *metadata, int index);

private:
//#ifdef SHOT_RECOVERY
    int                     retryGetBufferCount;
//#endif

    /* global variable for multi-singleton */
    static Mutex             g_nodeInstanceMutex;
    static ExynosCameraNode *g_node[FLITE_CNTS];
    static int               g_nodeRefCount[FLITE_CNTS];
};

}; /* namespace android */

#endif
