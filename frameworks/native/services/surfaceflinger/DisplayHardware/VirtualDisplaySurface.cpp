/*
 * Copyright 2013 The Android Open Source Project
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

// #define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "VirtualDisplaySurface"
#include "VirtualDisplaySurface.h"
#include "HWComposer.h"
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
#include "ExynosHWCService.h"
#endif

#include <gui/BufferItem.h>

// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

#if defined(FORCE_HWC_COPY_FOR_VIRTUAL_DISPLAYS)
static const bool sForceHwcCopy = true;
#else
static const bool sForceHwcCopy = false;
#endif
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
#define SCRATCH_BQ_BUF_COUNT    2
#endif

#define VDS_LOGE(msg, ...) ALOGE("[%s] " msg, \
        mDisplayName.string(), ##__VA_ARGS__)
#define VDS_LOGW_IF(cond, msg, ...) ALOGW_IF(cond, "[%s] " msg, \
        mDisplayName.string(), ##__VA_ARGS__)
#define VDS_LOGV(msg, ...) ALOGV("[%s] " msg, \
        mDisplayName.string(), ##__VA_ARGS__)

static const char* dbgCompositionTypeStr(DisplaySurface::CompositionType type) {
    switch (type) {
        case DisplaySurface::COMPOSITION_UNKNOWN: return "UNKNOWN";
        case DisplaySurface::COMPOSITION_GLES:    return "GLES";
        case DisplaySurface::COMPOSITION_HWC:     return "HWC";
        case DisplaySurface::COMPOSITION_MIXED:   return "MIXED";
        default:                                  return "<INVALID>";
    }
}

VirtualDisplaySurface::VirtualDisplaySurface(HWComposer& hwc, int32_t dispId,
        const sp<IGraphicBufferProducer>& sink,
        const sp<IGraphicBufferProducer>& bqProducer,
        const sp<IGraphicBufferConsumer>& bqConsumer,
        const String8& name)
:   ConsumerBase(bqConsumer),
    mHwc(hwc),
    mDisplayId(dispId),
    mDisplayName(name),
    mOutputUsage(GRALLOC_USAGE_HW_COMPOSER),
    mProducerSlotSource(0),
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    mFbProducerSlot(-1),
    mOutputProducerSlot(-1),
    mSinkUsage(0),
    mSinkFormat(0),
    mBQError(0),
#endif
#ifdef SUPPORT_Q_DQ_SEQUENCE
    mNextOutputProducerSlot(-1),
#endif
    /* Variable that is used for EXYNOS_AFBC is added*/
    mInternalFormat(0),
    mDbgState(DBG_STATE_IDLE),
    mDbgLastCompositionType(COMPOSITION_GLES),
    mMustRecompose(false)
{
    mSource[SOURCE_SINK] = sink;
    mSource[SOURCE_SCRATCH] = bqProducer;

    resetPerFrameState();

    int sinkWidth, sinkHeight;
    sink->query(NATIVE_WINDOW_WIDTH, &sinkWidth);
    sink->query(NATIVE_WINDOW_HEIGHT, &sinkHeight);
    mSinkBufferWidth = sinkWidth;
    mSinkBufferHeight = sinkHeight;

    // Pick the buffer format to request from the sink when not rendering to it
    // with GLES. If the consumer needs CPU access, use the default format
    // set by the consumer. Otherwise allow gralloc to decide the format based
    // on usage bits.
    int sinkUsage;
    sink->query(NATIVE_WINDOW_CONSUMER_USAGE_BITS, &sinkUsage);
    if (sinkUsage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK)) {
        int sinkFormat;
        sink->query(NATIVE_WINDOW_FORMAT, &sinkFormat);
        mDefaultOutputFormat = sinkFormat;
    } else {
        mDefaultOutputFormat = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
    }
    mOutputFormat = mDefaultOutputFormat;

    ConsumerBase::mName = String8::format("VDS: %s", mDisplayName.string());
    mConsumer->setConsumerName(ConsumerBase::mName);
    mConsumer->setConsumerUsageBits(GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER);
    mConsumer->setDefaultBufferSize(sinkWidth, sinkHeight);
    mConsumer->setDefaultMaxBufferCount(2);
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    mHwc.getVirtualDisplayProperties(HWC_DISPLAY_VIRTUAL, HWC_DISPLAY_SINK_BQ_WIDTH, &mSinkBufferWidth);
    mHwc.getVirtualDisplayProperties(HWC_DISPLAY_VIRTUAL, HWC_DISPLAY_SINK_BQ_HEIGHT, &mSinkBufferHeight);
    if (mSinkBufferWidth == 0 || mSinkBufferHeight == 0) {
        mSinkBufferWidth = sinkWidth;
        mSinkBufferHeight = sinkHeight;
    }
    mCompositionType = COMPOSITION_GLES;

    int format;
    sink->query(NATIVE_WINDOW_FORMAT, &format);
    VDS_LOGV("VirtualDisplaySurface(), BQP format 0x%x", format);
    sp<IServiceManager> sm = defaultServiceManager();
    mHwcService = interface_cast<android::IExynosHWCService>(sm->getService(String16("Exynos.HWCService")));
