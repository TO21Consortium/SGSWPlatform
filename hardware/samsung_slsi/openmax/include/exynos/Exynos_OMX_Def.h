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

/*
 * @file    Exynos_OMX_Def.h
 * @brief   Exynos_OMX specific define
 * @author  SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef EXYNOS_OMX_DEF
#define EXYNOS_OMX_DEF

//#define PERFORMANCE_DEBUG

#include "OMX_Types.h"
#include "OMX_IVCommon.h"
#include "OMX_Video.h"
#include "OMX_VideoExt.h"
#include "OMX_IndexExt.h"

#define VERSIONMAJOR_NUMBER                1
#define VERSIONMINOR_NUMBER                1
#define REVISION_NUMBER                    2
#define STEP_NUMBER                        0

#define RESOURCE_VIDEO_DEC 16
#define RESOURCE_VIDEO_ENC 16
#define RESOURCE_AUDIO_DEC 10

#define MAX_OMX_COMPONENT_NUM              40
#define MAX_OMX_COMPONENT_ROLE_NUM         10
#define MAX_OMX_COMPONENT_NAME_SIZE        OMX_MAX_STRINGNAME_SIZE
#define MAX_OMX_COMPONENT_ROLE_SIZE        OMX_MAX_STRINGNAME_SIZE
#define MAX_OMX_COMPONENT_LIBNAME_SIZE     OMX_MAX_STRINGNAME_SIZE * 2
#define MAX_OMX_MIMETYPE_SIZE              OMX_MAX_STRINGNAME_SIZE

#define MAX_BUFFER_REF       40
#define MAX_TIMESTAMP        MAX_BUFFER_REF
#define MAX_FLAGS            MAX_BUFFER_REF

#define MAX_BUFFER_PLANE     3

#define INDEX_AFTER_EOS      0xE05

#define DEFAULT_TIMESTAMP_VAL (-1010101010)
#define RESET_TIMESTAMP_VAL   (-1001001001)

// The largest metadata buffer size advertised
// when metadata buffer mode is used
#define  MAX_METADATA_BUFFER_SIZE (64)

#define  PREFIX_COMPONENT_NAME "OMX.Exynos."
#define  IS_CUSTOM_COMPONENT(name) (((char)(name[((int)sizeof(PREFIX_COMPONENT_NAME))-1]) >= 0x61)? OMX_TRUE:OMX_FALSE)

#define IS_64BIT_OS                 (((sizeof(int) != sizeof(void *))? OMX_TRUE:OMX_FALSE))
#define EXYNOS_OMX_INSTALL_PATH     "/system/lib/omx/"
#define EXYNOS_OMX64_INSTALL_PATH   "/system/lib64/omx/"

typedef enum _EXYNOS_CODEC_TYPE
{
    SW_CODEC,
    HW_VIDEO_DEC_CODEC,
    HW_VIDEO_ENC_CODEC,
    HW_VIDEO_DEC_SECURE_CODEC,
    HW_VIDEO_ENC_SECURE_CODEC,
    HW_AUDIO_DEC_CODEC,
    HW_AUDIO_ENC_CODEC
} EXYNOS_CODEC_TYPE;

#define PLANE_MAX_NUM 3
typedef enum _PLANE_TYPE {
    PLANE_MULTIPLE      = 0x00,
    PLANE_SINGLE        = 0x11,
    PLANE_SINGLE_USER   = 0x12,
} PLANE_TYPE;

typedef enum _EXYNOS_OMX_INDEXTYPE
{
#define EXYNOS_INDEX_PARAM_THUMBNAIL "OMX.SEC.index.ThumbnailMode"
    OMX_IndexVendorThumbnailMode                = 0x7F000001,

#define EXYNOS_INDEX_CONFIG_VIDEO_INTRAPERIOD "OMX.SEC.index.VideoIntraPeriod"
    OMX_IndexConfigVideoIntraPeriod             = 0x7F000002,

#ifdef USE_S3D_SUPPORT
#define EXYNOS_INDEX_PARAM_GET_S3D "OMX.SEC.index.S3DMode"
    OMX_IndexVendorS3DMode                      = 0x7F000003,
#endif

#define EXYNOS_INDEX_PARAM_NEED_CONTIG_MEMORY "OMX.SEC.index.NeedContigMemory"
    OMX_IndexVendorNeedContigMemory             = 0x7F000004,

#define EXYNOS_INDEX_CONFIG_GET_BUFFER_FD "OMX.SEC.index.GetBufferFD"
    OMX_IndexVendorGetBufferFD                  = 0x7F000005,

#define EXYNOS_INDEX_PARAM_SET_DTS_MODE "OMX.SEC.index.SetDTSMode"
    OMX_IndexVendorSetDTSMode                   = 0x7F000006,

#define EXYNOS_INDEX_CONFIG_SET_QOS_RATIO "OMX.SEC.index.SetQosRatio"
    OMX_IndexVendorSetQosRatio                  = 0x7F000007,

    /* for Android Native Window */
#define EXYNOS_INDEX_PARAM_ENABLE_ANB "OMX.google.android.index.enableAndroidNativeBuffers"
    OMX_IndexParamEnableAndroidBuffers          = 0x7F000011,

#define EXYNOS_INDEX_PARAM_GET_ANB "OMX.google.android.index.getAndroidNativeBufferUsage"
    OMX_IndexParamGetAndroidNativeBuffer        = 0x7F000012,

#define EXYNOS_INDEX_PARAM_USE_ANB "OMX.google.android.index.useAndroidNativeBuffer"
    OMX_IndexParamUseAndroidNativeBuffer        = 0x7F000013,

    /* for Android Store Metadata Inbuffer */
