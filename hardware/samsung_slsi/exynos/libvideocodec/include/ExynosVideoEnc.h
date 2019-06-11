/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

#ifndef _EXYNOS_VIDEO_ENC_H_
#define _EXYNOS_VIDEO_ENC_H_

/* Configurable */
/* Normal Node */
#define VIDEO_ENCODER_NAME                  "s5p-mfc-enc"
#define VIDEO_HEVC_ENCODER_NAME             "exynos-hevc-enc"
/* Secure Node */
#define VIDEO_SECURE_ENCODER_NAME           "s5p-mfc-enc-secure"
#define VIDEO_HEVC_SECURE_ENCODER_NAME      "exynos-hevc-enc-secure"

#define VIDEO_ENCODER_DEFAULT_INBUF_PLANES  2
#define VIDEO_ENCODER_DEFAULT_OUTBUF_PLANES 1
#define VIDEO_ENCODER_POLL_TIMEOUT      25

#define FRAME_RATE_CHANGE_THRESH_HOLD 5

typedef struct _ExynosVideoEncContext {
    int                     hEnc;
    ExynosVideoBoolType     bShareInbuf;
    ExynosVideoBoolType     bShareOutbuf;
    ExynosVideoBuffer      *pInbuf;
    ExynosVideoBuffer      *pOutbuf;

    /* FIXME : temp */
    ExynosVideoGeometry     inbufGeometry;
    ExynosVideoGeometry     outbufGeometry;
    int                     nInbufs;
    int                     nInbufPlanes;
    int                     nOutbufs;
    int                     nOutbufPlanes;
    ExynosVideoBoolType     bStreamonInbuf;
    ExynosVideoBoolType     bStreamonOutbuf;
    void                   *pPrivate;
    void                   *pInMutex;
    void                   *pOutMutex;
    ExynosVideoInstInfo     videoInstInfo;

    int                     hIONHandle;
    int                     nTemporalLayerShareBufferFD;
    void                   *pTemporalLayerShareBufferAddr;
    int                     nRoiShareBufferFD;
    void                   *pRoiShareBufferAddr;
    int                     oldFrameRate;
    int64_t                 oldTimeStamp;
    int64_t                 oldDuration;
} ExynosVideoEncContext;

ExynosVideoErrorType MFC_Exynos_Video_GetInstInfo_Encoder(
    ExynosVideoInstInfo *pVideoInstInfo);

int MFC_Exynos_Video_Register_Encoder(
    ExynosVideoEncOps       *pEncOps,
    ExynosVideoEncBufferOps *pInbufOps,
    ExynosVideoEncBufferOps *pOutbufOps);

#endif /* _EXYNOS_VIDEO_ENC_H_ */