#ifdef USES_VDS_OTHERFORMAT
    if (mHwcService != NULL)
        mHwcService->setVDSGlesFormat(format);
    else
        ALOGE("mHwcService is NULL");
#endif
#endif
}

VirtualDisplaySurface::~VirtualDisplaySurface() {
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IExynosHWCService> hwcService =
        interface_cast<android::IExynosHWCService>(sm->getService(String16("Exynos.HWCService")));
    if (hwcService != NULL) {
        int WFDMode = 0;
        WFDMode = hwcService->getWFDMode();
        ALOGE("VirtualDisplaySurface is deleted,WFDMode %d", WFDMode);
        if (WFDMode == 0)
            hwcService->setWFDOutputResolution(0, 0, 0, 0);
    }
    else
        ALOGE("HWCService::setWFDOutputResolution failed");
#endif
}

status_t VirtualDisplaySurface::beginFrame(bool mustRecompose) {
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    VDS_LOGV("beginFrame");
#endif
    if (mDisplayId < 0)
        return NO_ERROR;

    mMustRecompose = mustRecompose;

#ifndef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGW_IF(mDbgState != DBG_STATE_IDLE,
            "Unexpected beginFrame() in %s state", dbgStateStr());
#endif
    mDbgState = DBG_STATE_BEGUN;
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    mHwc.getVirtualDisplayProperties(HWC_DISPLAY_VIRTUAL, HWC_DISPLAY_SINK_BQ_WIDTH, &mSinkBufferWidth);
    mHwc.getVirtualDisplayProperties(HWC_DISPLAY_VIRTUAL, HWC_DISPLAY_SINK_BQ_HEIGHT, &mSinkBufferHeight);
    return NO_ERROR;
#else
    return refreshOutputBuffer();
#endif
}

status_t VirtualDisplaySurface::prepareFrame(CompositionType compositionType) {
    if (mDisplayId < 0)
        return NO_ERROR;

#ifndef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGW_IF(mDbgState != DBG_STATE_BEGUN,
            "Unexpected prepareFrame() in %s state", dbgStateStr());
#endif
    mDbgState = DBG_STATE_PREPARED;
    mCompositionType = compositionType;

#ifdef USES_DISABLE_COMPOSITIONTYPE_GLES
    uint32_t hwcCompositionType;
    mHwc.getVirtualDisplayProperties(HWC_DISPLAY_VIRTUAL, HWC_DISPLAY_COMPOSITION_TYPE, &hwcCompositionType);
    mCompositionType = (CompositionType)hwcCompositionType;
#endif

    if (sForceHwcCopy && mCompositionType == COMPOSITION_GLES) {
        // Some hardware can do RGB->YUV conversion more efficiently in hardware
        // controlled by HWC than in hardware controlled by the video encoder.
        // Forcing GLES-composed frames to go through an extra copy by the HWC
        // allows the format conversion to happen there, rather than passing RGB
        // directly to the consumer.
        //
        // On the other hand, when the consumer prefers RGB or can consume RGB
        // inexpensively, this forces an unnecessary copy.
        mCompositionType = COMPOSITION_MIXED;
    }

#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    if (mCompositionType != COMPOSITION_GLES) {
        mHwc.getVirtualDisplayProperties(HWC_DISPLAY_VIRTUAL, HWC_DISPLAY_SINK_BQ_USAGE, &mSinkUsage);
        mHwc.getVirtualDisplayProperties(HWC_DISPLAY_VIRTUAL, HWC_DISPLAY_SINK_BQ_FORMAT, &mSinkFormat);
    }
#endif

    if (mCompositionType != mDbgLastCompositionType) {
        VDS_LOGV("prepareFrame: composition type changed to %s",
                dbgCompositionTypeStr(mCompositionType));
        mDbgLastCompositionType = mCompositionType;
    }

#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    VDS_LOGV("prepareFrame: compositionType %s", dbgCompositionTypeStr(mCompositionType));
    if (mCompositionType != COMPOSITION_GLES) {
        mOutputUsage = mSinkUsage;
        if (mSinkFormat == 0xFFFFFFFF) {
            VDS_LOGV("prepareFrame: rotation state");
            return NO_ERROR;
        }
        return refreshOutputBuffer();
    }
#else
    if (mCompositionType != COMPOSITION_GLES &&
            (mOutputFormat != mDefaultOutputFormat ||
             mOutputUsage != GRALLOC_USAGE_HW_COMPOSER)) {
        // We must have just switched from GLES-only to MIXED or HWC
        // composition. Stop using the format and usage requested by the GLES
        // driver; they may be suboptimal when HWC is writing to the output
        // buffer. For example, if the output is going to a video encoder, and
        // HWC can write directly to YUV, some hardware can skip a
        // memory-to-memory RGB-to-YUV conversion step.
        //
        // If we just switched *to* GLES-only mode, we'll change the
        // format/usage and get a new buffer when the GLES driver calls
        // dequeueBuffer().
        mOutputFormat = mDefaultOutputFormat;
        mOutputUsage = GRALLOC_USAGE_HW_COMPOSER;
        refreshOutputBuffer();
    }