#define EXYNOS_INDEX_PARAM_STORE_METADATA_BUFFER "OMX.google.android.index.storeMetaDataInBuffers"
    OMX_IndexParamStoreMetaDataBuffer           = 0x7F000014,

    /* prepend SPS/PPS to I/IDR for H.264 Encoder */
#define EXYNOS_INDEX_PARAM_PREPEND_SPSPPS_TO_IDR "OMX.google.android.index.prependSPSPPSToIDRFrames"
    OMX_IndexParamPrependSPSPPSToIDR            = 0x7F000015,

#ifdef USE_KHRONOS_OMX_HEADER
    /* VP8 Codec */
#if 0 /* USE_VP8_SUPPORT : these are declared in OMX_IndexExt.h */
#define EXYNOS_INDEX_PARAM_VIDEO_VP8_TYPE "OMX.SEC.index.VideoVp8Type"
    OMX_IndexParamVideoVp8                      = 0x7F000016,

#define EXYNOS_INDEX_CONFIG_VIDEO_VP8_REFERENCEFRAME "OMX.SEC.index.VideoVp8ReferenceFrame"
    OMX_IndexConfigVideoVp8ReferenceFrame       = 0x7F000017,

#define EXYNOS_INDEX_CONFIG_VIDEO_VP8_REFERENCEFRAMETYPE "OMX.SEC.index.VideoVp8ReferenceFrameType"
    OMX_IndexConfigVideoVp8ReferenceFrameType   = 0x7F000018,
#endif  // USE_VP8_SUPPORT

    /* HEVC Codec */
#ifdef USE_HEVC_SUPPORT
#define EXYNOS_INDEX_PARAM_VIDEO_HEVC_TYPE "OMX.SEC.index.VideoHevcType"
    OMX_IndexParamVideoHevc                     = 0x7F000019,
#endif  // USE_HEVC_SUPPORT

#endif  // USE_KHRONOS_OMX_HEADER

    /* Thumbnail */
#define EXYNOS_INDEX_PARAM_ENABLE_THUMBNAIL "OMX.SEC.index.enableThumbnailMode"
    OMX_IndexParamEnableThumbnailMode           = 0x7F000020,

    /* Android Native Buffer */
#define EXYNOS_INDEX_PARAM_USE_ANB2 "OMX.google.android.index.useAndroidNativeBuffer2"
    OMX_IndexParamUseAndroidNativeBuffer2       = 0x7F000021,

    /* QP control */
#define EXYNOS_INDEX_PARAM_VIDEO_QPRANGE_TYPE "OMX.SEC.indexParam.VideoQPRange"
    OMX_IndexParamVideoQPRange                  = 0x7F000022,

#define EXYNOS_INDEX_CONFIG_VIDEO_QPRANGE_TYPE "OMX.SEC.indexConfig.VideoQPRange"
    OMX_IndexConfigVideoQPRange                 = 0x7F000023,

#ifdef USE_KHRONOS_OMX_HEADER
    /* VP8 Codec */
#ifdef USE_VP8_SUPPORT
#define EXYNOS_INDEX_PARAM_VIDEO_ANDROID_VP8_ENCODER "OMX.SEC.index.VideoAndroidVP8Encoder"
    OMX_IndexParamVideoAndroidVp8Encoder        = 0x7F000024,
#endif  // USE_VP8_SUPPORT

#define EXYNOS_INDEX_PARAM_SLICE_SEGMENTS "OMX.SEC.index.SliceSegments"
    OMX_IndexParamSliceSegments                 = 0x7F000025,

#endif  // USE_KHRONOS_OMX_HEADER

    /* Temporal SVC */
#define EXYNOS_INDEX_CONFIG_VIDEO_TEMPORALSVC "OMX.SEC.index.TemporalSVC"
    OMX_IndexConfigVideoTemporalSVC             = 0x7F000026,

#define EXYNOS_INDEX_PARAM_VIDEO_AVC_ENABLE_TEMPORALSVC "OMX.SEC.index.AVC.enableTemporalSVC"
    OMX_IndexParamVideoAVCEnableTemporalSVC     = 0x7F000027,

    /* VC1 Codec */
#ifndef USE_KHRONOS_OMX_1_2
#define EXYNOS_INDEX_PARAM_VIDEO_VC1_TYPE "OMX.SEC.index.VideoVc1Type"
    OMX_IndexParamVideoVC1                      = 0x7F000028,
#endif

//#ifdef USE_KHRONOS_OMX_HEADER
    /* VP9 Codec */
#ifdef USE_VP9_SUPPORT
#define EXYNOS_INDEX_PARAM_VIDEO_VP9_TYPE "OMX.SEC.index.VideoVp9Type"
    OMX_IndexParamVideoVp9                      = 0x7F000029,

#define EXYNOS_INDEX_CONFIG_VIDEO_VP9_REFERENCEFRAME "OMX.SEC.index.VideoVp9ReferenceFrame"
    OMX_IndexConfigVideoVp9ReferenceFrame       = 0x7F000030,

#define EXYNOS_INDEX_CONFIG_VIDEO_VP9_REFERENCEFRAMETYPE "OMX.SEC.index.VideoVp9ReferenceFrameType"
    OMX_IndexConfigVideoVp9ReferenceFrameType   = 0x7F000031,

#define EXYNOS_INDEX_PARAM_VIDEO_ANDROID_VP9_ENCODER "OMX.SEC.index.VideoAndroidVP9Encoder"
    OMX_IndexParamVideoAndroidVp9Encoder        = 0x7F000032,
#endif  // USE_VP9_SUPPORT
//#endif  // USE_KHRONOS_OMX_HEADER

    /* blur filtering */
