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

#ifndef EXYNOS_CAMERA3_HW_IMPLEMENTATION_H
#define EXYNOS_CAMERA3_HW_IMPLEMENTATION_H

#include "ExynosCameraDefine.h"

#include "ExynosCameraRequestManager.h"
#include "ExynosCameraStreamManager.h"
#include "ExynosCameraMetadataConverter.h"
#include "ExynosCamera3Parameters.h"
#include "ExynosCameraFrameManager.h"
#include "ExynosCamera3FrameFactory.h"
#include "ExynosCamera3FrameFactoryPreview.h"
#include "ExynosCamera3FrameReprocessingFactory.h"

namespace android {

typedef struct ExynosCameraRequestInfo {
    ExynosCameraRequest *request;
    uint32_t sensorControledFrameCount;
} request_info_t;

typedef ExynosCameraThread<ExynosCamera3> mainCameraThread;
typedef ExynosCameraList<ExynosCamera3FrameFactory *> framefactory3_queue_t;

class ExynosCamera3 {
public:
    ExynosCamera3() {};
    ExynosCamera3(int cameraId, camera_metadata_t **info);
    virtual             ~ExynosCamera3();

    /** Startup */
    status_t    initilizeDevice(const camera3_callback_ops *callbackOps);

    /** Stream configuration and buffer registration */
    status_t    configureStreams(camera3_stream_configuration_t *stream_list);

    status_t    registerStreamBuffers(const camera3_stream_buffer_set_t *buffer_set);

    /** Template request settings provision */
    status_t    construct_default_request_settings(camera_metadata_t **request, int type);

    /** Submission of capture requests to HAL */
    status_t    processCaptureRequest(camera3_capture_request *request);

    /** Vendor metadata registration */
    void        get_metadata_vendor_tag_ops(const camera3_device_t *, vendor_tag_query_ops_t *ops);

    /** Flush all currently in-process captures and all buffers */
    status_t    flush(void);

    /** Print out debugging state for the camera device */
    void        dump(void);

    /** Stop */
    status_t    releaseDevice(void);

    void        release();

    /* Common functions */
    int         getCameraId() const;

private:
    /* Helper functions for initialization */
    void        m_createThreads(void);
    void        m_createManagers(void);

    /* Helper functions for notification */
    status_t    m_sendRawCaptureResult(ExynosCameraFrame *frame, uint32_t pipeId, bool isSrc);
    status_t    m_sendZSLCaptureResult(ExynosCameraFrame *frame, uint32_t pipeId, bool isSrc);
    status_t    m_sendNotify(uint32_t frameNumber, int type);

    /* Helper functions of Buffer operation */
    status_t    m_createIonAllocator(ExynosCameraIonAllocator **allocator);
    status_t    m_createInternalBufferManager(ExynosCameraBufferManager **bufferManager, const char *name);
    status_t    m_createServiceBufferManager(ExynosCameraBufferManager **bufferManager, const char *name);
    status_t    m_createBufferManager(
                    ExynosCameraBufferManager **bufferManager,
                    const char *name,
                    buffer_manager_type type = BUFFER_MANAGER_ION_TYPE);

    status_t    m_setBuffers(void);
    status_t    m_putBuffers(ExynosCameraBufferManager *bufManager, int bufIndex);

    status_t    m_registerStreamBuffers(camera3_capture_request *request);
    status_t    m_registerBuffers(              /* for serviceBufferManager */
                    ExynosCameraBufferManager *bufManager,
                    int requestKey,
                    const camera3_stream_buffer_t *streamBuffer);

    status_t    m_allocBuffers(
                    ExynosCameraBufferManager *bufManager,
                    int  planeCount,
                    unsigned int *planeSize,
                    unsigned int *bytePerLine,
                    int  startIndex,            /* for serviceBufferManager */
                    int  reqBufCount,
                    bool createMetaPlane,
                    bool needMmap);