#endif
    return NO_ERROR;
}

status_t VirtualDisplaySurface::compositionComplete() {
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    VDS_LOGV("compositionComplete");
#endif
    return NO_ERROR;
}

status_t VirtualDisplaySurface::advanceFrame() {
    if (mDisplayId < 0)
        return NO_ERROR;

#ifndef SUPPORT_Q_DQ_SEQUENCE
    if (mCompositionType == COMPOSITION_HWC) {
        VDS_LOGW_IF(mDbgState != DBG_STATE_PREPARED,
                "Unexpected advanceFrame() in %s state on HWC frame",
                dbgStateStr());
    } else {
        VDS_LOGW_IF(mDbgState != DBG_STATE_GLES_DONE,
                "Unexpected advanceFrame() in %s state on GLES/MIXED frame",
                dbgStateStr());
    }
#endif
    mDbgState = DBG_STATE_HWC;
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    if (mBQError == NO_INIT) {
        ALOGE("advanceFrame(), buffer queue has been abadoned, error 0x%x", mBQError);
        mHwc.setOutputBuffer(mDisplayId, Fence::NO_FENCE, NULL);
        return NO_ERROR;
    }
#endif
    if (mOutputProducerSlot < 0 ||
            (mCompositionType != COMPOSITION_HWC && mFbProducerSlot < 0)) {
        // Last chance bailout if something bad happened earlier. For example,
        // in a GLES configuration, if the sink disappears then dequeueBuffer
        // will fail, the GLES driver won't queue a buffer, but SurfaceFlinger
        // will soldier on. So we end up here without a buffer. There should
        // be lots of scary messages in the log just before this.
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
        if (mCompositionType == COMPOSITION_HWC && mSinkFormat == 0xFFFFFFFF)
            return NO_ERROR;
#endif
#ifdef SUPPORT_Q_DQ_SEQUENCE
        VDS_LOGE("advanceFrame: no buffer, bailing out, mFbProducerSlot %d, mOutputProducerSlot %d", mFbProducerSlot, mOutputProducerSlot);
#else
        VDS_LOGE("advanceFrame: no buffer, bailing out");
#endif
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
        mHwc.setOutputBuffer(mDisplayId, Fence::NO_FENCE, NULL);
#endif
        return NO_MEMORY;
    }

    sp<GraphicBuffer> fbBuffer = mFbProducerSlot >= 0 ?
            mProducerBuffers[mFbProducerSlot] : sp<GraphicBuffer>(NULL);
    sp<GraphicBuffer> outBuffer = mProducerBuffers[mOutputProducerSlot];
    VDS_LOGV("advanceFrame: fb=%d(%p) out=%d(%p), mFbFence=0x%p, mOutputFence=0x%p",
            mFbProducerSlot, fbBuffer.get(),
            mOutputProducerSlot, outBuffer.get(), mFbFence.get(), mOutputFence.get());

    // At this point we know the output buffer acquire fence,
    // so update HWC state with it.
    mHwc.setOutputBuffer(mDisplayId, mOutputFence, outBuffer);

    status_t result = NO_ERROR;
    if (fbBuffer != NULL) {
        result = mHwc.fbPost(mDisplayId, mFbFence, fbBuffer);
    }

    return result;
}

void VirtualDisplaySurface::onFrameCommitted() {
    if (mDisplayId < 0)
        return;

#ifndef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGW_IF(mDbgState != DBG_STATE_HWC,
            "Unexpected onFrameCommitted() in %s state", dbgStateStr());
#endif
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    if (mBQError == NO_INIT) {
        ALOGE("onFrameCommitted(), buffer queue has been abadoned, error 0x%x", mBQError);
        return;
    }
#endif
    mDbgState = DBG_STATE_IDLE;
    sp<Fence> fbFence = mHwc.getAndResetReleaseFence(mDisplayId);