#define EXYNOS_INDEX_PARAM_ENABLE_BLUR_FILTER "OMX.SEC.indexParam.enableBlurFilter"
    OMX_IndexParamEnableBlurFilter              = 0x7F000033,
#define EXYNOS_INDEX_CONFIG_BLUR_INFO "OMX.SEC.indexConfig.BlurInfo"
    OMX_IndexConfigBlurInfo                     = 0x7F000034,

#define EXYNOS_INDEX_PARAM_VIDEO_HEVC_ENABLE_TEMPORALSVC "OMX.SEC.index.Hevc.enableTemporalSVC"
    OMX_IndexParamVideoHevcEnableTemporalSVC    = 0x7F000035,

    /* ROI Information */
#define EXYNOS_INDEX_CONFIG_VIDEO_ROIINFO "OMX.SEC.index.RoiInfo"
    OMX_IndexConfigVideoRoiInfo                 = 0x7F000036,
#define EXYNOS_INDEX_PARAM_VIDEO_ENABLE_ROIINFO "OMX.SEC.index.enableRoiInfo"
    OMX_IndexParamVideoEnableRoiInfo            = 0x7F000037,

    /* rotation about encoder's input  */
#define EXYNOS_INDEX_PARAM_ROATION_INFO "OMX.SEC.indexParam.RotationInfo"
    OMX_IndexParamRotationInfo                  = 0x7F000038,

#define EXYNOS_INDEX_CONFIG_OPERATING_RATE "OMX.SEC.index.OperatingRate"
#ifdef USE_KHRONOS_OMX_HEADER
    OMX_IndexConfigOperatingRate                = 0x7F000039,
#endif

    /* for custom component(MSRND) */
    OMX_IndexExynosStartUnused              = 0x7F050000, /* Reserved region for Exynos Extensions */

#define EXYNOS_CUSTOM_INDEX_CONFIG_PTS_MODE "OMX.SEC.CUSTOM.index.PTSMode"
    OMX_IndexExynosConfigPTSMode            = 0x7F050001, /* to support PTS Video TimeStamp - OMX_BOOL */
#define EXYNOS_CUSTOM_INDEX_CONFIG_DISPLAY_DELAY "OMX.SEC.CUSTOM.index.DisplayDelay"
    OMX_IndexExynosConfigDisplayDelay       = 0x7F050002, /* to support H264 DisplayDelay Setting - OMX_U32 */
    /* Timestamp reorder */
#define EXYNOS_CUSTOM_INDEX_PARAM_REORDER_MODE "OMX.SEC.CUSTOM.index.ReorderMode"
    OMX_IndexExynosParamReorderMode         = 0x7F050003,
#define EXYNOS_CUSTOM_INDEX_PARAM_CORRUPTEDHEADER "OMX.SEC.CUSTOM.index.CorruptedHeader"
    OMX_IndexExynosParamCorruptedHeader     = 0x7F050004,


    /* for Skype HD Common */
#define OMX_MS_SKYPE_PARAM_DRIVERVER "OMX.microsoft.skype.index.driverversion"
    OMX_IndexSkypeParamDriverVersion        = 0x7F060001,
    /* for Skype HD Decode */
#define OMX_MS_SKYPE_PARAM_DECODERSETTING "OMX.microsoft.skype.index.decodersetting"
    OMX_IndexSkypeParamDecoderSetting       = 0x7F060002,
#define OMX_MS_SKYPE_PARAM_DECODERCAP "OMX.microsoft.skype.index.decodercapability"
    OMX_IndexSkypeParamDecoderCapability    = 0x7F060003,
    /* for Skype HD Encode */
#define OMX_MS_SKYPE_PARAM_ENCODERSETTING "OMX.microsoft.skype.index.encodersetting"
    OMX_IndexSkypeParamEncoderSetting       = 0x7F060004,
#define OMX_MS_SKYPE_PARAM_ENCODERCAP "OMX.microsoft.skype.index.encodercapability"
    OMX_IndexSkypeParamEncoderCapability    = 0x7F060005,
#define OMX_MS_SKYPE_CONFIG_MARKLTRFRAME "OMX.microsoft.skype.index.markltrframe"
    OMX_IndexSkypeConfigMarkLTRFrame        = 0x7F060006,
#define OMX_MS_SKYPE_CONFIG_USELTRFRAME "OMX.microsoft.skype.index.useltrframe"
    OMX_IndexSkypeConfigUseLTRFrame         = 0x7F060007,
#define OMX_MS_SKYPE_CONFIG_QP "OMX.microsoft.skype.index.qp"
    OMX_IndexSkypeConfigQP                  = 0x7F060008,
#define OMX_MS_SKYPE_CONFIG_TEMPORALLAYERCOUNT "OMX.microsoft.skype.index.temporallayercount"
    OMX_IndexSkypeConfigTemporalLayerCount  = 0x7F060009,
#define OMX_MS_SKYPE_CONFIG_BASELAYERPID "OMX.microsoft.skype.index.basepid"
    OMX_IndexSkypeConfigBasePid             = 0x7F06000a,

    OMX_IndexExynosEndUnused                = 0x7F05FFFF,
} EXYNOS_OMX_INDEXTYPE;