    status_t    m_allocBuffers(
                    ExynosCameraBufferManager *bufManager,
                    int  planeCount,
                    unsigned int *planeSize,
                    unsigned int *bytePerLine,
                    int  reqBufCount,
                    bool createMetaPlane,
                    bool needMmap = false);
    status_t    m_allocBuffers(
                    ExynosCameraBufferManager *bufManager,
                    int  planeCount,
                    unsigned int *planeSize,
                    unsigned int *bytePerLine,
                    int  minBufCount,
                    int  maxBufCount,
                    exynos_camera_buffer_type_t type,
                    bool createMetaPlane,
                    bool needMmap = false);
    status_t    m_allocBuffers(
                    ExynosCameraBufferManager *bufManager,
                    int  planeCount,
                    unsigned int *planeSize,
                    unsigned int *bytePerLine,
                    int  minBufCount,
                    int  maxBufCount,
                    exynos_camera_buffer_type_t type,
                    buffer_manager_allocation_mode_t allocMode,
                    bool createMetaPlane,
                    bool needMmap = false);

    status_t    m_releaseBuffers(void);

    /* helper functions for set buffer to frame */
    status_t    m_setupEntity(uint32_t pipeId, ExynosCameraFrame *newFrame,
                                ExynosCameraBuffer *srcBuf = NULL,
                                ExynosCameraBuffer *dstBuf = NULL);
    status_t    m_setSrcBuffer(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer);
    status_t    m_setDstBuffer(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer);

    /* status_t    m_setupEntity(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBufferManager *srcBufMgr, ExynosCameraBufferManager *dstBufMgr); */
    status_t    m_setSrcBuffer(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer, ExynosCameraBufferManager *bufMgr);
    status_t    m_setDstBuffer(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer, ExynosCameraBufferManager *bufMgr);

    status_t    m_resetBufferState(uint32_t pipeId, ExynosCameraFrame *frame);

    status_t    m_getBufferManager(uint32_t pipeId, ExynosCameraBufferManager **bufMgr, uint32_t direction);

    /* helper functions for frame factory */
    status_t    m_constructFrameFactory(void);
    status_t    m_startFrameFactory(ExynosCamera3FrameFactory *factory);
    status_t    m_startReprocessingFrameFactory(ExynosCamera3FrameFactory *factory);
    status_t    m_stopFrameFactory(ExynosCamera3FrameFactory *factory);
    status_t    m_stopReprocessingFrameFactory(ExynosCamera3FrameFactory *factory);
    status_t    m_deinitFrameFactory();

    /* frame Generation / Done handler */
    status_t    m_createRequestFrameFunc(ExynosCameraRequest *request);
    status_t    m_createInternalFrameFunc(void);
    status_t    m_createPrepareFrameFunc(ExynosCameraRequest *request);
    status_t    m_createFrameFunc(void);

    status_t    m_previewframeHandler(ExynosCameraRequest *request, ExynosCamera3FrameFactory *targetfactory);
    status_t    m_captureframeHandler(ExynosCameraRequest *request, ExynosCamera3FrameFactory *targetfactory);
    bool        m_previewStreamFunc(ExynosCameraFrame *newFrame, int pipeId);

    void        m_updateCropRegion(struct camera2_shot_ext *shot_ext);
    status_t    m_updateJpegControlInfo(const struct camera2_shot_ext *shot_ext);

    /* helper functions for frame */
    status_t    m_generateFrame(int32_t frameCount,
                                ExynosCamera3FrameFactory *factory,
                                List<ExynosCameraFrame *> *list,
                                Mutex *listLock,
                                ExynosCameraFrame **newFrame);
    status_t    m_generateInternalFrame(uint32_t frameCount,
                                        ExynosCamera3FrameFactory *factory,
                                        List<ExynosCameraFrame *> *list,
                                        Mutex *listLock,
                                        ExynosCameraFrame **newFrame);
    status_t    m_searchFrameFromList(List<ExynosCameraFrame *> *list,
                                      Mutex *listLock,
                                      uint32_t frameCount,
                                      ExynosCameraFrame **frame);
    status_t    m_removeFrameFromList(List<ExynosCameraFrame *> *list,
                                      Mutex *listLock,
                                      ExynosCameraFrame *frame);

    status_t    m_clearList(List<ExynosCameraFrame *> *list, Mutex *listLock);

    status_t    m_removeInternalFrames(List<ExynosCameraFrame *> *list, Mutex *listLock);
    status_t    m_releaseInternalFrame(ExynosCameraFrame *frame);
    status_t    m_generateDuplicateBuffers(ExynosCameraFrame *frame, int pipeIdSrc);
    status_t    m_updateTimestamp(ExynosCameraFrame *frame, ExynosCameraBuffer *timestampBuffer, bool flagPushResult = true);
    status_t    m_handlePreviewFrame(ExynosCameraFrame *frame, int pipeId);
    status_t    m_handleInternalFrame(ExynosCameraFrame *frame);
    status_t    m_handleIsChainDone(ExynosCameraFrame *frame);
    status_t    m_handleScalerDone(ExynosCameraFrame *frame);
    status_t    m_handleThumbnailReprocessingFrame(ExynosCameraFrame *frame);
    status_t    m_handleYuvCaptureFrame(ExynosCameraFrame *frame);
    status_t    m_handleJpegFrame(ExynosCameraFrame *frame);
    status_t    m_handleBayerBuffer(ExynosCameraFrame *frame);