#ifdef SUPPORT_Q_DQ_SEQUENCE
    if (mCompositionType == COMPOSITION_MIXED &&
        (mFbProducerSlot >= BufferQueue::NUM_BUFFER_SLOTS - SCRATCH_BQ_BUF_COUNT)) {
#else
    if (mCompositionType == COMPOSITION_MIXED && mFbProducerSlot >= 0) {
#endif
        // release the scratch buffer back to the pool
        Mutex::Autolock lock(mMutex);
        int sslot = mapProducer2SourceSlot(SOURCE_SCRATCH, mFbProducerSlot);
        VDS_LOGV("onFrameCommitted: release scratch sslot=%d", sslot);
        addReleaseFenceLocked(sslot, mProducerBuffers[mFbProducerSlot], fbFence);
        releaseBufferLocked(sslot, mProducerBuffers[mFbProducerSlot],
                EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
    }

    if (mOutputProducerSlot >= 0) {
        int sslot = mapProducer2SourceSlot(SOURCE_SINK, mOutputProducerSlot);
        QueueBufferOutput qbo;
        sp<Fence> outFence = mHwc.getLastRetireFence(mDisplayId);
        VDS_LOGV("onFrameCommitted: queue sink sslot=%d, outFence=0x%p", sslot, outFence.get());
        if (mMustRecompose) {
            /* Parameter that is used for EXYNOS_AFBC is added */
            status_t result = mSource[SOURCE_SINK]->queueBuffer(sslot,
                    QueueBufferInput(
                        systemTime(), false /* isAutoTimestamp */,
                        HAL_DATASPACE_UNKNOWN,
                        Rect(mSinkBufferWidth, mSinkBufferHeight),
                        NATIVE_WINDOW_SCALING_MODE_FREEZE, 0 /* transform */,
                        true /* async*/,
                        outFence, mInternalFormat),
                    &qbo);
            if (result == NO_ERROR) {
                updateQueueBufferOutput(qbo);
            }
        } else {
            // If the surface hadn't actually been updated, then we only went
            // through the motions of updating the display to keep our state
            // machine happy. We cancel the buffer to avoid triggering another
            // re-composition and causing an infinite loop.
            mSource[SOURCE_SINK]->cancelBuffer(sslot, outFence);
        }
    }

    resetPerFrameState();
}

void VirtualDisplaySurface::dumpAsString(String8& /* result */) const {
}

void VirtualDisplaySurface::resizeBuffers(const uint32_t w, const uint32_t h) {
    uint32_t tmpW, tmpH, transformHint, numPendingBuffers;
    mQueueBufferOutput.deflate(&tmpW, &tmpH, &transformHint, &numPendingBuffers);
    mQueueBufferOutput.inflate(w, h, transformHint, numPendingBuffers);

    mSinkBufferWidth = w;
    mSinkBufferHeight = h;
}

status_t VirtualDisplaySurface::requestBuffer(int pslot,
        sp<GraphicBuffer>* outBuf) {
    if (mDisplayId < 0)
        return mSource[SOURCE_SINK]->requestBuffer(pslot, outBuf);
#ifndef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGW_IF(mDbgState != DBG_STATE_GLES,
            "Unexpected requestBuffer pslot=%d in %s state",
            pslot, dbgStateStr());
#endif
    *outBuf = mProducerBuffers[pslot];
    return NO_ERROR;
}

status_t VirtualDisplaySurface::setBufferCount(int bufferCount) {
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    VDS_LOGV("setBufferCount %d", bufferCount);
#endif
    return mSource[SOURCE_SINK]->setBufferCount(bufferCount);
}

status_t VirtualDisplaySurface::dequeueBuffer(Source source,
        PixelFormat format, uint32_t usage, int* sslot, sp<Fence>* fence) {
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    VDS_LOGV("dequeueBuffer(vds) source %d, format 0x%x, usage 0x%x",
        source, format, usage);
#endif
    LOG_FATAL_IF(mDisplayId < 0, "mDisplayId=%d but should not be < 0.", mDisplayId);
    // Don't let a slow consumer block us
    bool async = (source == SOURCE_SINK);

#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    status_t result;
    if ((source == SOURCE_SINK) && (mCompositionType == COMPOSITION_MIXED || mCompositionType == COMPOSITION_HWC)) {
        mHwc.getVirtualDisplayProperties(HWC_DISPLAY_VIRTUAL, HWC_DISPLAY_SINK_BQ_USAGE, &mSinkUsage);
        VDS_LOGV("dequeueBuffer(vds) source %d, mSinkFormat 0x%x, mSinkUsage 0x%x", source, mSinkFormat, mSinkUsage);
        result = mSource[source]->dequeueBuffer(sslot, fence, async,
            mSinkBufferWidth, mSinkBufferHeight, mSinkFormat, mSinkUsage);
    } else {
        result = mSource[source]->dequeueBuffer(sslot, fence, async,
            mSinkBufferWidth, mSinkBufferHeight, format, usage);
    }
    mBQError = result;
#else
    status_t result = mSource[source]->dequeueBuffer(sslot, fence, async,
            mSinkBufferWidth, mSinkBufferHeight, format, usage);
#endif

    if (result < 0) {
        ALOGE("dequeueBuffer(vds) dequeueBuffer(%s) return err %d", dbgSourceStr(source), result);
        return result;
    }

    int pslot = mapSource2ProducerSlot(source, *sslot);
    VDS_LOGV("dequeueBuffer(%s): sslot=%d pslot=%d result=%d",
            dbgSourceStr(source), *sslot, pslot, result);
    uint64_t sourceBit = static_cast<uint64_t>(source) << pslot;

    if ((mProducerSlotSource & (1ULL << pslot)) != sourceBit) {
        // This slot was previously dequeued from the other source; must
        // re-request the buffer.
        result |= BUFFER_NEEDS_REALLOCATION;
        mProducerSlotSource &= ~(1ULL << pslot);
        mProducerSlotSource |= sourceBit;
    }

    if (result & RELEASE_ALL_BUFFERS) {
        for (uint32_t i = 0; i < BufferQueue::NUM_BUFFER_SLOTS; i++) {
            if ((mProducerSlotSource & (1ULL << i)) == sourceBit)
                mProducerBuffers[i].clear();
        }
    }
    if (result & BUFFER_NEEDS_REALLOCATION) {
#ifdef USES_VIRTUAL_DISPLAY
        mSource[source]->requestBuffer(*sslot, &mProducerBuffers[pslot]);
#else
        result = mSource[source]->requestBuffer(*sslot, &mProducerBuffers[pslot]);
        if (result < 0) {
            mProducerBuffers[pslot].clear();
            mSource[source]->cancelBuffer(*sslot, *fence);
            return result;
        }
#endif
        VDS_LOGV("dequeueBuffer(%s): buffers[%d]=%p fmt=%d usage=%#x",
                dbgSourceStr(source), pslot, mProducerBuffers[pslot].get(),
                mProducerBuffers[pslot]->getPixelFormat(),
                mProducerBuffers[pslot]->getUsage());
    }
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    VDS_LOGV("dequeueBuffer pslot %d", pslot);
#endif
    return result;
}