typedef enum _EXYNOS_OMX_ERRORTYPE
{
    OMX_ErrorNoEOF              = (OMX_S32) 0x90000001,
    OMX_ErrorInputDataDecodeYet = (OMX_S32) 0x90000002,
    OMX_ErrorInputDataEncodeYet = (OMX_S32) 0x90000003,
    OMX_ErrorCodecInit          = (OMX_S32) 0x90000004,
    OMX_ErrorCodecDecode        = (OMX_S32) 0x90000005,
    OMX_ErrorCodecEncode        = (OMX_S32) 0x90000006,
    OMX_ErrorCodecFlush         = (OMX_S32) 0x90000007,
    OMX_ErrorOutputBufferUseYet = (OMX_S32) 0x90000008,
    OMX_ErrorCorruptedFrame     = (OMX_S32) 0x90000009,
    OMX_ErrorNeedNextHeaderInfo = (OMX_S32) 0x90000010,
    OMX_ErrorNoneSrcSetupFinish = (OMX_S32) 0x90000011,
    OMX_ErrorCorruptedHeader    = (OMX_S32) 0x90000012,
    OMX_ErrorNoneExpiration     = (OMX_S32) 0x90000013,
} EXYNOS_OMX_ERRORTYPE;

typedef enum _EXYNOS_OMX_COMMANDTYPE
{
    EXYNOS_OMX_CommandComponentDeInit = 0x7F000001,
    EXYNOS_OMX_CommandEmptyBuffer,
    EXYNOS_OMX_CommandFillBuffer,
    EXYNOS_OMX_CommandFakeBuffer
} EXYNOS_OMX_COMMANDTYPE;

typedef enum _EXYNOS_OMX_TRANS_STATETYPE {
    EXYNOS_OMX_TransStateInvalid,
    EXYNOS_OMX_TransStateLoadedToIdle,
    EXYNOS_OMX_TransStateIdleToExecuting,
    EXYNOS_OMX_TransStateExecutingToIdle,
    EXYNOS_OMX_TransStateIdleToLoaded,
    EXYNOS_OMX_TransStateMax = 0X7FFFFFFF
} EXYNOS_OMX_TRANS_STATETYPE;

typedef enum _EXYNOS_OMX_COLOR_FORMATTYPE {
    OMX_SEC_COLOR_FormatNV12TPhysicalAddress        = 0x7F000001, /**< Reserved region for introducing Vendor Extensions */
    OMX_SEC_COLOR_FormatNV12LPhysicalAddress        = 0x7F000002,
    OMX_SEC_COLOR_FormatNV12LVirtualAddress         = 0x7F000003,
#ifdef USE_KHRONOS_OMX_HEADER
    OMX_SEC_COLOR_FormatNV12Tiled                   = 0x7FC00002,  /* 0x7FC00002 */
#endif
    OMX_SEC_COLOR_FormatNV21LPhysicalAddress        = 0x7F000010,
    OMX_SEC_COLOR_FormatNV21Linear                  = 0x7F000011,
    OMX_SEC_COLOR_FormatYVU420Planar                = 0x7F000012,
    OMX_SEC_COLOR_Format32bitABGR8888               = 0x7F000013,  /* unused */
    OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace   = 0x7F000014,
    OMX_SEC_COLOR_Format10bitYUV420SemiPlanar       = 0x7F000015,

    /* to copy a encoded data for drm component using gsc or fimc */
    OMX_SEC_COLOR_FormatEncodedData                 = OMX_COLOR_FormatYCbYCr,
#ifdef USE_KHRONOS_OMX_HEADER
    /* for Android SurfaceMediaSource*/
    OMX_COLOR_FormatAndroidOpaque                   = 0x7F000789,
    OMX_COLOR_Format32BitRGBA8888                   = 0x7F00A000,
#endif
}EXYNOS_OMX_COLOR_FORMATTYPE;

typedef enum _EXYNOS_OMX_SUPPORTFORMAT_TYPE
{
    supportFormat_0 = 0x00,
    supportFormat_1,
    supportFormat_2,
    supportFormat_3,
    supportFormat_4,
    supportFormat_5,
    supportFormat_6,
    supportFormat_7,
} EXYNOS_OMX_SUPPORTFORMAT_TYPE;

typedef enum _EXYNOS_OMX_BUFFERPROCESS_TYPE
{
    BUFFER_DEFAULT        = 0x00,
    BUFFER_COPY           = 0x01,
    BUFFER_SHARE          = 0x02,
    BUFFER_METADATA       = 0x04,
    BUFFER_ANBSHARE       = 0x08,
} EXYNOS_OMX_BUFFERPROCESS_TYPE;

#ifdef USE_S3D_SUPPORT
typedef enum _EXYNOS_OMX_FPARGMT_TYPE
{
    OMX_SEC_FPARGMT_INVALID           = -1,
    OMX_SEC_FPARGMT_CHECKERBRD_INTERL = 0x00,
    OMX_SEC_FPARGMT_COLUMN_INTERL     = 0x01,
    OMX_SEC_FPARGMT_ROW_INTERL        = 0x02,
    OMX_SEC_FPARGMT_SIDE_BY_SIDE      = 0x03,
    OMX_SEC_FPARGMT_TOP_BOTTOM        = 0x04,
    OMX_SEC_FPARGMT_TEMPORAL_INTERL   = 0x05,
    OMX_SEC_FPARGMT_NONE              = 0x0A
} EXYNOS_OMX_FPARGMT_TYPE;
#endif

typedef enum _EXYNOS_OMX_EVENTTYPE
{
   OMX_EventVendorStart    = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
#ifdef USE_S3D_SUPPORT
   OMX_EventS3DInformation,
#endif
} EXYNOS_OMX_EVENTTYPE;

typedef enum _EXYNOS_ANB_TYPE {
    NATIVE_GRAPHIC_BUFFER1,
    NATIVE_GRAPHIC_BUFFER2,
} EXYNOS_ANB_TYPE;

typedef enum _EXYNOS_OMX_VIDEO_CONTROLRATETYPE {
    OMX_Video_ControlRateVendorStart    = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_Video_ControlRateConstantVTCall = 0x7F000001,
} EXYNOS_OMX_VIDEO_CONTROLRATETYPE;

