/*
 * Copyright 2015, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXYNOS_CAMERA_NODE_JPEG_HAL_H__
#define EXYNOS_CAMERA_NODE_JPEG_HAL_H__

#include "ExynosCameraNode.h"

using namespace android;

namespace android {
/* ExynosCameraNode
 *
 * ingroup Exynos
 */

/*
 * Mapping table
 *
 * |-------------------------------------------------------------------------
 * | ExynosCamera        | ExynosCameraNode             | Jpeg HAL Interface
 * |-------------------------------------------------------------------------
 * | setSize()           | setSize() - NONE             | S_FMT
 * | setColorFormat()    | setColorFormat() - NONE      | S_FMT
 * | setBufferType()     | setBufferTyoe() - NONE       | S_FMT
 * | prepare()           | prepare() - m_setInput()     | S_INPUT
 * |                     | prepare() - m_setFmt()       | S_FMT
 * | reqBuffer()         | reqBuffer() - m_reqBuf()     | REQ_BUF
 * | queryBuffer()       | queryBuffer() - m_queryBuf() | QUERY_BUF
 * | setBuffer()         | setBuffer() - m_qBuf()       | Q_BUF
 * | getBuffer()         | getBuffer() - m_qBuf()       | Q_BUF
 * | putBuffer()         | putBuffer() - m_dqBuf()      | DQ_BUF
 * | start()             | start() - m_streamOn()       | STREAM_ON
 * | polling()           | polling() - m_poll()         | POLL
 * |-------------------------------------------------------------------------
 * | setBufferRef()      |                              |
 * | getSize()           |                              |
 * | getColorFormat()    |                              |
 * | getBufferType()     |                              |
 * |-------------------------------------------------------------------------
 *
 */

class ExynosCameraNodeJpegHAL : public virtual ExynosCameraNode {
private:

public:
    /* Constructor */
    ExynosCameraNodeJpegHAL();
    /* Destructor */
    virtual ~ExynosCameraNodeJpegHAL();

    /* Create the instance */
    status_t create(const char *nodeName,
                            int cameraId,
                            enum EXYNOS_CAMERA_NODE_JPEG_HAL_LOCATION location,
                            ExynosJpegEncoderForCamera *jpegEncoder);

    /* open Node */
    status_t open(int videoNodeNum);
    /* open Node */
    status_t open(int videoNodeNum, bool useThumbnailHWFC);
    /* close Node */
    status_t close(void);
    /* get file descriptor */
    status_t getFd(int *fd);
    /* get Jpeg Encoder */
    status_t getJpegEncoder(ExynosJpegEncoderForCamera **jpegEncoder);
    /* get name */
    char    *getName(void);
    /* get video Num */
    int      getNodeNum(void);

    /* set v4l2 color format */
    status_t setColorFormat(int v4l2Colorformat, int planeCount, enum YUV_RANGE yuvRange = YUV_FULL_RANGE);

    /* set size */
    status_t setQuality(int quality);
    status_t setQuality(const unsigned char qtable[]);

    /* set size */
    status_t setSize(int w, int h);

    /* request buffers */
    status_t reqBuffers(void);
    /* clear buffers */
    status_t clrBuffers(void);
    /* check buffers */
    unsigned int reqBuffersCount(void);

    /* set id */
    status_t setControl(unsigned int id, int value);
    status_t getControl(unsigned int id, int *value);

    /* polling */
    status_t polling(void);

    /* setInput */
    status_t setInput(int sensorId);
    /* getInput */
    int      getInput(void);
    /* resetInput */
    int      resetInput(void);

    /* setCrop */
    status_t setCrop(enum v4l2_buf_type type, int x, int y, int w, int h);

    /* setFormat */
    status_t setFormat(void);
    status_t setFormat(unsigned int bytesPerPlane[]);

    /* set capture information */
    status_t setExifInfo(exif_attribute_t *exifInfo);
    status_t setDebugInfo(debug_attribute_t *debugInfo);

    /* startNode */
    status_t start(void);
    /* stopNode */
    status_t stop(void);

    /* Check if the instance was created */
    bool isCreated(void);
    /* Check if it start */
    bool isStarted(void);

    /* prepare Buffers */
    virtual status_t prepareBuffer(ExynosCameraBuffer *buf);

    /* putBuffer */
    status_t putBuffer(ExynosCameraBuffer *buf);

    /* getBuffer */
    status_t getBuffer(ExynosCameraBuffer *buf, int *dqIndex);

    /* dump the object info */
    void dump(void);
    /* dump state info */
    void dumpState(void);
    /* dump queue info */
    void dumpQueue(void);

private:

    /*
     * thoes member value should be declare in private
     * but we declare in publuc to support backward compatibility
     */
public:

private:
    ExynosJpegEncoderForCamera *m_jpegEncoder;
    enum EXYNOS_CAMERA_NODE_JPEG_HAL_LOCATION   m_jpegNodeLocation;
    ExynosCameraBuffer  m_srcBuffer;
    ExynosCameraBuffer  m_dstBuffer;
    exif_attribute_t    m_exifInfo;
    debug_attribute_t   m_debugInfo;
};

};
#endif /* EXYNOS_CAMERA_NODE_JPEG_HAL_H__ */