/* Parameters and variables those are used for EXYNOS_AFBC are added */
status_t VirtualDisplaySurface::dequeueBuffer(int* pslot, sp<Fence>* fence, bool async,
        uint32_t w, uint32_t h, PixelFormat format, uint32_t usage, int* preferCompression) {
    if (mDisplayId < 0)
        return mSource[SOURCE_SINK]->dequeueBuffer(pslot, fence, async, w, h, format, usage);
#ifndef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGW_IF(mDbgState != DBG_STATE_PREPARED,
            "Unexpected dequeueBuffer() in %s state", dbgStateStr());
#endif

    status_t result = NO_ERROR;
#ifdef SUPPORT_DQ_Q_SEQUENCE
    if (mDbgState == DBG_STATE_IDLE) {
        mOutputUsage = usage | GRALLOC_USAGE_HW_COMPOSER;
        result = refreshOutputBuffer();
        *pslot = mOutputProducerSlot;
        *fence = mOutputFence;
        mOutputProducerSlot = -1;
        return result;
    }
#endif
    mDbgState = DBG_STATE_GLES;
    if (preferCompression)
        *preferCompression = 0;

#ifndef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGW_IF(!async, "EGL called dequeueBuffer with !async despite eglSwapInterval(0)");
#endif
    VDS_LOGV("dequeueBuffer(g3d) %dx%d fmt=%d usage=%#x", w, h, format, usage);

    Source source = fbSourceForCompositionType(mCompositionType);

    if (source == SOURCE_SINK) {
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
        mSinkFormat = format;
        mOutputUsage = usage | GRALLOC_USAGE_HW_COMPOSER;
        result = refreshOutputBuffer();
        if (result < 0)
            return result;
#else
        if (mOutputProducerSlot < 0) {
            // Last chance bailout if something bad happened earlier. For example,
            // in a GLES configuration, if the sink disappears then dequeueBuffer
            // will fail, the GLES driver won't queue a buffer, but SurfaceFlinger
            // will soldier on. So we end up here without a buffer. There should
            // be lots of scary messages in the log just before this.
            VDS_LOGE("dequeueBuffer: no buffer, bailing out");
            return NO_MEMORY;
        }

        // We already dequeued the output buffer. If the GLES driver wants
        // something incompatible, we have to cancel and get a new one. This
        // will mean that HWC will see a different output buffer between
        // prepare and set, but since we're in GLES-only mode already it
        // shouldn't matter.

        usage |= GRALLOC_USAGE_HW_COMPOSER;
        const sp<GraphicBuffer>& buf = mProducerBuffers[mOutputProducerSlot];
        if ((usage & ~buf->getUsage()) != 0 ||
                (format != 0 && format != buf->getPixelFormat()) ||
                (w != 0 && w != mSinkBufferWidth) ||
                (h != 0 && h != mSinkBufferHeight)) {
            VDS_LOGV("dequeueBuffer: dequeueing new output buffer: "
                    "want %dx%d fmt=%d use=%#x, "
                    "have %dx%d fmt=%d use=%#x",
                    w, h, format, usage,
                    mSinkBufferWidth, mSinkBufferHeight,
                    buf->getPixelFormat(), buf->getUsage());
            mOutputFormat = format;
            mOutputUsage = usage;
            result = refreshOutputBuffer();
            if (result < 0)
                return result;
        }
#endif
    }

    if (source == SOURCE_SINK) {
#ifdef SUPPORT_Q_DQ_SEQUENCE
        *pslot = mNextOutputProducerSlot;
        *fence = mNextOutputFence;
#else
        *pslot = mOutputProducerSlot;
        *fence = mOutputFence;
#endif
    } else {
        int sslot;
        result = dequeueBuffer(source, format, usage, &sslot, fence);
        if (result >= 0) {
            *pslot = mapSource2ProducerSlot(source, sslot);
        }
    }
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    VDS_LOGV("dequeueBuffer pslot %d", *pslot);
#endif
    return result;
}