typedef struct _EXYNOS_OMX_PRIORITYMGMTTYPE
{
    OMX_U32 nGroupPriority; /* the value 0 represents the highest priority */
                            /* for a group of components                   */
    OMX_U32 nGroupID;
} EXYNOS_OMX_PRIORITYMGMTTYPE;

typedef struct _EXYNOS_OMX_VIDEO_PROFILELEVEL
{
    OMX_S32  profile;
    OMX_S32  level;
} EXYNOS_OMX_VIDEO_PROFILELEVEL;

typedef struct _EXYNOS_OMX_VIDEO_THUMBNAILMODE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bEnable;
} EXYNOS_OMX_VIDEO_THUMBNAILMODE;

typedef struct _EXYNOS_OMX_VIDEO_PARAM_PORTMEMTYPE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bNeedContigMem;
} EXYNOS_OMX_VIDEO_PARAM_PORTMEMTYPE;

typedef struct _EXYNOS_OMX_VIDEO_PARAM_DTSMODE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL        bDTSMode;
} EXYNOS_OMX_VIDEO_PARAM_DTSMODE;

typedef struct _EXYNOS_OMX_VIDEO_PARAM_REORDERMODE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL        bReorderMode;
} EXYNOS_OMX_VIDEO_PARAM_REORDERMODE;

typedef struct _EXYNOS_OMX_VIDEO_PARAM_CORRUPTEDHEADER {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL        bDiscardEvent;
} EXYNOS_OMX_VIDEO_PARAM_CORRUPTEDHEADER;

typedef struct _EXYNOS_OMX_VIDEO_CONFIG_BUFFERINFO {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_PTR OMX_IN  pVirAddr;
    OMX_S32 OMX_OUT fd;
} EXYNOS_OMX_VIDEO_CONFIG_BUFFERINFO;

typedef struct _EXYNOS_OMX_VIDEO_CONFIG_QOSINFO {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nQosRatio;
} EXYNOS_OMX_VIDEO_CONFIG_QOSINFO;

typedef struct OMX_VIDEO_QPRANGE {
    OMX_U32 nMinQP;
    OMX_U32 nMaxQP;
} OMX_VIDEO_QPRANGE;

typedef struct OMX_VIDEO_QPRANGETYPE {
    OMX_U32             nSize;
    OMX_VERSIONTYPE     nVersion;
    OMX_U32             nPortIndex;
    OMX_VIDEO_QPRANGE   qpRangeI;
    OMX_VIDEO_QPRANGE   qpRangeP;
    OMX_VIDEO_QPRANGE   qpRangeB;  /* H.264, HEVC, MPEG4 */
} OMX_VIDEO_QPRANGETYPE;

/* Temporal SVC */
/* Maximum number of temporal layers */
#define OMX_VIDEO_MAX_TEMPORAL_LAYERS 7
#define OMX_VIDEO_MAX_TEMPORAL_LAYERS_WITH_LTR 3
#define OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS 7
#define OMX_VIDEO_ANDROID_MAXHEVCTEMPORALLAYERS 7

typedef struct _EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bEnableTemporalSVC;
} EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC;

typedef struct _EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_U32         nKeyFrameInterval;
    OMX_U32         nTemporalLayerCount;
    OMX_U32         nTemporalLayerBitrateRatio[OMX_VIDEO_MAX_TEMPORAL_LAYERS];
    OMX_U32         nMinQuantizer;
    OMX_U32         nMaxQuantizer;
} EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC;

typedef enum _EXYNOS_OMX_BLUR_MODE
{
    BLUR_MODE_NONE          = 0x00,
    BLUR_MODE_DOWNUP        = 0x01,
    BLUR_MODE_COEFFICIENT   = 0x02,
} EXYNOS_OMX_BLUR_MODE;

typedef enum _EXYNOS_OMX_BLUR_RESOL
{
    BLUR_RESOL_240     = 426 * 240,   /* 426 x 240 */
    BLUR_RESOL_480     = 854 * 480,   /* 854 x 480 */
    BLUR_RESOL_720     = 1280 * 720,  /* 1280 x 720 */
    BLUR_RESOL_1080    = 1920 * 1080, /* 1920 x 1080 */
} EXYNOS_OMX_BLUR_RESOL;

typedef struct _EXYNOS_OMX_VIDEO_PARAM_ENABLE_BLURFILTER {
    OMX_U32                 nSize;
    OMX_VERSIONTYPE         nVersion;
    OMX_U32                 nPortIndex;
    OMX_BOOL                bUseBlurFilter;
} EXYNOS_OMX_VIDEO_PARAM_ENABLE_BLURFILTER;

typedef struct _EXYNOS_OMX_VIDEO_CONFIG_BLURINFO {
    OMX_U32                 nSize;
    OMX_VERSIONTYPE         nVersion;
    OMX_U32                 nPortIndex;
    EXYNOS_OMX_BLUR_MODE    eBlurMode;
    EXYNOS_OMX_BLUR_RESOL   eTargetResol;
} EXYNOS_OMX_VIDEO_CONFIG_BLURINFO;
/* ROI Information */

typedef struct _EXYNOS_OMX_VIDEO_CONFIG_ROIINFO {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_S32         nUpperQpOffset;
    OMX_S32         nLowerQpOffset;
    OMX_BOOL        bUseRoiInfo;
    OMX_S32         nRoiMBInfoSize;
    OMX_PTR         pRoiMBInfo;
} EXYNOS_OMX_VIDEO_CONFIG_ROIINFO;

typedef struct _EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bEnableRoiInfo;
} EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO;


