/*
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

#define LOG_TAG "ExynosCameraFusionWrapper"

#include "ExynosCameraFusionWrapper.h"

ExynosCameraFusionWrapper::ExynosCameraFusionWrapper()
{
    ALOGD("DEBUG(%s[%d]):new ExynosCameraFusionWrapper object allocated", __FUNCTION__, __LINE__);

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_flagCreated[i] = false;

        m_width[i] = 0;
        m_height[i] = 0;
        m_stride[i] = 0;
    }
}

ExynosCameraFusionWrapper::~ExynosCameraFusionWrapper()
{
    status_t ret = NO_ERROR;

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (flagCreate(i) == true) {
            ret = destroy(i);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):destroy(%d) fail", __FUNCTION__, __LINE__, i);
            }
        }
    }
}

status_t ExynosCameraFusionWrapper::create(int cameraId,
                                           int srcWidth, int srcHeight,
                                           int dstWidth, int dstHeight,
                                           char *calData, int calDataSize)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    Mutex::Autolock lock(m_createLock);

    status_t ret = NO_ERROR;

    if (CAMERA_ID_MAX <= cameraId) {
        CLOGE("ERR(%s[%d]):invalid cameraId(%d). so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    if (m_flagCreated[cameraId] == true) {
        CLOGE("ERR(%s[%d]):cameraId(%d) is alread created. so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    if (srcWidth == 0 || srcWidth == 0 || dstWidth == 0 || dstHeight == 0) {
        CLOGE("ERR(%s[%d]):srcWidth == %d || srcWidth == %d || dstWidth == %d || dstHeight == %d. so, fail",
            __FUNCTION__, __LINE__, srcWidth, srcWidth, dstWidth, dstHeight);
        return INVALID_OPERATION;
    }

    m_init(cameraId);

    CLOGD("DEBUG(%s[%d]):create(calData(%p), calDataSize(%d), srcWidth(%d), srcHeight(%d), dstWidth(%d), dstHeight(%d)",
        __FUNCTION__, __LINE__,
        calData, calDataSize, srcWidth, srcHeight, dstWidth, dstHeight);

    // set info int width, int height, int stride
    m_width      [cameraId] = srcWidth;
    m_height     [cameraId] = srcHeight;
    m_stride     [cameraId] = srcWidth;

    // declare it created
    m_flagCreated[cameraId] = true;

    return NO_ERROR;
}

status_t ExynosCameraFusionWrapper::destroy(int cameraId)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    Mutex::Autolock lock(m_createLock);

    status_t ret = NO_ERROR;

    if (CAMERA_ID_MAX <= cameraId) {
        CLOGE("ERR(%s[%d]):invalid cameraId(%d). so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    if (m_flagCreated[cameraId] == false) {
        CLOGE("ERR(%s[%d]):cameraId(%d) is alread destroyed. so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    CLOGD("DEBUG(%s[%d]):destroy()", __FUNCTION__, __LINE__);

    m_flagCreated[cameraId] = false;

    return NO_ERROR;
}

bool ExynosCameraFusionWrapper::flagCreate(int cameraId)
{
    Mutex::Autolock lock(m_createLock);

    if (CAMERA_ID_MAX <= cameraId) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid cameraId(%d), assert!!!!",
                __FUNCTION__, __LINE__, cameraId);
    }

    return m_flagCreated[cameraId];
}

bool ExynosCameraFusionWrapper::flagReady(int cameraId)
{
    return m_flagCreated[cameraId];
}

status_t ExynosCameraFusionWrapper::execute(int cameraId,
                                            __unused struct camera2_shot_ext *shot_ext[], __unused DOF *dof[],
                                            ExynosCameraBuffer srcBuffer[], ExynosRect srcRect[], __unused ExynosCameraBufferManager *srcBufferManager[],
                                            ExynosCameraBuffer dstBuffer,   ExynosRect dstRect,   __unused ExynosCameraBufferManager *dstBufferManager)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    status_t ret = NO_ERROR;

    if (this->flagCreate(cameraId) == false) {
        CLOGE("ERR(%s[%d]):flagCreate(%d) == false. so fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    m_emulationProcessTimer.start();

    ret = m_execute(cameraId,
                    srcBuffer, srcRect,
                    dstBuffer, dstRect);
    m_emulationProcessTimer.stop();
    m_emulationProcessTime = (int)m_emulationProcessTimer.durationUsecs();

    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_execute() fail", __FUNCTION__, __LINE__);
    }

    return ret;
}

status_t ExynosCameraFusionWrapper::m_execute(int cameraId,
                                              ExynosCameraBuffer srcBuffer[], ExynosRect srcRect[],
                                              ExynosCameraBuffer dstBuffer,   ExynosRect dstRect)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    status_t ret = NO_ERROR;

    char *srcYAddr    = NULL;
    char *srcCbcrAddr = NULL;

    char *dstYAddr    = NULL;
    char *dstCbcrAddr = NULL;

    unsigned int bpp = 0;
    unsigned int planeCount  = 1;

    int srcPlaneSize = 0;
    int srcHalfPlaneSize = 0;

    int dstPlaneSize = 0;
    int dstHalfPlaneSize = 0;

    int copySize = 0;

    /*
     * if previous emulationProcessTime is slow than 33msec,
     * we need change the next copy time
     *
     * ex :
     * frame 0 :
     * 1.0(copyRatio) = 33333 / 33333(previousFusionProcessTime : init value)
     * 1.0  (copyRatio) = 1.0(copyRatio) * 1.0(m_emulationCopyRatio);
     * m_emulationCopyRatio = 1.0
     * m_emulationProcessTime = 66666

     * frame 1 : because of frame0's low performance, shrink down copyRatio.
     * 0.5(copyRatio) = 33333 / 66666(previousFusionProcessTime)
     * 0.5(copyRatio) = 0.5(copyRatio) * 1.0(m_emulationCopyRatio);
     * m_emulationCopyRatio = 0.5
     * m_emulationProcessTime = 33333

     * frame 2 : acquire the proper copy time
     * 1.0(copyRatio) = 33333 / 33333(previousFusionProcessTime)
     * 0.5(copyRatio) = 1.0(copyRatio) * 0.5(m_emulationCopyRatio);
     * m_emulationCopyRatio = 0.5
     * m_emulationProcessTime = 16666

     * frame 3 : because of frame2's fast performance, increase copyRatio.
     * 2.0(copyRatio) = 33333 / 16666(previousFusionProcessTime)
     * 1.0(copyRatio) = 2.0(copyRatio) * 0.5(m_emulationCopyRatio);
     * m_emulationCopyRatio = 1.0
     * m_emulationProcessTime = 33333
     */
    int previousFusionProcessTime = m_emulationProcessTime;
    if (previousFusionProcessTime <= 0)
        previousFusionProcessTime = 1;

    float copyRatio = (float)FUSION_PROCESSTIME_STANDARD / (float)previousFusionProcessTime;
    copyRatio = copyRatio * m_emulationCopyRatio;

    if (1.0f <= copyRatio) {
        copyRatio = 1.0f;
    } else if (0.1f < copyRatio) {
        copyRatio -= 0.05f; // threshold value : 5%
    } else {
        CLOGW("WARN(%s[%d]):copyRatio(%d) is too smaller than 0.1f. previousFusionProcessTime(%d), m_emulationCopyRatio(%f)",
            __FUNCTION__, __LINE__, copyRatio, previousFusionProcessTime, m_emulationCopyRatio);
    }

    m_emulationCopyRatio = copyRatio;

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (m_flagValidCameraId[i] == false)
            continue;

        srcPlaneSize = srcRect[i].fullW * srcRect[i].fullH;
        srcHalfPlaneSize = srcPlaneSize / 2;

        dstPlaneSize = dstRect.fullW * dstRect.fullH;
        dstHalfPlaneSize = dstPlaneSize / 2;

        copySize = (srcHalfPlaneSize < dstHalfPlaneSize) ? srcHalfPlaneSize : dstHalfPlaneSize;

        ret = getYuvFormatInfo(srcRect[i].colorFormat, &bpp, &planeCount);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getYuvFormatInfo(srcRect[%d].colorFormat(%x)) fail", __FUNCTION__, __LINE__, i, srcRect[i].colorFormat);
        }

        srcYAddr    = srcBuffer[i].addr[0];
        dstYAddr    = dstBuffer.addr[0];

        switch (planeCount) {
        case 1:
            srcCbcrAddr = srcBuffer[i].addr[0] + srcRect[i].fullW * srcRect[i].fullH;
            dstCbcrAddr = dstBuffer.addr[0]    + dstRect.fullW    * dstRect.fullH;
            break;
        case 2:
            srcCbcrAddr = srcBuffer[i].addr[1];
            dstCbcrAddr = dstBuffer.addr[1];
            break;
        default:
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid planeCount(%d), assert!!!!",
                    __FUNCTION__, __LINE__, planeCount);
            break;
        }

        EXYNOS_CAMERA_FUSION_WRAPPER_DEBUG_LOG("DEBUG(%s[%d]):fusion emulationn running ~~~ memcpy(%d, %d, %d) by src(%d, %d), dst(%d, %d), previousFusionProcessTime(%d) copyRatio(%f)",
            __FUNCTION__, __LINE__,
            dstBuffer.addr[0], srcBuffer[i].addr[0], copySize,
            srcRect[i].fullW, srcRect[i].fullH,
            dstRect.fullW, dstRect.fullH,
            previousFusionProcessTime, copyRatio);

        if (i == m_subCameraId) {
            dstYAddr    += dstHalfPlaneSize;
            dstCbcrAddr += dstHalfPlaneSize / 2;
        }

        if (srcRect[i].fullW == dstRect.fullW &&
            srcRect[i].fullH == dstRect.fullH) {
            int oldCopySize = copySize;
            copySize = (int)((float)copySize * copyRatio);

            if (oldCopySize < copySize) {
                CLOGW("WARN(%s[%d]):oldCopySize(%d) < copySize(%d). just adjust oldCopySize",
                    __FUNCTION__, __LINE__, oldCopySize, copySize);

                copySize = oldCopySize;
            }

            memcpy(dstYAddr,    srcYAddr,    copySize);
            memcpy(dstCbcrAddr, srcCbcrAddr, copySize / 2);
        } else {
            int width  = (srcRect[i].fullW < dstRect.fullW) ? srcRect[i].fullW : dstRect.fullW;
            int height = (srcRect[i].fullH < dstRect.fullH) ? srcRect[i].fullH : dstRect.fullH;

            int oldHeight = height;
            height = (int)((float)height * copyRatio);

            if (oldHeight < height) {
                CLOGW("WARN(%s[%d]):oldHeight(%d) < height(%d). just adjust oldHeight",
                    __FUNCTION__, __LINE__, oldHeight, height);

                height = oldHeight;
            }

            for (int h = 0; h < height / 2; h++) {
                memcpy(dstYAddr,    srcYAddr,    width);
                srcYAddr += srcRect[i].fullW;
                dstYAddr += dstRect.fullW;
            }

            for (int h = 0; h < height / 4; h++) {
                memcpy(dstCbcrAddr, srcCbcrAddr, width);
                srcCbcrAddr += srcRect[i].fullW;
                dstCbcrAddr += dstRect.fullW;
            }
        }
    }

    return ret;
}
void ExynosCameraFusionWrapper::m_init(int cameraId)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    getDualCameraId(&m_mainCameraId, &m_subCameraId);

    CLOGD("DEBUG(%s[%d]):m_mainCameraId(CAMERA_ID_%d), m_subCameraId(CAMERA_ID_%d)",
        __FUNCTION__, __LINE__, m_mainCameraId, m_subCameraId);

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_flagValidCameraId[i] = false;
    }

    m_flagValidCameraId[m_mainCameraId] = true;
    m_flagValidCameraId[m_subCameraId] = true;

    m_emulationProcessTime = FUSION_PROCESSTIME_STANDARD;
    m_emulationCopyRatio = 1.0f;
}