status_t VirtualDisplaySurface::detachBuffer(int /* slot */) {
    VDS_LOGE("detachBuffer is not available for VirtualDisplaySurface");
    return INVALID_OPERATION;
}

status_t VirtualDisplaySurface::detachNextBuffer(
        sp<GraphicBuffer>* /* outBuffer */, sp<Fence>* /* outFence */) {
    VDS_LOGE("detachNextBuffer is not available for VirtualDisplaySurface");
    return INVALID_OPERATION;
}

status_t VirtualDisplaySurface::attachBuffer(int* /* outSlot */,
        const sp<GraphicBuffer>& /* buffer */) {
    VDS_LOGE("attachBuffer is not available for VirtualDisplaySurface");
    return INVALID_OPERATION;
}

status_t VirtualDisplaySurface::queueBuffer(int pslot,
        const QueueBufferInput& input, QueueBufferOutput* output) {
    if (mDisplayId < 0)
        return mSource[SOURCE_SINK]->queueBuffer(pslot, input, output);
#ifndef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGW_IF(mDbgState != DBG_STATE_GLES,
            "Unexpected queueBuffer(pslot=%d) in %s state", pslot,
            dbgStateStr());
#endif
    mDbgState = DBG_STATE_GLES_DONE;

#ifdef SUPPORT_Q_DQ_SEQUENCE
    if (mOutputProducerSlot <= -1)
        mOutputProducerSlot = mNextOutputProducerSlot;
#endif

    VDS_LOGV("queueBuffer pslot=%d", pslot);

    status_t result;
    if (mCompositionType == COMPOSITION_MIXED) {
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
        if (pslot < BufferQueue::NUM_BUFFER_SLOTS - SCRATCH_BQ_BUF_COUNT) {
            int64_t timestamp;
            bool isAutoTimestamp;
            android_dataspace dataSpace;
            Rect crop;
            int scalingMode;
            uint32_t transform;
            bool async;
            sp<Fence> queuedFence;
            input.deflate(&timestamp, &isAutoTimestamp, &dataSpace, &crop, &scalingMode,
                    &transform, &async, &queuedFence);
            mSource[SOURCE_SINK]->cancelBuffer(pslot, queuedFence);
            *output = mQueueBufferOutput;
            mFbProducerSlot = pslot;
            VDS_LOGV("SOURCE_SINK cancelBuffer pslot=%d, queuedFence 0x%p", pslot, queuedFence.get());
            return NO_ERROR;
        }
#endif
        // Queue the buffer back into the scratch pool
        QueueBufferOutput scratchQBO;
        int sslot = mapProducer2SourceSlot(SOURCE_SCRATCH, pslot);
        result = mSource[SOURCE_SCRATCH]->queueBuffer(sslot, input, &scratchQBO);
        if (result != NO_ERROR)
            return result;

        // Now acquire the buffer from the scratch pool -- should be the same
        // slot and fence as we just queued.
        Mutex::Autolock lock(mMutex);
        BufferItem item;
        result = acquireBufferLocked(&item, 0);
        if (result != NO_ERROR)
            return result;
#ifdef SUPPORT_Q_DQ_SEQUENCE
        VDS_LOGV("queueBuffer: acquired sslot %d from SCRATCH after queueing sslot %d",
            item.mBuf, sslot);
#else
        VDS_LOGW_IF(item.mBuf != sslot,
                "queueBuffer: acquired sslot %d from SCRATCH after queueing sslot %d",
                item.mBuf, sslot);
#endif
        mFbProducerSlot = mapSource2ProducerSlot(SOURCE_SCRATCH, item.mBuf);
        mFbFence = mSlots[item.mBuf].mFence;

#ifdef SUPPORT_Q_DQ_SEQUENCE
        VDS_LOGV("queueBuffer(%s): pslot=%d, mFbProducerSlot=%d, mFbFence=0x%p", dbgSourceStr(SOURCE_SCRATCH), pslot, mFbProducerSlot, mFbFence.get());
#endif
    } else {
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
#else
        LOG_FATAL_IF(mCompositionType != COMPOSITION_GLES,
                "Unexpected queueBuffer in state %s for compositionType %s",
                dbgStateStr(), dbgCompositionTypeStr(mCompositionType));
#endif

#ifdef SUPPORT_Q_DQ_SEQUENCE
        if (pslot >= BufferQueue::NUM_BUFFER_SLOTS - SCRATCH_BQ_BUF_COUNT) {
            int64_t timestamp;
            bool isAutoTimestamp;
            android_dataspace dataSpace;
            Rect crop;
            int scalingMode;
            uint32_t transform;
            bool async;
            sp<Fence> queuedFence;
            input.deflate(&timestamp, &isAutoTimestamp, &dataSpace, &crop, &scalingMode,
                    &transform, &async, &queuedFence);
            mSource[SOURCE_SCRATCH]->cancelBuffer(mapProducer2SourceSlot(SOURCE_SCRATCH, pslot), queuedFence);
            *output = mQueueBufferOutput;
            VDS_LOGV("SOURCE_SCRATCH cancelBuffer pslot=%d, queuedFence 0x%p",
                mapProducer2SourceSlot(SOURCE_SCRATCH, pslot), queuedFence.get());
            return NO_ERROR;
        }
#endif
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
        mOutputProducerSlot = pslot;
#endif

        // Extract the GLES release fence for HWC to acquire
        int64_t timestamp;
        bool isAutoTimestamp;
        android_dataspace dataSpace;
        Rect crop;
        int scalingMode;
        uint32_t transform;
        bool async;
        /* Parameters and variables those are used for EXYNOS_AFBC are added */
        input.deflate(&timestamp, &isAutoTimestamp, &dataSpace, &crop,
                &scalingMode, &transform, &async, &mFbFence, NULL, &mInternalFormat);

        mFbProducerSlot = pslot;
        mOutputFence = mFbFence;
    }

    *output = mQueueBufferOutput;