typedef enum _EXYNOS_OMX_ROTATION_TYPE
{
    ROTATE_0        = 0,
    ROTATE_90       = 90,
    ROTATE_180      = 180,
    ROTATE_270      = 270,
} EXYNOS_OMX_ROTATION_TYPE;

typedef struct _EXYNOS_OMX_VIDEO_PARAM_ROTATION_INFO {
    OMX_U32                     nSize;
    OMX_VERSIONTYPE             nVersion;
    OMX_U32                     nPortIndex;
    EXYNOS_OMX_ROTATION_TYPE    eRotationType;
} EXYNOS_OMX_VIDEO_PARAM_ROTATION_INFO;

/* for AVC */
typedef enum EXYNOS_OMX_VIDEO_AVCPROFILETYPE {
    OMX_VIDEO_AVCProfileConstrainedBaseline = 0x7F000001,
    OMX_VIDEO_AVCProfileConstrainedHigh     = 0x7F000002,
} EXYNOS_OMX_VIDEO_AVCPROFILETYPE;

#ifdef USE_KHRONOS_OMX_HEADER
typedef enum EXYNOS_OMX_VIDEO_AVCLEVELTYPE {
    OMX_VIDEO_AVCLevel52  = 0x10000,  /**< Level 5.2 */
} EXYNOS_OMX_VIDEO_AVCLEVELTYPE;
#endif

/* for Ext-Codec support */
typedef enum EXYNOS_OMX_VIDEO_CODINGTYPE {
#ifdef USE_KHRONOS_OMX_HEADER
    OMX_VIDEO_CodingHEVC       = 0x7F000002,   /**< ITU H.265/HEVC */
    OMX_VIDEO_CodingVP9        = 0x7F000003,   /**< Google VP9 */
#endif
    OMX_VIDEO_VendorCodingMAX  = 0x7FFFFFFF,
} EXYNOS_OMX_VIDEO_CODINGTYPE;

typedef enum _EXYNOS_OMX_HIERARCHICAL_CODING_TYPE
{
    EXYNOS_OMX_Hierarchical_P = 0x00,
    EXYNOS_OMX_Hierarchical_B,
} EXYNOS_OMX_HIERARCHICAL_CODING_TYPE;

#ifdef USE_KHRONOS_OMX_HEADER

#if defined(USE_VP8_SUPPORT) || defined(USE_VP9_SUPPORT)
/** VP8 temporal layer patterns */
typedef enum OMX_VIDEO_ANDROID_VPXTEMPORALLAYERPATTERNTYPE {
    OMX_VIDEO_VPXTemporalLayerPatternNone   = 0,
    OMX_VIDEO_VPXTemporalLayerPatternWebRTC = 1,
    OMX_VIDEO_VPXTemporalLayerPatternMax    = 0x7FFFFFFF
} OMX_VIDEO_ANDROID_VPXTEMPORALLAYERPATTERNTYPE;
#endif

    /* VP8 Codec */
#ifdef USE_VP8_SUPPORT
/** Maximum number of VP8 temporal layers */
#define OMX_VIDEO_ANDROID_MAXVP8TEMPORALLAYERS 3

/**
 * Android specific VP8 encoder params
 *
 * STRUCT MEMBERS:
 *  nSize                      : Size of the structure in bytes
 *  nVersion                   : OMX specification version information
 *  nPortIndex                 : Port that this structure applies to
 *  nKeyFrameInterval          : Key frame interval in frames
 *  eTemporalPattern           : Type of temporal layer pattern
 *  nTemporalLayerCount        : Number of temporal coding layers
 *  nTemporalLayerBitrateRatio : Bitrate ratio allocation between temporal
 *                               streams in percentage
 *  nMinQuantizer              : Minimum (best quality) quantizer
 *  nMaxQuantizer              : Maximum (worst quality) quantizer
 */
typedef struct OMX_VIDEO_PARAM_ANDROID_VP8ENCODERTYPE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_U32         nKeyFrameInterval;
    OMX_VIDEO_ANDROID_VPXTEMPORALLAYERPATTERNTYPE eTemporalPattern;
    OMX_U32         nTemporalLayerCount;
    OMX_U32         nTemporalLayerBitrateRatio[OMX_VIDEO_ANDROID_MAXVP8TEMPORALLAYERS];
    OMX_U32         nMinQuantizer;
    OMX_U32         nMaxQuantizer;
} OMX_VIDEO_PARAM_ANDROID_VP8ENCODERTYPE;
#endif  // USE_VP8_SUPPORT

    /* HEVC codec */
#ifdef USE_HEVC_SUPPORT
/** HEVC Profile enum type */
typedef enum OMX_VIDEO_HEVCPROFILETYPE {
    OMX_VIDEO_HEVCProfileUnknown           = 0x0,
    OMX_VIDEO_HEVCProfileMain              = 0x01,          /**< Main profile */
    OMX_VIDEO_HEVCProfileMain10            = 0x02,          /**< Main 10 profile */
    OMX_VIDEO_HEVCProfileMainStillPicture  = 0x04,          /**< Main Still Picture */
    OMX_VIDEO_HEVCProfileKhronosExtensions = 0x6F000000,    /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_VIDEO_HEVCProfileVendorStartUnused = 0x7F000000,    /**< Reserved region for introducing Vendor Extensions */
    OMX_VIDEO_HEVCProfileMax               = 0x7FFFFFFF
} OMX_VIDEO_HEVCPROFILETYPE;