    /* helper functions for request */
    status_t    m_pushRequest(camera3_capture_request *request);
    status_t    m_popRequest(ExynosCameraRequest **request);
    status_t    m_deleteRequest(uint32_t frameCount);
    status_t    m_pushResult(uint32_t frameCount, struct camera2_shot_ext *src_ext);
    status_t    m_pushJpegResult(ExynosCameraFrame *frame, int size, ExynosCameraBuffer *buffer);
    ExynosCameraRequest*    m_popResult(CameraMetadata &request, uint32_t frameCount);
    bool        m_needNotify(ExynosCameraRequest *request);
    void        m_updateCurrentShot(void);

    /* helper functions for configuration options */
    uint32_t    m_getBayerPipeId(void);
    status_t    m_setFrameManager();
    status_t    m_setupFrameFactoryToRequest();
    status_t    m_setConfigInform();
    status_t    m_setStreamInfo(camera3_stream_configuration *streamList);
    status_t    m_enumStreamInfo(camera3_stream_t *stream);

    status_t    m_getBayerServiceBuffer(ExynosCameraFrame *frame, ExynosCameraBuffer *buffer);
    status_t    m_getBayerBuffer(uint32_t pipeId, uint32_t frameCount, ExynosCameraBuffer *buffer, ExynosCameraFrameSelector *selector, camera2_shot_ext *updateDmShot = NULL);
    status_t    m_checkBufferAvailable(uint32_t pipeId, ExynosCameraBufferManager *bufferMgr);
    status_t    m_convertingStreamToShotExt(ExynosCameraBuffer *buffer, struct camera2_node_output *outputInfo);

    status_t    m_doDestCSC(bool enableCSC, ExynosCameraFrame *frame, int pipeIdSrc, int halStreamId, int pipeExtScalerId);

public:


private:
    ExynosCameraRequestManager              *m_requestMgr;
    ExynosCameraMetadataConverter           *m_metadataConverter;
    ExynosCamera3Parameters                 *m_parameters;
    ExynosCameraStreamManager               *m_streamManager;

    ExynosCamera3FrameFactory               *m_frameFactory[FRAME_FACTORY_TYPE_MAX];
    framefactory3_queue_t                   *m_frameFactoryQ;

    ExynosCameraActivityControl             *m_activityControl;

    ExynosCameraIonAllocator                *m_ionAllocator;

    /* Internal buffer managers */
    ExynosCameraBufferManager               *m_fliteBufferMgr;
    ExynosCameraBufferManager               *m_3aaBufferMgr;
    ExynosCameraBufferManager               *m_ispBufferMgr;
    ExynosCameraBufferManager               *m_internalScpBufferMgr;
    ExynosCameraBufferManager               *m_vraBufferMgr;
    ExynosCameraBufferManager               *m_gscBufferMgr;

    /* internal reprocessing buffer managers */
    ExynosCameraBufferManager               *m_yuvCaptureReprocessingBufferMgr;
    ExynosCameraBufferManager               *m_ispReprocessingBufferMgr;
    ExynosCameraBufferManager               *m_yuvCaptureBufferMgr;
    ExynosCameraBufferManager               *m_thumbnailBufferMgr;

    /* internal temporary buffer managers */
    ExynosCameraBufferManager               *m_skipBufferMgr;

    /* service buffer managers */
    ExynosCameraBufferManager               *m_bayerBufferMgr;

    ExynosCameraFrameSelector               *m_captureSelector;
    ExynosCameraFrameSelector               *m_captureZslSelector;
    ExynosCameraFrameSelector               *m_sccCaptureSelector;
    uint32_t                                m_prepareFliteCnt;

private:
    uint32_t                        m_cameraId;
    char                            m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    mutable Mutex                   m_requestLock;
    mutable Mutex                   m_resultLock;
    mutable Condition               m_captureResultDoneCondition;
    mutable Mutex                   m_captureResultDoneLock;
    bool                            m_use_companion;
    int                             m_captureCount;