#ifdef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGV("queueBuffer pslot=%d, mFbProducerSlot %d", pslot, mFbProducerSlot);
#endif
    return NO_ERROR;
}

void VirtualDisplaySurface::cancelBuffer(int pslot, const sp<Fence>& fence) {
    if (mDisplayId < 0)
        return mSource[SOURCE_SINK]->cancelBuffer(mapProducer2SourceSlot(SOURCE_SINK, pslot), fence);
#ifndef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGW_IF(mDbgState != DBG_STATE_GLES,
            "Unexpected cancelBuffer(pslot=%d) in %s state", pslot,
            dbgStateStr());
#endif
    VDS_LOGV("cancelBuffer pslot=%d", pslot);
    Source source = fbSourceForCompositionType(mCompositionType);
    return mSource[source]->cancelBuffer(
            mapProducer2SourceSlot(source, pslot), fence);
}

int VirtualDisplaySurface::query(int what, int* value) {
    switch (what) {
        case NATIVE_WINDOW_WIDTH:
            *value = mSinkBufferWidth;
            break;
        case NATIVE_WINDOW_HEIGHT:
            *value = mSinkBufferHeight;
            break;
        default:
#ifdef USES_DISABLE_COMPOSITIONTYPE_GLES
            return mSource[SOURCE_SCRATCH]->query(what, value);
#else
            return mSource[SOURCE_SINK]->query(what, value);
#endif
    }
    return NO_ERROR;
}

status_t VirtualDisplaySurface::connect(const sp<IProducerListener>& listener,
        int api, bool producerControlledByApp,
        QueueBufferOutput* output) {
    QueueBufferOutput qbo;
#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
    status_t result = mSource[SOURCE_SINK]->connect(listener, api,
            producerControlledByApp, &qbo);
    ALOGI("connect(SOURCE_SINK) listener %p, api %d, producerControlledByApp %d, result %d",
        listener.get(), api, producerControlledByApp, result);
    if (result != NO_ERROR) {
        result = mSource[SOURCE_SCRATCH]->connect(listener, api,
            producerControlledByApp, &qbo);
        ALOGI("connect(SOURCE_SCRATCH) listener %p, api %d, producerControlledByApp %d, result %d",
            listener.get(), api, producerControlledByApp, result);
    }
#else
    status_t result = mSource[SOURCE_SINK]->connect(listener, api,
            producerControlledByApp, &qbo);
#endif
    if (result == NO_ERROR) {
        updateQueueBufferOutput(qbo);
        *output = mQueueBufferOutput;
    }
    return result;
}

status_t VirtualDisplaySurface::disconnect(int api) {
#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
    ALOGI("disconnect() api %d", api);
    status_t result = mSource[SOURCE_SINK]->disconnect(api);
    if (result != NO_ERROR) {
        result = mSource[SOURCE_SCRATCH]->disconnect(api);
    }
    return result;
#else
    return mSource[SOURCE_SINK]->disconnect(api);
#endif
}

status_t VirtualDisplaySurface::setSidebandStream(const sp<NativeHandle>& /*stream*/) {
    return INVALID_OPERATION;
}

void VirtualDisplaySurface::allocateBuffers(bool /* async */,
        uint32_t /* width */, uint32_t /* height */, PixelFormat /* format */,
        uint32_t /* usage */) {
    // TODO: Should we actually allocate buffers for a virtual display?
}

status_t VirtualDisplaySurface::allowAllocation(bool /* allow */) {
    return INVALID_OPERATION;
}

status_t VirtualDisplaySurface::setGenerationNumber(uint32_t /* generation */) {
    ALOGE("setGenerationNumber not supported on VirtualDisplaySurface");
    return INVALID_OPERATION;
}

String8 VirtualDisplaySurface::getConsumerName() const {
    return String8("VirtualDisplaySurface");
}

void VirtualDisplaySurface::updateQueueBufferOutput(
        const QueueBufferOutput& qbo) {
    uint32_t w, h, transformHint, numPendingBuffers;
    qbo.deflate(&w, &h, &transformHint, &numPendingBuffers);
    mQueueBufferOutput.inflate(w, h, 0, numPendingBuffers);
}