/** HEVC Level enum type */
typedef enum OMX_VIDEO_HEVCLEVELTYPE {
    OMX_VIDEO_HEVCLevelUnknown              = 0x00000000,
    OMX_VIDEO_HEVCMainTierLevel1            = 0x00000001,   /**< Level 1 */
    OMX_VIDEO_HEVCHighTierLevel1            = 0x00000002,
    OMX_VIDEO_HEVCMainTierLevel2            = 0x00000004,   /**< Level 2 */
    OMX_VIDEO_HEVCHighTierLevel2            = 0x00000008,
    OMX_VIDEO_HEVCMainTierLevel21           = 0x00000010,   /**< Level 2.1 */
    OMX_VIDEO_HEVCHighTierLevel21           = 0x00000020,
    OMX_VIDEO_HEVCMainTierLevel3            = 0x00000040,   /**< Level 3 */
    OMX_VIDEO_HEVCHighTierLevel3            = 0x00000080,
    OMX_VIDEO_HEVCMainTierLevel31           = 0x00000100,   /**< Level 3.1 */
    OMX_VIDEO_HEVCHighTierLevel31           = 0x00000200,
    OMX_VIDEO_HEVCMainTierLevel4            = 0x00000400,   /**< Level 4 */
    OMX_VIDEO_HEVCHighTierLevel4            = 0x00000800,
    OMX_VIDEO_HEVCMainTierLevel41           = 0x00001000,   /**< Level 4.1 */
    OMX_VIDEO_HEVCHighTierLevel41           = 0x00002000,
    OMX_VIDEO_HEVCMainTierLevel5            = 0x00004000,   /**< Level 5 */
    OMX_VIDEO_HEVCHighTierLevel5            = 0x00008000,
    OMX_VIDEO_HEVCMainTierLevel51           = 0x00010000,   /**< Level 5.1 */
    OMX_VIDEO_HEVCHighTierLevel51           = 0x00020000,
    OMX_VIDEO_HEVCMainTierLevel52           = 0x00040000,   /**< Level 5.2 */
    OMX_VIDEO_HEVCHighTierLevel52           = 0x00080000,
    OMX_VIDEO_HEVCMainTierLevel6            = 0x00100000,   /**< Level 6 */
    OMX_VIDEO_HEVCHighTierLevel6            = 0x00200000,
    OMX_VIDEO_HEVCMainTierLevel61           = 0x00400000,   /**< Level 6.1 */
    OMX_VIDEO_HEVCHighTierLevel61           = 0x00800000,
    OMX_VIDEO_HEVCMainTierLevel62           = 0x01000000,   /**< Level 6.2 */
    OMX_VIDEO_HEVCHighTierLevel62           = 0x02000000,
    OMX_VIDEO_HEVCLevelKhronosExtensions    = 0x6F000000,   /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_VIDEO_HEVCLevelVendorStartUnused    = 0x7F000000,   /**< Reserved region for introducing Vendor Extensions */
    OMX_VIDEO_HEVCHighMAX                   = 0x7FFFFFFF
} OMX_VIDEO_HEVCLEVELTYPE;

/** Structure for controlling HEVC video encoding and decoding */
typedef struct OMX_VIDEO_PARAM_HEVCTYPE {
    OMX_U32                     nSize;
    OMX_VERSIONTYPE             nVersion;
    OMX_U32                     nPortIndex;
    OMX_VIDEO_HEVCPROFILETYPE   eProfile;
    OMX_VIDEO_HEVCLEVELTYPE     eLevel;
} OMX_VIDEO_PARAM_HEVCTYPE;

#endif  // USE_HEVC_SUPPORT

/** Structure to define if dependent slice segments should be used */
typedef struct OMX_VIDEO_SLICESEGMENTSTYPE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bDepedentSegments;
    OMX_BOOL        bEnableLoopFilterAcrossSlices;
} OMX_VIDEO_SLICESEGMENTSTYPE;

#endif  // USE_KHRONOS_OMX_HEADER

//#ifdef USE_KHRONOS_OMX_HEADER
    /* VP9 Codec */
#ifdef USE_VP9_SUPPORT
    /* VP9 profiles */
typedef enum OMX_VIDEO_VP9PROFILETYPE {
    OMX_VIDEO_VP9ProfileMain                = 0x00000001,
    OMX_VIDEO_VP9ProfileUnknown             = 0x6EFFFFFF,
    OMX_VIDEO_VP9ProfileKhronosExtensions   = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_VIDEO_VP9ProfileVendorStartUnused   = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_VIDEO_VP9ProfileMax                 = 0x7FFFFFFF
} OMX_VIDEO_VP9PROFILETYPE;

    /* VP9 levels */
typedef enum OMX_VIDEO_VP9LEVELTYPE {
    OMX_VIDEO_VP9Level_Version0             = 0x00000001,
    OMX_VIDEO_VP9LevelUnknown               = 0x6EFFFFFF,
    OMX_VIDEO_VP9LevelKhronosExtensions     = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_VIDEO_VP9LevelVendorStartUnused     = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_VIDEO_VP9LevelMax                   = 0x7FFFFFFF
} OMX_VIDEO_VP9LEVELTYPE;

    /* VP9 param */
typedef struct OMX_VIDEO_PARAM_VP9TYPE {
    OMX_U32                     nSize;
    OMX_VERSIONTYPE             nVersion;
    OMX_U32                     nPortIndex;
    OMX_VIDEO_VP9PROFILETYPE    eProfile;
    OMX_VIDEO_VP9LEVELTYPE      eLevel;
    OMX_BOOL                    bErrorResilientMode;
} OMX_VIDEO_PARAM_VP9TYPE;

    /* VP9 reference frames */
typedef struct OMX_VIDEO_VP9REFERENCEFRAMETYPE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        nPreviousFrameRefresh;
    OMX_BOOL        bGoldenFrameRefresh;
    OMX_BOOL        bAlternateFrameRefresh;
    OMX_BOOL        bUsePreviousFrame;
    OMX_BOOL        bUseGoldenFrame;
    OMX_BOOL        bUseAlternateFrame;
} OMX_VIDEO_VP9REFERENCEFRAMETYPE;

    /* VP9 reference frame type */