    nsecs_t                         m_lastFrametime;

    /* HACK : check capture stream */
    bool                            isCaptureConfig;
    bool                            isRestarted;

    /* HACK : check recording stream */
    bool                            isRecordingConfig;
    bool                            recordingEnabled;

    bool                            m_checkConfigStream;

    ExynosCameraFrameManager        *m_frameMgr;
    struct ExynosConfigInfo         *m_exynosconfig;
    struct camera2_shot_ext         *m_currentShot;

    bool                            m_flushWaitEnable;
    bool                            m_factoryStartFlag;
    bool                            m_flushFlag;
    bool                            m_flagStartFrameFactory;
    bool                            m_flagStartReprocessingFrameFactory;
    bool                            m_flagBayerRequest;
#ifdef MONITOR_LOG_SYNC
    static uint32_t                 cameraSyncLogId;
    int                             m_syncLogDuration;
    uint32_t                        m_getSyncLogId(void);
#endif
    /* process queue */
    List<ExynosCameraFrame *>       m_processList;
    mutable Mutex                   m_processLock;
    List<ExynosCameraFrame *>       m_captureProcessList;
    mutable Mutex                   m_captureProcessLock;
    frame_queue_t                   *m_pipeFrameDoneQ[MAX_PIPE_NUM];

    frame_queue_t                   *m_duplicateBufferDoneQ;
    frame_queue_t                   *m_pipeCaptureFrameDoneQ;

    /* capture Queue */
    frame_queue_t                   *m_selectBayerQ;
    frame_queue_t                   *m_captureQ;
    frame_queue_t                   *m_reprocessingDoneQ;

    ExynosCameraList<uint32_t>      *m_shotDoneQ;
    List<request_info_t *>          m_requestWaitingList;

    frame_queue_t                   *m_internalFrameDoneQ;

    int                             m_internalFrameCount;
    bool                            m_isNeedInternalFrame;
    bool                            m_isNeedRequestFrame;

    /* Thread */
    sp<mainCameraThread>            m_mainThread;
    bool                            m_mainThreadFunc(void);

    sp<mainCameraThread>            m_internalFrameThread;
    bool                            m_internalFrameThreadFunc(void);

    sp<mainCameraThread>            m_previewStreamBayerThread;
    bool                            m_previewStreamBayerPipeThreadFunc(void);

    sp<mainCameraThread>            m_previewStream3AAThread;
    bool                            m_previewStream3AAPipeThreadFunc(void);

    sp<mainCameraThread>            m_previewStreamISPThread;
    bool                            m_previewStreamISPPipeThreadFunc(void);

    sp<mainCameraThread>            m_previewStreamVRAThread;
    bool                            m_previewStreamVRAPipeThreadFunc(void);

    sp<mainCameraThread>            m_duplicateBufferThread;
    bool                            m_duplicateBufferThreadFunc(void);

    sp<mainCameraThread>            m_selectBayerThread;
    bool                            m_selectBayerThreadFunc(void);

    sp<mainCameraThread>            m_captureThread;
    bool                            m_captureThreadFunc(void);

    sp<mainCameraThread>            m_captureStreamThread;
    bool                            m_captureStreamThreadFunc(void);

    sp<mainCameraThread>            m_setBuffersThread;
    bool                            m_setBuffersThreadFunc(void);

    sp<mainCameraThread>            m_framefactoryCreateThread;
    bool                            m_frameFactoryCreateThreadFunc(void);

    sp<mainCameraThread>            m_reprocessingFrameFactoryStartThread;
    bool                            m_reprocessingFrameFactoryStartThreadFunc(void);

    sp<mainCameraThread>            m_startPictureBufferThread;
    bool                            m_startPictureBufferThreadFunc(void);

    bool                            m_frameFactoryStartDone;
    sp<mainCameraThread>            m_frameFactoryStartThread;
    bool                            m_frameFactoryStartThreadFunc(void);

    status_t                        m_setupReprocessingPipeline(void);

    sp<mainCameraThread>            m_monitorThread;
    bool                            m_monitorThreadFunc(void);

    status_t                        m_setInternalScpBuffer(void);

    status_t                        m_setPictureBuffer(void);

    status_t                        m_setReprocessingBuffer(void);

    bool                            m_doInternalFrame(ExynosCameraRequest *request);
};

}; /* namespace android */
#endif
