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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPipeJpeg"
#include <cutils/log.h>

#include "ExynosCameraPipeJpeg.h"

/* For test */
#include "ExynosCameraBuffer.h"

namespace android {

ExynosCameraPipeJpeg::~ExynosCameraPipeJpeg()
{
    this->destroy();
}

status_t ExynosCameraPipeJpeg::create(__unused int32_t *sensorIds)
{
    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipeJpeg::m_mainThreadFunc, "JpegThread");

    m_inputFrameQ = new frame_queue_t(m_mainThread);

    CLOGI("INFO(%s[%d]):create() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeJpeg::destroy(void)
{
    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        delete m_inputFrameQ;
        m_inputFrameQ = NULL;
    }

    if (m_shot_ext != NULL) {
        delete m_shot_ext;
        m_shot_ext = NULL;
    }

    CLOGI("INFO(%s[%d]):destroy() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeJpeg::start(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    /* TODO: check state ready for start */

    return NO_ERROR;
}

status_t ExynosCameraPipeJpeg::stop(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    m_jpegEnc.destroy();

    m_mainThread->requestExitAndWait();

    CLOGD("DEBUG(%s[%d]): thead exited", __FUNCTION__, __LINE__);

    m_inputFrameQ->release();

    return NO_ERROR;
}

status_t ExynosCameraPipeJpeg::startThread(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    start();

    if (m_outputFrameQ == NULL) {
        CLOGE("ERR(%s):outputFrameQ is NULL, cannot start", __FUNCTION__);
        return INVALID_OPERATION;
    }

    m_mainThread->run();

    CLOGI("INFO(%s[%d]):startThread is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeJpeg::m_run(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    status_t ret = 0;
    ExynosCameraFrame *newFrame = NULL;

    ExynosCameraBuffer yuvBuf;
    ExynosCameraBuffer jpegBuf;

    ExynosRect pictureRect;
    ExynosRect thumbnailRect;
    int jpegQuality = m_parameters->getJpegQuality();
    int thumbnailQuality = m_parameters->getThumbnailQuality();
    int jpegformat = (JPEG_INPUT_COLOR_FMT == V4L2_PIX_FMT_YUYV) ?  V4L2_PIX_FMT_JPEG_422 : V4L2_PIX_FMT_JPEG_420;

    memset(m_shot_ext, 0x00, sizeof(struct camera2_shot_ext));

    exif_attribute_t exifInfo;
    m_parameters->getFixedExifInfo(&exifInfo);

    pictureRect.colorFormat = m_parameters->getHwPictureFormat();
    pictureRect.colorFormat = JPEG_INPUT_COLOR_FMT;

    m_parameters->getPictureSize(&pictureRect.w, &pictureRect.h);
    m_parameters->getThumbnailSize(&thumbnailRect.w, &thumbnailRect.h);

    CLOGD("DEBUG(%s[%d]):picture size(%dx%d), thumbnail size(%dx%d)",
            __FUNCTION__, __LINE__, pictureRect.w, pictureRect.h, thumbnailRect.w, thumbnailRect.h);

    ALOGD("DEBUG(%s[%d]):wait JPEG pipe inputFrameQ", __FUNCTION__, __LINE__);
    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout", __FUNCTION__);
        } else {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
        }
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s):new frame is NULL", __FUNCTION__);
        return NO_ERROR;
    }

    ALOGD("DEBUG(%s[%d]):JPEG pipe inputFrameQ output done", __FUNCTION__, __LINE__);

