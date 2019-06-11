/*
 * Copyright (C) 2014, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_STREAM_MANAGER_H
#define EXYNOS_CAMERA_STREAM_MANAGER_H

#include <cutils/log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utils/List.h>
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <map>
#include <list>

#include <hardware/camera.h>
#include <hardware/camera3.h>
#include <hardware/gralloc.h>

#include "ExynosCameraDefine.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraBufferManager.h"

#include "exynos_format.h"

namespace android {

using namespace std;

namespace EXYNOS_STREAM {
    enum STATE {
        HAL_STREAM_STS_INIT = 0x00,
        HAL_STREAM_STS_INVALID = 0x01,
        HAL_STREAM_STS_VALID = 0x02,
        HAL_STREAM_STS_UNREGISTERED = 0x11,
        HAL_STREAM_STS_REGISTERED = 0x12
    };
};

class ExynosCameraStream  : public RefBase {
public:
    ExynosCameraStream(){};
    virtual ~ExynosCameraStream(){};

    virtual status_t          setStream(camera3_stream_t *stream) = 0;
    virtual status_t          getStream(camera3_stream_t **stream) = 0;
    virtual status_t          setID(int id) = 0;
    virtual status_t          getID(int *id) = 0;
    virtual status_t          setFormat(int id) = 0;
    virtual status_t          getFormat(int *format) = 0;
    virtual status_t          setPlaneCount(int format) = 0;
    virtual status_t          getPlaneCount(int *format) = 0;
    virtual status_t          setOutputPortId(int id) = 0;
    virtual status_t          getOutputPortId(int *id) = 0;
    virtual status_t          setRegisterStream(EXYNOS_STREAM::STATE state) = 0;
    virtual status_t          getRegisterStream(EXYNOS_STREAM::STATE *state) = 0;
    virtual status_t          setRegisterBuffer(EXYNOS_STREAM::STATE state) = 0;
    virtual status_t          getRegisterBuffer(EXYNOS_STREAM::STATE *state) = 0;
    virtual status_t          setRequestBuffer(int bufferCnt) = 0;
    virtual status_t          getRequestBuffer(int *bufferCnt) = 0;
    virtual status_t          setBufferManager(ExynosCameraBufferManager *bufferManager) = 0;
    virtual status_t          getBufferManager(ExynosCameraBufferManager **bufferManager) = 0;
};


class ExynosCamera3Stream  : public ExynosCameraStream {
private:
    ExynosCamera3Stream(){};

public:


    ExynosCamera3Stream(int id, camera3_stream_t *stream);
    virtual ~ExynosCamera3Stream();

    virtual status_t          setStream(camera3_stream_t *stream);
    virtual status_t          getStream(camera3_stream_t **stream);
    virtual status_t          setID(int id);
    virtual status_t          getID(int *id);
    virtual status_t          setFormat(int format);
    virtual status_t          getFormat(int *format);
    virtual status_t          setPlaneCount(int planes);
    virtual status_t          getPlaneCount(int *planes);
    virtual status_t          setOutputPortId(int id);
    virtual status_t          getOutputPortId(int *id);
    virtual status_t          setRegisterStream(EXYNOS_STREAM::STATE state);
    virtual status_t          getRegisterStream(EXYNOS_STREAM::STATE *state);
    virtual status_t          setRegisterBuffer(EXYNOS_STREAM::STATE state);
    virtual status_t          getRegisterBuffer(EXYNOS_STREAM::STATE *state);
    virtual status_t          setRequestBuffer(int bufferCnt);
    virtual status_t          getRequestBuffer(int *bufferCnt);
    virtual status_t          setBufferManager(ExynosCameraBufferManager *bufferManager);
    virtual status_t          getBufferManager(ExynosCameraBufferManager **bufferManager);

private:
    status_t          m_init();
    status_t          m_deinit();

private:
    camera3_stream_t            *m_stream;
    int                         m_id;
    int                         m_actualFormat;
    int                         m_planeCount;
    int                         m_outputPortId;
    EXYNOS_STREAM::STATE        m_registerStream;
    EXYNOS_STREAM::STATE        m_registerBuffer;
    int                         m_requestbuffer;
    ExynosCameraBufferManager   *m_bufferManager;
};

class ExynosCameraStreamManager : public RefBase {
public:
    /* Constructor */
    ExynosCameraStreamManager(int cameraId);

    /* Destructor */
    virtual ~ExynosCameraStreamManager();

    ExynosCameraStream* createStream(int id, camera3_stream_t *stream);
    status_t deleteStream(int id);

    status_t getStream(int id, ExynosCameraStream **stream);

    status_t getStreamKeys(List<int>* keylist);

    bool     findStream(int id);

    status_t setConfig(struct ExynosConfigInfo *config);

    status_t setYuvStreamMaxCount(int32_t count);
    int32_t  getYuvStreamCount(void);

    int      getYuvStreamId(int outputPortId);
    int      getOutputPortId(int streamId);

    status_t dumpCurrentStreamList(void);

protected:
    typedef map<int, ExynosCameraStream*>            StreamInfoMap;
    typedef map<int, ExynosCameraStream*>::iterator  StreamInfoIterator;

private:
    void     m_init();
    void     m_deinit();

    status_t m_insert(int id, ExynosCameraStream *item, StreamInfoMap *list, Mutex *lock);
    status_t m_erase(int id, ExynosCameraStream **item, StreamInfoMap *list, Mutex *lock);
    status_t m_find(int id, StreamInfoMap *list, Mutex *lock);
    status_t m_get(int id, ExynosCameraStream **item, StreamInfoMap *list, Mutex *lock);
    status_t m_delete(int id, StreamInfoMap *list, Mutex *lock);
    status_t m_delete(ExynosCameraStream *stream);

    status_t m_increaseYuvStreamCount(int id);
    status_t m_decreaseYuvStreamCount(int id);

protected:
    int                         m_cameraId;
    struct ExynosConfigInfo     *m_exynosconfig;

    StreamInfoMap               m_streamInfoMap;
    mutable Mutex               m_streamInfoLock;
    int32_t                     m_yuvStreamCount;
    int32_t                     m_yuvStreamMaxCount;
    int                         m_yuvStreamIdMap[3];
};

}; /* namespace android */
#endif