typedef struct OMX_VIDEO_VP9REFERENCEFRAMEINFOTYPE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bIsIntraFrame;
    OMX_BOOL        bIsGoldenOrAlternateFrame;
} OMX_VIDEO_VP9REFERENCEFRAMEINFOTYPE;

/** Maximum number of VP9 temporal layers */
#define OMX_VIDEO_ANDROID_MAXVP9TEMPORALLAYERS 3

/**
 * Android specific VP9 encoder params
 *
 * STRUCT MEMBERS:
 *  nSize                      : Size of the structure in bytes
 *  nVersion                   : OMX specification version information
 *  nPortIndex                 : Port that this structure applies to
 *  nKeyFrameInterval          : Key frame interval in frames
 *  eTemporalPattern           : Type of temporal layer pattern
 *  nTemporalLayerCount        : Number of temporal coding layers
 *  nTemporalLayerBitrateRatio : Bitrate ratio allocation between temporal
 *                               streams in percentage
 *  nMinQuantizer              : Minimum (best quality) quantizer
 *  nMaxQuantizer              : Maximum (worst quality) quantizer
 */
typedef struct OMX_VIDEO_PARAM_ANDROID_VP9ENCODERTYPE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_U32         nKeyFrameInterval;
    OMX_VIDEO_ANDROID_VPXTEMPORALLAYERPATTERNTYPE eTemporalPattern;
    OMX_U32         nTemporalLayerCount;
    OMX_U32         nTemporalLayerBitrateRatio[OMX_VIDEO_ANDROID_MAXVP9TEMPORALLAYERS];
    OMX_U32         nMinQuantizer;
    OMX_U32         nMaxQuantizer;
} OMX_VIDEO_PARAM_ANDROID_VP9ENCODERTYPE;
#endif  // USE_VP9_SUPPORT
//#endif  // USE_KHRONOS_OMX_HEADER

#ifndef USE_KHRONOS_OMX_1_2
/* WMV codec */
/** WMV Profile enum type */
typedef enum OMX_VIDEO_WMVPROFILETYPE {
    OMX_VIDEO_WMVProfileSimple = 0,
    OMX_VIDEO_WMVProfileMain,
    OMX_VIDEO_WMVProfileAdvanced,
    OMX_VIDEO_WMVProfileUnknown           = 0x6EFFFFFF,
    OMX_VIDEO_WMVProfileKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_VIDEO_WMVProfileVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
} OMX_VIDEO_WMVPROFILETYPE;

/** WMV Level enum type */
typedef enum OMX_VIDEO_WMVLEVELTYPE {
    OMX_VIDEO_WMVLevelLow = 0,
    OMX_VIDEO_WMVLevelMedium,
    OMX_VIDEO_WMVLevelHigh,
    OMX_VIDEO_WMVLevelL0,
    OMX_VIDEO_WMVLevelL1,
    OMX_VIDEO_WMVLevelL2,
    OMX_VIDEO_WMVLevelL3,
    OMX_VIDEO_WMVLevelL4,
    OMX_VIDEO_WMVLevelUnknown           = 0x6EFFFFFF,
    OMX_VIDEO_WMVLevelKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_VIDEO_WMVLevelVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
} OMX_VIDEO_WMVLEVELTYPE;

/* VC1 codec */
/** VC1 Profile enum type */
typedef enum OMX_VIDEO_VC1PROFILETYPE {
    OMX_VIDEO_VC1ProfileUnused = 0,
    OMX_VIDEO_VC1ProfileSimple,
    OMX_VIDEO_VC1ProfileMain,
    OMX_VIDEO_VC1ProfileAdvanced,
    OMX_VIDEO_VC1ProfileUnknown           = 0x6EFFFFFF,
    OMX_VIDEO_VC1ProfileKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_VIDEO_VC1ProfileVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_VIDEO_VC1ProfileMax
} OMX_VIDEO_VC1PROFILETYPE;

/** VC1 Level enum type */
typedef enum OMX_VIDEO_VC1LEVELTYPE {
    OMX_VIDEO_VC1LevelUnused = 0,
    OMX_VIDEO_VC1LevelLow,
    OMX_VIDEO_VC1LevelMedium,
    OMX_VIDEO_VC1LevelHigh,
    OMX_VIDEO_VC1Level0,
    OMX_VIDEO_VC1Level1,
    OMX_VIDEO_VC1Level2,
    OMX_VIDEO_VC1Level3,
    OMX_VIDEO_VC1Level4,
    OMX_VIDEO_VC1LevelUnknown           = 0x6EFFFFFF,
    OMX_VIDEO_VC1LevelKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_VIDEO_VC1LevelVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_VIDEO_VC1LevelMax
} OMX_VIDEO_VC1LEVELTYPE;

/** Structure for controlling VC1 video encoding and decoding */
typedef struct OMX_VIDEO_PARAM_VC1TYPE {
    OMX_U32                     nSize;
    OMX_VERSIONTYPE             nVersion;
    OMX_U32                     nPortIndex;
    OMX_VIDEO_VC1PROFILETYPE    eProfile;
    OMX_VIDEO_VC1LEVELTYPE      eLevel;
} OMX_VIDEO_PARAM_VC1TYPE;
#endif

#ifndef __OMX_EXPORTS
#define __OMX_EXPORTS
#define EXYNOS_EXPORT_REF __attribute__((visibility("default")))
#define EXYNOS_IMPORT_REF __attribute__((visibility("default")))
#endif

#endif