    if (m_parameters->getHalVersion() == IS_HAL_VER_3_2) {
        newFrame->getMetaData(m_shot_ext);

        /* JPEG Quality, Thumbnail Quality Setting */
        jpegQuality = (int) m_shot_ext->shot.ctl.jpeg.quality;
        thumbnailQuality = (int) m_shot_ext->shot.ctl.jpeg.thumbnailQuality;

        /* JPEG Thumbnail Size Setting */
        thumbnailRect.w = m_shot_ext->shot.ctl.jpeg.thumbnailSize[0];
        thumbnailRect.h = m_shot_ext->shot.ctl.jpeg.thumbnailSize[1];
    }
    ret = newFrame->getSrcBuffer(getPipeId(), &yuvBuf);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get src buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return OK;
    }

    ret = newFrame->getDstBuffer(getPipeId(), &jpegBuf);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get dst buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return OK;
    }

    if (m_jpegEnc.create()) {
        CLOGE("ERR(%s):m_jpegEnc.create() fail", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    m_jpegEnc.setExtScalerNum(m_parameters->getScalerNodeNumPicture());

    if (m_jpegEnc.setQuality(jpegQuality)) {
        CLOGE("ERR(%s[%d]):m_jpegEnc.setQuality() fail", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.setSize(pictureRect.w, pictureRect.h)) {
        CLOGE("ERR(%s):m_jpegEnc.setSize() fail", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.setColorFormat(pictureRect.colorFormat)) {
        CLOGE("ERR(%s):m_jpegEnc.setColorFormat() fail", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.setJpegFormat(jpegformat)) {
        CLOGE("ERR(%s):m_jpegEnc.setJpegFormat() fail", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (thumbnailRect.w != 0 && thumbnailRect.h != 0) {
        exifInfo.enableThumb = true;
        if (pictureRect.w < 320 || pictureRect.h < 240) {
            thumbnailRect.w = 160;
            thumbnailRect.h = 120;
        }
        if (m_jpegEnc.setThumbnailSize(thumbnailRect.w, thumbnailRect.h)) {
            CLOGE("ERR(%s):m_jpegEnc.setThumbnailSize(%d, %d) fail", __FUNCTION__, thumbnailRect.w, thumbnailRect.h);
            ret = INVALID_OPERATION;
            goto jpeg_encode_done;
        }
        if (0 < thumbnailQuality && thumbnailQuality <= 100) {
            if (m_jpegEnc.setThumbnailQuality(thumbnailQuality)) {
                ret = INVALID_OPERATION;
                CLOGE("ERR(%s):m_jpegEnc.setThumbnailQuality(%d) fail", __FUNCTION__, thumbnailQuality);
            }
        }
    } else {
        exifInfo.enableThumb = false;
    }

    /* wait for medata update */
    if(newFrame->getMetaDataEnable() == false) {
        CLOGD("DEBUG(%s[%d]): Waiting for update jpeg metadata failed (%d) ", __FUNCTION__, __LINE__, ret);
    }

    /* get dynamic meters for make exif info */
    newFrame->getDynamicMeta(m_shot_ext);
    newFrame->getUserDynamicMeta(m_shot_ext);

    m_parameters->setExifChangedAttribute(&exifInfo, &pictureRect, &thumbnailRect, &m_shot_ext->shot);

    if (m_jpegEnc.setInBuf((int *)&(yuvBuf.fd), (int *)yuvBuf.size)) {
        CLOGE("ERR(%s):m_jpegEnc.setInBuf() fail", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.setOutBuf(jpegBuf.fd[0], jpegBuf.size[0] + jpegBuf.size[1] + jpegBuf.size[2])) {
        CLOGE("ERR(%s):m_jpegEnc.setOutBuf() fail", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.updateConfig()) {
        CLOGE("ERR(%s):m_jpegEnc.updateConfig() fail", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.encode((int *)&jpegBuf.size, &exifInfo, (char **)jpegBuf.addr, m_parameters->getDebugAttribute())) {
        CLOGE("ERR(%s):m_jpegEnc.encode() fail", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    newFrame->setJpegSize(jpegBuf.size[0]);

    ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):set entity state fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return OK;
    }

    m_outputFrameQ->pushProcessQ(&newFrame);

jpeg_encode_done:
    if (ret != NO_ERROR) {
        CLOGD("[jpegBuf.fd[0] %d][jpegBuf.size[0] + jpegBuf.size[1] + jpegBuf.size[2] %d]",
            jpegBuf.fd[0], jpegBuf.size[0] + jpegBuf.size[1] + jpegBuf.size[2]);
        CLOGD("[pictureW %d][pictureH %d][pictureFormat %d]",
            pictureRect.w, pictureRect.h, pictureRect.colorFormat);
    }

    if (m_jpegEnc.flagCreate() == true)
        m_jpegEnc.destroy();

    CLOGI("DEBUG(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

bool ExynosCameraPipeJpeg::m_mainThreadFunc(void)
{
    int ret = 0;

    ret = m_run();
    if (ret < 0) {
        if (ret == TIMED_OUT)
            return true;
        CLOGE("ERR(%s):m_run fail", __FUNCTION__);
        /* TODO: doing exception handling */
        return false;
    }

    /* one time */
    return m_checkThreadLoop();
}

void ExynosCameraPipeJpeg::m_init(void)
{
    m_reprocessing = 1;
    m_csc = NULL;
    m_shot_ext = new struct camera2_shot_ext;
}

}; /* namespace android */