void VirtualDisplaySurface::resetPerFrameState() {
    mCompositionType = COMPOSITION_UNKNOWN;
    mFbFence = Fence::NO_FENCE;
    mOutputFence = Fence::NO_FENCE;
#ifdef SUPPORT_Q_DQ_SEQUENCE
    mNextOutputProducerSlot = -1;
    mNextOutputFence = Fence::NO_FENCE;
#else
    mOutputProducerSlot = -1;
#endif
    mFbProducerSlot = -1;
}

status_t VirtualDisplaySurface::refreshOutputBuffer() {
#ifndef SUPPORT_Q_DQ_SEQUENCE
    if (mOutputProducerSlot >= 0) {
        mSource[SOURCE_SINK]->cancelBuffer(
                mapProducer2SourceSlot(SOURCE_SINK, mOutputProducerSlot),
                mOutputFence);
    }
#endif
    int sslot;
    status_t result;
#ifdef SUPPORT_Q_DQ_SEQUENCE
    sp<Fence> outFence;
    result = dequeueBuffer(SOURCE_SINK, mSinkFormat, mOutputUsage, &sslot, &outFence);
    if (mCompositionType == COMPOSITION_GLES) {
        mNextOutputProducerSlot = mapSource2ProducerSlot(SOURCE_SINK, sslot);
        mNextOutputFence = outFence;
        if(mOutputProducerSlot <= -1)
            mOutputProducerSlot = mNextOutputProducerSlot;
    } else {
        mOutputProducerSlot = mapSource2ProducerSlot(SOURCE_SINK, sslot);
        mOutputFence = outFence;
    }
#endif
#ifdef SUPPORT_DQ_Q_SEQUENCE
    sp<Fence> outFence;
    if ((result = dequeueBuffer(SOURCE_SINK, mSinkFormat, mOutputUsage, &sslot, &outFence)) >= 0) {
        mOutputProducerSlot = mapSource2ProducerSlot(SOURCE_SINK, sslot);
        mOutputFence = outFence;
    }
#endif

#ifndef SUPPORT_Q_DQ_SEQUENCE
#ifndef SUPPORT_DQ_Q_SEQUENCE
    result = dequeueBuffer(SOURCE_SINK, mOutputFormat, mOutputUsage,
            &sslot, &mOutputFence);
    mOutputProducerSlot = mapSource2ProducerSlot(SOURCE_SINK, sslot);
#endif
#endif

    if (result < 0)
        return result;

    // On GLES-only frames, we don't have the right output buffer acquire fence
    // until after GLES calls queueBuffer(). So here we just set the buffer
    // (for use in HWC prepare) but not the fence; we'll call this again with
    // the proper fence once we have it.
#if defined (SUPPORT_DQ_Q_SEQUENCE) || defined(SUPPORT_Q_DQ_SEQUENCE)
    // g3d use result. Don't over-write result.
    mHwc.setOutputBuffer(mDisplayId, Fence::NO_FENCE,
        mProducerBuffers[mOutputProducerSlot]);
#else
    result = mHwc.setOutputBuffer(mDisplayId, Fence::NO_FENCE,
            mProducerBuffers[mOutputProducerSlot]);
#endif

#ifdef SUPPORT_Q_DQ_SEQUENCE
    VDS_LOGV("refreshOutputBuffer: mOutputProducerSlot=%d, mNextOutputProducerSlot=%d", mOutputProducerSlot, mNextOutputProducerSlot);
#endif

    return result;
}

// This slot mapping function is its own inverse, so two copies are unnecessary.
// Both are kept to make the intent clear where the function is called, and for
// the (unlikely) chance that we switch to a different mapping function.
int VirtualDisplaySurface::mapSource2ProducerSlot(Source source, int sslot) {
    if (source == SOURCE_SCRATCH) {
        return BufferQueue::NUM_BUFFER_SLOTS - sslot - 1;
    } else {
        return sslot;
    }
}
int VirtualDisplaySurface::mapProducer2SourceSlot(Source source, int pslot) {
    return mapSource2ProducerSlot(source, pslot);
}

VirtualDisplaySurface::Source
VirtualDisplaySurface::fbSourceForCompositionType(CompositionType type) {
    return type == COMPOSITION_MIXED ? SOURCE_SCRATCH : SOURCE_SINK;
}

const char* VirtualDisplaySurface::dbgStateStr() const {
    switch (mDbgState) {
        case DBG_STATE_IDLE:      return "IDLE";
        case DBG_STATE_PREPARED:  return "PREPARED";
        case DBG_STATE_GLES:      return "GLES";
        case DBG_STATE_GLES_DONE: return "GLES_DONE";
        case DBG_STATE_HWC:       return "HWC";
        default:                  return "INVALID";
    }
}

const char* VirtualDisplaySurface::dbgSourceStr(Source s) {
    switch (s) {
        case SOURCE_SINK:    return "SINK";
        case SOURCE_SCRATCH: return "SCRATCH";
        default:             return "INVALID";
    }
}

// ---------------------------------------------------------------------------
} // namespace android
// ---------------------------------------------------------------------------
