#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <utils/Trace.h>
#include "ExynosMPP.h"
#include "ExynosHWCUtils.h"
#ifdef USES_VIRTUAL_DISPLAY
#include "ExynosVirtualDisplay.h"
#endif

size_t visibleWidth(ExynosMPP *processor, hwc_layer_1_t &layer, int format,
        int xres)
{
    int bpp;
    if (processor->isProcessingRequired(layer, format) && format != HAL_PIXEL_FORMAT_RGB_565)
        bpp = 32;
    else
        bpp = formatToBpp(format);
    int left = max(layer.displayFrame.left, 0);
    int right = min(layer.displayFrame.right, xres);

    return (right - left) * bpp / 8;
}

ExynosMPP::ExynosMPP()
{
    ExynosMPP(NULL, 0);
}

ExynosMPP::ExynosMPP(ExynosDisplay *display, int gscIndex)
{
    ATRACE_CALL();
    this->mDisplay = display;
    this->mIndex = gscIndex;
    mNeedReqbufs = false;
    mWaitVsyncCount = 0;
    mCountSameConfig = 0;
    mGscHandle = NULL;
    memset(&mSrcConfig, 0, sizeof(mSrcConfig));
    memset(&mMidConfig, 0, sizeof(mMidConfig));
    memset(&mDstConfig, 0, sizeof(mDstConfig));
    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
        mDstBuffers[i] = NULL;
        mMidBuffers[i] = NULL;
        mDstBufFence[i] = -1;
        mMidBufFence[i] = -1;
    }
    mNumAvailableDstBuffers = NUM_GSC_DST_BUFS;
    mCurrentBuf = 0;
    mGSCMode = 0;
    mLastGSCLayerHandle = -1;
    mS3DMode = 0;
    mppFact = NULL;
    libmpp = NULL;
    mDoubleOperation = false;
    mBufferFreeThread = new BufferFreeThread(this);
    mBufferFreeThread->mRunning = true;
    mBufferFreeThread->run();
}

ExynosMPP::~ExynosMPP()
{
    if (mBufferFreeThread != NULL) {
        mBufferFreeThread->mRunning = false;
        mBufferFreeThread->requestExitAndWait();
        delete mBufferFreeThread;
    }
}

bool ExynosMPP::isM2M()
{
    return mGSCMode == exynos5_gsc_map_t::GSC_M2M;
}

bool ExynosMPP::isUsingMSC()
{
    return (AVAILABLE_GSC_UNITS[mIndex] >= 4 && AVAILABLE_GSC_UNITS[mIndex] <= 6);
}

bool ExynosMPP::isOTF()
{
    return mGSCMode == exynos5_gsc_map_t::GSC_LOCAL;
}

void ExynosMPP::setMode(int mode)
{
    mGSCMode = mode;
}

void ExynosMPP::free()
{
    if (mNeedReqbufs) {
        if (mWaitVsyncCount > 0) {
            if (mGscHandle)
                freeMPP(mGscHandle);
            mNeedReqbufs = false;
            mWaitVsyncCount = 0;
            mDisplay->mOtfMode = OTF_OFF;
            mGscHandle = NULL;
            libmpp = NULL;
            memset(&mSrcConfig, 0, sizeof(mSrcConfig));
            memset(&mMidConfig, 0, sizeof(mMidConfig));
            memset(&mDstConfig, 0, sizeof(mDstConfig));
            memset(mDstBuffers, 0, sizeof(mDstBuffers));
            memset(mMidBuffers, 0, sizeof(mMidBuffers));
            mCurrentBuf = 0;
            mGSCMode = 0;
            mLastGSCLayerHandle = 0;

            for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
                mDstBufFence[i] = -1;
                mMidBufFence[i] = -1;
            }
        } else {
            mWaitVsyncCount++;
        }
    }
}

bool ExynosMPP::isSrcConfigChanged(exynos_mpp_img &c1, exynos_mpp_img &c2)
{
    return isDstConfigChanged(c1, c2) ||
            c1.fw != c2.fw ||
            c1.fh != c2.fh;
}

bool ExynosMPP::isFormatSupportedByGsc(int format)
{

    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
        return true;

    default:
        return false;
    }
}

bool ExynosMPP::formatRequiresGsc(int format)
{
    return (isFormatSupportedByGsc(format) &&
           (format != HAL_PIXEL_FORMAT_RGBX_8888) && (format != HAL_PIXEL_FORMAT_RGB_565) &&
		(format != HAL_PIXEL_FORMAT_RGBA_8888));
}

int ExynosMPP::getDownscaleRatio(int *downNumerator, int *downDenominator)
{
    *downNumerator = 0;
    *downDenominator = 0;
    return -1;
}

bool ExynosMPP::isFormatSupportedByGscOtf(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
        return true;
    default:
        return false;
    }
}

int ExynosMPP::isProcessingSupported(hwc_layer_1_t &layer, int format,
        bool local_path, int downNumerator, int downDenominator)
{
    if (local_path && downNumerator == 0) {
        return -eMPPUnsupportedDownScale;
    }

    if (isUsingMSC() && local_path) {
        return -eMPPUnsupportedHW;
    }

    if (!isUsingMSC() && layer.blending != HWC_BLENDING_NONE)
        return -eMPPUnsupportedBlending;

    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    if (isUsingMSC() && handle &&
        isFormatRgb(handle->format) && !hasAlpha(handle->format) &&
        (layer.blending == HWC_BLENDING_PREMULT))
        return -eMPPUnsupportedBlending;

    int max_w = maxWidth(layer);
    int max_h = maxHeight(layer);
    int min_w = minWidth(layer);
    int min_h = minHeight(layer);
    int crop_max_w = 0;
    int crop_max_h = 0;
    int dest_min_h = 0;

    if (isUsingMSC()) {
        crop_max_w = 8192;
        crop_max_h = 8192;
    } else {
        crop_max_w = isRotated(layer) ? 2016 : 4800;
        crop_max_h = isRotated(layer) ? 2016 : 3344;
    }
    int crop_min_w = isRotated(layer) ? 32: 64;
    int crop_min_h = isRotated(layer) ? 64: 32;

    if (local_path)
        dest_min_h = 64;
    else
        dest_min_h = 32;

    int srcWidthAlign = sourceWidthAlign(handle->format);
    int srcHeightAlign = sourceHeightAlign(handle->format);
    int dstAlign;
    if (local_path)
        dstAlign = destinationAlign(HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M);
    else
        dstAlign = destinationAlign(HAL_PIXEL_FORMAT_BGRA_8888);

    int maxDstWidth;
    int maxDstHeight;

    bool rot90or270 = !!(layer.transform & HAL_TRANSFORM_ROT_90);
    // n.b.: HAL_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_90 |
    //                               HAL_TRANSFORM_ROT_180

    int src_w = WIDTH(layer.sourceCropf), src_h = HEIGHT(layer.sourceCropf);
    if (!isUsingMSC()) {
        src_w = getAdjustedCrop(src_w, WIDTH(layer.displayFrame), handle->format, false, rot90or270);
        src_h = getAdjustedCrop(src_h, HEIGHT(layer.displayFrame), handle->format, true, rot90or270);
    }
    int dest_w, dest_h;
    if (rot90or270) {
        dest_w = HEIGHT(layer.displayFrame);
        dest_h = WIDTH(layer.displayFrame);
    } else {
        dest_w = WIDTH(layer.displayFrame);
        dest_h = HEIGHT(layer.displayFrame);
    }

    if (getDrmMode(handle->flags) != NO_DRM)
        alignCropAndCenter(dest_w, dest_h, NULL,
                GSC_DST_CROP_W_ALIGNMENT_RGB888);

    maxDstWidth = 2560;
    maxDstHeight = 2560;
    int max_upscale = 8;

    /* check whether GSC can handle with local path */
    if (local_path) {
        if (isFormatRgb(format) && rot90or270)
            return -eMPPUnsupportedRotation;

        /* GSC OTF can't handle rot90 or rot270 */
        if (!rotationSupported(rot90or270))
            return -eMPPUnsupportedRotation;
        /*
         * if display co-ordinates are out of the lcd resolution,
         * skip that scenario to OpenGL.
         * GSC OTF can't handle such scenarios.
         */
        if (layer.displayFrame.left < 0 || layer.displayFrame.top < 0 ||
            layer.displayFrame.right > mDisplay->mXres || layer.displayFrame.bottom > mDisplay->mYres)
            return -eMPPUnsupportedCoordinate;

        /* GSC OTF can't handle GRALLOC_USAGE_PROTECTED layer */
        if (getDrmMode(handle->flags) != NO_DRM)
            return -eMPPUnsupportedDRMContents;

        if (!isFormatSupportedByGsc(format))
            return -eMPPUnsupportedFormat;
        else if (!isFormatSupportedByGscOtf(format))
            return -eMPPUnsupportedFormat;
        else if (format == HAL_PIXEL_FORMAT_RGBA_8888 && layer.blending != HWC_BLENDING_NONE)
            return -eMPPUnsupportedBlending;
        else if (mDisplay->mHwc->mS3DMode != S3D_MODE_DISABLED)
            return -eMPPUnsupportedS3DContents;
        else if (!paritySupported(dest_w, dest_h))
            return -eMPPNotAlignedDstSize;
        else if (handle->stride > max_w)
            return -eMPPExceedHStrideMaximum;
        else if (src_w  * downNumerator > dest_w * downDenominator)
            return -eMPPExeedMaxDownScale;
        else if (dest_w > maxDstWidth)
            return -eMPPExeedMaxDstWidth;
        else if (dest_w > src_w * max_upscale)
            return -eMPPExeedMaxUpScale;
        else if (handle->vstride > max_h)
            return -eMPPExceedVStrideMaximum;
        else if (src_h * downNumerator > dest_h * downDenominator)
            return -eMPPExeedMaxDownScale;
        else if (dest_h > maxDstHeight)
            return -eMPPExeedMaxDstHeight;
        else if (dest_h > src_h * max_upscale)
            return -eMPPExeedMaxUpScale;
        else if (src_w > crop_max_w)
            return -eMPPExeedSrcWCropMax;
        else if (src_h > crop_max_h)
            return -eMPPExeedSrcHCropMax;
        else if (src_w < crop_min_w)
            return -eMPPExeedSrcWCropMin;
        else if (src_h < crop_min_h)
            return -eMPPExeedSrcHCropMin;
        else if (dest_h < dest_min_h)
            return -eMPPExeedMinDstHeight;

        return 1;
     }

    bool need_gsc_op_twice = false;
    if (getDrmMode(handle->flags) != NO_DRM) {
        need_gsc_op_twice = ((dest_w > src_w * max_upscale) ||
                                   (dest_h > src_h * max_upscale)) ? true : false;
        if (need_gsc_op_twice)
            max_upscale = 8 * 8;
    } else {
        if (!mDisplay->mHasDrmSurface) {
            need_gsc_op_twice = false;
            max_upscale = 8;
        }
    }

    if (getDrmMode(handle->flags) != NO_DRM) {
        /* make even for gscaler */
        layer.sourceCropf.top = (unsigned int)layer.sourceCropf.top & ~1;
        layer.sourceCropf.left = (unsigned int)layer.sourceCropf.left & ~1;
        layer.sourceCropf.bottom = (unsigned int)layer.sourceCropf.bottom & ~1;
        layer.sourceCropf.right = (unsigned int)layer.sourceCropf.right & ~1;
    }

    /* check whether GSC can handle with M2M */
    if (!isFormatSupportedByGsc(format))
        return -eMPPUnsupportedFormat;
    else if (src_w < min_w)
        return -eMPPExeedMinSrcWidth;
    else if (src_h < min_h)
        return -eMPPExeedMinSrcHeight;
    else if (!isDstCropWidthAligned(dest_w))
        return -eMPPNotAlignedDstSize;
    else if (handle->stride > max_w)
        return -eMPPExceedHStrideMaximum;
    else if (handle->stride % srcWidthAlign != 0)
        return -eMPPNotAlignedHStride;
    else if (src_w * downNumerator >= dest_w * downDenominator)
        return -eMPPExeedMaxDownScale;
    else if (dest_w > src_w * max_upscale)
        return -eMPPExeedMaxUpScale;
    else if (handle->vstride > max_h)
        return -eMPPExceedVStrideMaximum;
    else if (handle->vstride % srcHeightAlign != 0)
        return -eMPPNotAlignedVStride;
    else if (src_h * downNumerator >= dest_h * downDenominator)
        return -eMPPExeedMaxDownScale;
    else if (dest_h > src_h * max_upscale)
        return -eMPPExeedMaxUpScale;
    else if (src_w > crop_max_w)
        return -eMPPExeedSrcWCropMax;
    else if (src_h > crop_max_h)
        return -eMPPExeedSrcHCropMax;
    else if (src_w < crop_min_w)
        return -eMPPExeedSrcWCropMin;
    else if (src_h < crop_min_h)
        return -eMPPExeedSrcHCropMin;
    else if (dest_h < dest_min_h)
        return -eMPPExeedMinDstHeight;
    // per 46.3.1.6

    return 1;
}

bool ExynosMPP::isProcessingRequired(hwc_layer_1_t &layer, int format)
{
    return formatRequiresGsc(format) || isScaled(layer)
            || isTransformed(layer);
}

int ExynosMPP::getAdjustedCrop(int rawSrcSize, int dstSize, int format, bool isVertical, bool isRotated)
{
    int ratio;
    int adjustedSize;
    int align;

    if (dstSize >= rawSrcSize || rawSrcSize <= dstSize * FIRST_PRESCALER_THRESHOLD)
        ratio = 1;
    else if (rawSrcSize < dstSize * SECOND_PRESCALER_THRESHOLD)
        ratio = 2;
    else
        ratio = 4;

    if (isFormatRgb(format)) {
        if (isRotated)
            align = ratio << 1;
        else
            align = ratio;
    } else {
        align = ratio << 1;
    }

    return rawSrcSize - (rawSrcSize % align);
}

void ExynosMPP::setupSource(exynos_mpp_img &src_img, hwc_layer_1_t &layer)
{
    bool rotation = !!(layer.transform & HAL_TRANSFORM_ROT_90);
    private_handle_t *src_handle = private_handle_t::dynamicCast(layer.handle);
    src_img.x = ALIGN((unsigned int)layer.sourceCropf.left, srcXOffsetAlign(layer));
    src_img.y = ALIGN((unsigned int)layer.sourceCropf.top, srcYOffsetAlign(layer));
    src_img.w = WIDTH(layer.sourceCropf) - (src_img.x - (uint32_t)layer.sourceCropf.left);
    src_img.fw = src_handle->stride;
    if (src_img.x + src_img.w > src_img.fw)
        src_img.w = src_img.fw - src_img.x;
    src_img.h = HEIGHT(layer.sourceCropf) - (src_img.y - (uint32_t)layer.sourceCropf.top);
    src_img.fh = src_handle->vstride;
    if (src_img.y + src_img.h > src_img.fh)
        src_img.h = src_img.fh - src_img.y;
    src_img.yaddr = src_handle->fd;
    if (!isUsingMSC()) {
        src_img.w = getAdjustedCrop(src_img.w, WIDTH(layer.displayFrame), src_handle->format, false, rotation);
        src_img.h = getAdjustedCrop(src_img.h, HEIGHT(layer.displayFrame), src_handle->format, true, rotation);
    }
    if (mS3DMode == S3D_SBS)
        src_img.w /= 2;
    if (mS3DMode == S3D_TB)
        src_img.h /= 2;
    if (isFormatYCrCb(src_handle->format)) {
        src_img.uaddr = src_handle->fd2;
        src_img.vaddr = src_handle->fd1;
    } else {
        src_img.uaddr = src_handle->fd1;
        src_img.vaddr = src_handle->fd2;
    }
    if (src_handle->format != HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL)
        src_img.format = src_handle->format;
    else
        src_img.format = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;

    if (layer.blending == HWC_BLENDING_COVERAGE)
        src_img.pre_multi = false;
    else
        src_img.pre_multi = true;

    src_img.drmMode = !!(getDrmMode(src_handle->flags) == SECURE_DRM);
    src_img.acquireFenceFd = layer.acquireFenceFd;
}

void ExynosMPP::setupOtfDestination(exynos_mpp_img &src_img, exynos_mpp_img &dst_img, hwc_layer_1_t &layer)
{
    dst_img.x = layer.displayFrame.left;
    dst_img.y = layer.displayFrame.top;
    dst_img.fw = mDisplay->mXres;
    dst_img.fh = mDisplay->mYres;
    dst_img.w = WIDTH(layer.displayFrame);
    dst_img.h = HEIGHT(layer.displayFrame);
    dst_img.w = min(dst_img.w, dst_img.fw - dst_img.x);
    dst_img.h = min(dst_img.h, dst_img.fh - dst_img.y);
    dst_img.format = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
    dst_img.rot = layer.transform;
    dst_img.drmMode = src_img.drmMode;
    dst_img.yaddr = (ptrdiff_t)NULL;
}

int ExynosMPP::sourceWidthAlign(int format)
{
    return 16;
}

int ExynosMPP::sourceHeightAlign(int format)
{
    return 16;
}

int ExynosMPP::destinationAlign(int format)
{
    return 16;
}

int ExynosMPP::reconfigureOtf(exynos_mpp_img *src_img, exynos_mpp_img *dst_img)
{
    int ret = 0;
    if (mGscHandle) {
        ret = stopMPP(mGscHandle);
        if (ret < 0) {
            ALOGE("failed to stop gscaler %u", mIndex);
            return ret;
        }
        mNeedReqbufs = true;
        mCountSameConfig = 0;
    }

    if (!mGscHandle) {
        mGscHandle = createMPP(AVAILABLE_GSC_UNITS[mIndex],
            GSC_OUTPUT_MODE, GSC_OUT_FIMD, false);
        if (!mGscHandle) {
            ALOGE("failed to create gscaler handle");
            return -1;
        }
    }

    ret = configMPP(mGscHandle, src_img, dst_img);
    if (ret < 0) {
        ALOGE("failed to configure gscaler %u", mIndex);
        return ret;
    }

    return ret;
}

int ExynosMPP::processOTF(hwc_layer_1_t &layer)
{
    ATRACE_CALL();
    ALOGV("configuring gscaler %u for memory-to-fimd-localout", mIndex);

    buffer_handle_t dst_buf;
    private_handle_t *dst_handle;
    int ret = 0;

    int dstAlign;

    exynos_mpp_img src_img, dst_img;
    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));

    setupSource(src_img, layer);
    setupOtfDestination(src_img, dst_img, layer);

    dstAlign = destinationAlign(dst_img.format);

    ALOGV("source configuration:");
    dumpMPPImage(src_img);

    if (!mGscHandle || isSrcConfigChanged(src_img, mSrcConfig) ||
            isDstConfigChanged(dst_img, mDstConfig)) {
        if (!isPerFrameSrcChanged(src_img, mSrcConfig) ||
                !isPerFrameDstChanged(dst_img, mDstConfig)) {
            ret = reconfigureOtf(&src_img, &dst_img);
	    if (ret < 0)
                goto err_gsc_local;
        }
    }

    if (isSourceCropChanged(src_img, mSrcConfig)) {
        setInputCrop(mGscHandle, &src_img, &dst_img);
    }

    ALOGV("destination configuration:");
    dumpMPPImage(dst_img);

    ret = runMPP(mGscHandle, &src_img, &dst_img);
    if (ret < 0) {
        ALOGE("failed to run gscaler %u", mIndex);
        goto err_gsc_local;
    }

    memcpy(&mSrcConfig, &src_img, sizeof(mSrcConfig));
    memcpy(&mDstConfig, &dst_img, sizeof(mDstConfig));

    return 0;

err_gsc_local:
    if (src_img.acquireFenceFd >= 0)
        close(src_img.acquireFenceFd);

    memset(&mSrcConfig, 0, sizeof(mSrcConfig));
    memset(&mDstConfig, 0, sizeof(mDstConfig));

    return ret;
}

bool ExynosMPP::setupDoubleOperation(exynos_mpp_img &src_img, exynos_mpp_img &mid_img, hwc_layer_1_t &layer)
{
    /* check if GSC need to operate twice */
    bool need_gsc_op_twice = false;
    bool need_unscaled_csc = false;
    private_handle_t *src_handle = private_handle_t::dynamicCast(layer.handle);
    const int max_upscale = 8;
    bool rot90or270 = !!(layer.transform & HAL_TRANSFORM_ROT_90);
    int src_w = WIDTH(layer.sourceCropf), src_h = HEIGHT(layer.sourceCropf);
    int dest_w, dest_h;
    if (rot90or270) {
        dest_w = HEIGHT(layer.displayFrame);
        dest_h = WIDTH(layer.displayFrame);
    } else {
        dest_w = WIDTH(layer.displayFrame);
        dest_h = HEIGHT(layer.displayFrame);
    }
    if (getDrmMode(src_handle->flags) != NO_DRM)
        need_gsc_op_twice = ((dest_w > src_w * max_upscale) ||
                             (dest_h > src_h * max_upscale)) ? true : false;

    if (isUsingMSC() && src_handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED) {
        need_gsc_op_twice = true;
        need_unscaled_csc = true;
    }

    if (need_gsc_op_twice) {
        mid_img.x = 0;
        mid_img.y = 0;

        int mid_w = 0, mid_h = 0;

        if (need_unscaled_csc) {
            mid_img.w = src_w;
            mid_img.h = src_h;
        } else {
            if (rot90or270) {
                mid_w = HEIGHT(layer.displayFrame);
                mid_h = WIDTH(layer.displayFrame);
            } else {
                mid_w = WIDTH(layer.displayFrame);
                mid_h = HEIGHT(layer.displayFrame);
            }

            if (WIDTH(layer.sourceCropf) * max_upscale  < mid_w)
                mid_img.w = (((mid_w + 7) / 8) + 1) & ~1;
            else
                mid_img.w = mid_w;

            if (HEIGHT(layer.sourceCropf) * max_upscale < mid_h)
                mid_img.h = (((mid_h + 7) / 8) + 1) & ~1;
            else
                mid_img.h = mid_h;
        }
        mid_img.drmMode = src_img.drmMode;
        mid_img.format = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;

        if (layer.blending == HWC_BLENDING_PREMULT || layer.blending == HWC_BLENDING_COVERAGE)
            mid_img.pre_multi = false;
        else
            mid_img.pre_multi = true;

        mid_img.mem_type = GSC_MEM_DMABUF;
        mid_img.narrowRgb = !isFormatRgb(src_handle->format);
    }

    return need_gsc_op_twice;
}

void ExynosMPP::setupM2MDestination(exynos_mpp_img &src_img, exynos_mpp_img &dst_img,
        int dst_format, hwc_layer_1_t &layer, hwc_frect_t *sourceCrop)
{
    private_handle_t *src_handle = private_handle_t::dynamicCast(layer.handle);
    dst_img.x = 0;
    dst_img.y = 0;
    dst_img.w = WIDTH(layer.displayFrame);
    dst_img.h = HEIGHT(layer.displayFrame);
    dst_img.rot = layer.transform;
    dst_img.drmMode = src_img.drmMode;
    dst_img.format = dst_format;

    if (layer.blending == HWC_BLENDING_PREMULT || layer.blending == HWC_BLENDING_COVERAGE)
        dst_img.pre_multi = false;
    else
        dst_img.pre_multi = true;

    dst_img.mem_type = GSC_MEM_DMABUF;
    if (src_handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL)
        dst_img.narrowRgb = 0;
    else
        dst_img.narrowRgb = !isFormatRgb(src_handle->format);
    if (dst_img.drmMode)
        alignCropAndCenter(dst_img.w, dst_img.h, sourceCrop,
                GSC_DST_CROP_W_ALIGNMENT_RGB888);
}

int ExynosMPP::reallocateBuffers(private_handle_t *src_handle, exynos_mpp_img &dst_img, exynos_mpp_img &mid_img, bool need_gsc_op_twice)
{
    ATRACE_CALL();
    alloc_device_t* alloc_device = mDisplay->mAllocDevice;
    int ret = 0;
    int dst_stride;
    int usage = GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_NOZEROED |
#ifdef USE_FB_PHY_LINEAR
            ((mIndex == FIMD_GSC_IDX) ? GRALLOC_USAGE_PHYSICALLY_LINEAR : 0) |
#endif
            GRALLOC_USAGE_HW_COMPOSER;

#ifdef USE_FB_PHY_LINEAR
    usage |= GRALLOC_USAGE_PROTECTED;
    usage &= ~GRALLOC_USAGE_PRIVATE_NONSECURE;
#else
    if (getDrmMode(src_handle->flags) == SECURE_DRM) {
        usage |= GRALLOC_USAGE_PROTECTED;
        usage &= ~GRALLOC_USAGE_PRIVATE_NONSECURE;
    } else if (getDrmMode(src_handle->flags) == NORMAL_DRM) {
        usage |= GRALLOC_USAGE_PROTECTED;
        usage |= GRALLOC_USAGE_PRIVATE_NONSECURE;
    }
#endif
    /* HACK: for distinguishing FIMD_VIDEO region  */
    if (!(src_handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN ||
        src_handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN ||
        src_handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_TILED)) {
        usage |= (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER);
    }

    int w, h;
    {
        int dstAlign = destinationAlign(dst_img.format);
        w = ALIGN(mDisplay->mXres, dstAlign);
        h = ALIGN(mDisplay->mYres, dstAlign);
    }

    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
        if (mDstBuffers[i]) {
            android::Mutex::Autolock lock(mMutex);
            mFreedBuffers.push_back(mDstBuffers[i]);
            mDstBuffers[i] = NULL;
        }

        if (mDstBufFence[i] >= 0) {
            close(mDstBufFence[i]);
            mDstBufFence[i] = -1;
        }

        if (mMidBuffers[i] != NULL) {
            android::Mutex::Autolock lock(mMutex);
            mFreedBuffers.push_back(mMidBuffers[i]);
            mMidBuffers[i] = NULL;
        }

        if (mMidBufFence[i] >= 0) {
            close(mMidBufFence[i]);
            mMidBufFence[i] = -1;
        }
    }

    if (getDrmMode(src_handle->flags) != NO_DRM &&
        mDisplay != NULL && mDisplay->mHwc != NULL &&
        (ExynosDisplay *)mDisplay->mHwc->externalDisplay == mDisplay)
        mNumAvailableDstBuffers = NUM_DRM_GSC_DST_BUFS;
    else
        mNumAvailableDstBuffers = NUM_GSC_DST_BUFS;

    for (size_t i = 0; i < mNumAvailableDstBuffers; i++) {
        int format = dst_img.format;
        ret = alloc_device->alloc(alloc_device, w, h,
                format, usage, &mDstBuffers[i],
                &dst_stride);
        if (ret < 0) {
            ALOGE("failed to allocate destination buffer(%dx%d): %s", w, h,
                    strerror(-ret));
            return ret;
        }

        if (need_gsc_op_twice) {
            ret = alloc_device->alloc(alloc_device, mid_img.w, mid_img.h,
                     HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M, usage, &mMidBuffers[i],
                     &dst_stride);
            if (ret < 0) {
                ALOGE("failed to allocate intermediate buffer(%dx%d): %s", mid_img.w, mid_img.h,
                        strerror(-ret));
                return ret;
            }
        }
    }
    {
        android::Mutex::Autolock lock(mMutex);
        mBufferFreeThread->mCondition.signal();
    }
    return ret;
}

#ifdef USES_VIRTUAL_DISPLAY
int ExynosMPP::processM2M(hwc_layer_1_t &layer, int dst_format, hwc_frect_t *sourceCrop, bool isNeedBufferAlloc)
#else
int ExynosMPP::processM2M(hwc_layer_1_t &layer, int dst_format, hwc_frect_t *sourceCrop)
#endif
{
    ATRACE_CALL();
    ALOGV("configuring gscaler %u for memory-to-memory", AVAILABLE_GSC_UNITS[mIndex]);

    alloc_device_t* alloc_device = mDisplay->mAllocDevice;
    private_handle_t *src_handle = private_handle_t::dynamicCast(layer.handle);
    buffer_handle_t dst_buf;
    private_handle_t *dst_handle;
    buffer_handle_t mid_buf;
    private_handle_t *mid_handle;
    int ret = 0;
    int dstAlign;

    if (isUsingMSC()) {
        if (dst_format != HAL_PIXEL_FORMAT_RGB_565)
            dst_format = HAL_PIXEL_FORMAT_BGRA_8888;
    }

    exynos_mpp_img src_img, dst_img;
    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));
    exynos_mpp_img mid_img;
    memset(&mid_img, 0, sizeof(mid_img));

    hwc_frect_t sourceCropTemp;
    if (!sourceCrop)
        sourceCrop = &sourceCropTemp;

    setupSource(src_img, layer);
    src_img.mem_type = GSC_MEM_DMABUF;

    if (isUsingMSC()) {
        if (src_img.format == HAL_PIXEL_FORMAT_RGBA_8888 || src_img.format == HAL_PIXEL_FORMAT_RGBX_8888)
            src_img.format = HAL_PIXEL_FORMAT_BGRA_8888;
    }

    bool need_gsc_op_twice = setupDoubleOperation(src_img, mid_img, layer);

    setupM2MDestination(src_img, dst_img, dst_format, layer, sourceCrop);

#ifdef USES_VIRTUAL_DISPLAY
    if (!isNeedBufferAlloc) {
        dst_img.x = mDisplay->mHwc->mVirtualDisplayRect.left;
        dst_img.y = mDisplay->mHwc->mVirtualDisplayRect.top;
        dst_img.w = mDisplay->mHwc->mVirtualDisplayRect.width;
        dst_img.h = mDisplay->mHwc->mVirtualDisplayRect.height;
    }
#endif

    ALOGV("source configuration:");
    dumpMPPImage(src_img);

    bool reconfigure = isSrcConfigChanged(src_img, mSrcConfig) ||
            isDstConfigChanged(dst_img, mDstConfig);

#ifdef USES_VIRTUAL_DISPLAY
    if (isNeedBufferAlloc) {
#endif
    bool realloc = mDstConfig.fw <= 0 || formatToBpp(mDstConfig.format) != formatToBpp(dst_format);

    /* ext_only andn int_only changes */
    if (!need_gsc_op_twice && getDrmMode(src_handle->flags) == SECURE_DRM) {
        if (dst_img.drmMode != mDstConfig.drmMode)
            realloc = true;
        else
            realloc = false;
    }

    if (need_gsc_op_twice && isSrcConfigChanged(mid_img, mMidConfig))
        realloc = true;

    if (need_gsc_op_twice && !mDoubleOperation)
        realloc = true;
    mDoubleOperation = need_gsc_op_twice;

    if (reconfigure && realloc) {
        if ((ret = reallocateBuffers(src_handle, dst_img, mid_img, need_gsc_op_twice)) < 0)
            goto err_alloc;

        mCurrentBuf = 0;
        mLastGSCLayerHandle = 0;
    }

    if (!reconfigure && (mLastGSCLayerHandle == (ptrdiff_t)layer.handle)) {
        ALOGV("[USE] GSC_SKIP_DUPLICATE_FRAME_PROCESSING\n");
        if (layer.acquireFenceFd >= 0)
            close(layer.acquireFenceFd);

        layer.releaseFenceFd = -1;
        layer.acquireFenceFd = -1;
        mDstConfig.releaseFenceFd = -1;

        mCurrentBuf = (mCurrentBuf + mNumAvailableDstBuffers- 1) % mNumAvailableDstBuffers;
        return 0;
    } else {
        mLastGSCLayerHandle = (ptrdiff_t)layer.handle;
    }
#ifdef USES_VIRTUAL_DISPLAY
    } else {
        if (reconfigure && need_gsc_op_twice) {
            int dst_stride;
            if (mMidBuffers[0] != NULL) {
                android::Mutex::Autolock lock(mMutex);
                mFreedBuffers.push_back(mMidBuffers[0]);
                mBufferFreeThread->mCondition.signal();
                mMidBuffers[0] = NULL;
            }

            if (mMidBufFence[0] >= 0) {
                close(mMidBufFence[0]);
                mMidBufFence[0] = -1;
            }

            ret = alloc_device->alloc(alloc_device, mid_img.w, mid_img.h,
                    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M, ((ExynosVirtualDisplay*)mDisplay)->mSinkUsage, &mMidBuffers[0],
                    &dst_stride);
            if (ret < 0) {
                ALOGE("failed to allocate intermediate buffer(%dx%d): %s", mid_img.w, mid_img.h,
                        strerror(-ret));
                goto err_alloc;
            }
        }
    }
#endif

    layer.acquireFenceFd = -1;
    if (need_gsc_op_twice) {
        mid_img.acquireFenceFd = mMidBufFence[mCurrentBuf];
        mMidBufFence[mCurrentBuf] = -1;
        mid_buf = mMidBuffers[mCurrentBuf];
        mid_handle = private_handle_t::dynamicCast(mid_buf);

        mid_img.fw = mid_handle->stride;
        mid_img.fh = mid_handle->vstride;
        mid_img.yaddr = mid_handle->fd;
        if (isFormatYCrCb(mid_handle->format)) {
            mid_img.uaddr = mid_handle->fd2;
            mid_img.vaddr = mid_handle->fd1;
        } else {
            mid_img.uaddr = mid_handle->fd1;
            mid_img.vaddr = mid_handle->fd2;
        }
        //mid_img.acquireFenceFd = -1;

        ALOGV("mid configuration:");
        dumpMPPImage(mid_img);
    }

    dst_buf = mDstBuffers[mCurrentBuf];
    dst_handle = private_handle_t::dynamicCast(dst_buf);

    dst_img.fw = dst_handle->stride;
    dst_img.fh = dst_handle->vstride;
    dst_img.yaddr = dst_handle->fd;
    dst_img.uaddr = dst_handle->fd1;
    dst_img.vaddr = dst_handle->fd2;
    dst_img.acquireFenceFd = mDstBufFence[mCurrentBuf];
    mDstBufFence[mCurrentBuf] = -1;

    ALOGV("destination configuration:");
    dumpMPPImage(dst_img);

    if ((int)dst_img.w != WIDTH(layer.displayFrame))
        ALOGV("padding %u x %u output to %u x %u and cropping to {%7.1f,%7.1f,%7.1f,%7.1f}",
                WIDTH(layer.displayFrame), HEIGHT(layer.displayFrame),
                dst_img.w, dst_img.h, sourceCrop->left, sourceCrop->top,
                sourceCrop->right, sourceCrop->bottom);

    if (mGscHandle) {
        ALOGV("reusing open gscaler %u", AVAILABLE_GSC_UNITS[mIndex]);
    } else {
        ALOGV("opening gscaler %u", AVAILABLE_GSC_UNITS[mIndex]);
        mGscHandle = createMPP(
                AVAILABLE_GSC_UNITS[mIndex], GSC_M2M_MODE, GSC_DUMMY, (getDrmMode(src_handle->flags) != NO_DRM));
        if (!mGscHandle) {
            ALOGE("failed to create gscaler handle");
            ret = -1;
            goto err_alloc;
        }
    }

    if (!need_gsc_op_twice)
        memcpy(&mid_img, &dst_img, sizeof(exynos_mpp_img));

    /* src -> mid or src->dest */
    if (reconfigure || need_gsc_op_twice) {
        ret = stopMPP(mGscHandle);
        if (ret < 0) {
            ALOGE("failed to stop gscaler %u", mIndex);
            goto err_gsc_config;
        }

        ret = setCSCProperty(mGscHandle, 0, !mid_img.narrowRgb, 1);
        ret = configMPP(mGscHandle, &src_img, &mid_img);
        if (ret < 0) {
            ALOGE("failed to configure gscaler %u", mIndex);
            goto err_gsc_config;
        }
    }

    ret = runMPP(mGscHandle, &src_img, &mid_img);
    if (ret < 0) {
        ALOGE("failed to run gscaler %u", mIndex);
        goto err_gsc_config;
    }

    /* mid -> dst */
    if (need_gsc_op_twice) {
        ret = stopMPP(mGscHandle);
        if (ret < 0) {
            ALOGE("failed to stop gscaler %u", mIndex);
            goto err_gsc_config;
        }

        mid_img.acquireFenceFd = mid_img.releaseFenceFd;

        ret = setCSCProperty(mGscHandle, 0, !dst_img.narrowRgb, 1);
        ret = configMPP(mGscHandle, &mid_img, &dst_img);
        if (ret < 0) {
            ALOGE("failed to configure gscaler %u", mIndex);
            goto err_gsc_config;
        }

        ret = runMPP(mGscHandle, &mid_img, &dst_img);
        if (ret < 0) {
            ALOGE("failed to run gscaler %u", mIndex);
             goto err_gsc_config;
        }
        mMidBufFence[mCurrentBuf] = mid_img.releaseFenceFd;
    }

    mSrcConfig = src_img;
    mMidConfig = mid_img;

    if (need_gsc_op_twice) {
        mDstConfig = dst_img;
    } else {
        mDstConfig = mid_img;
    }

    layer.releaseFenceFd = src_img.releaseFenceFd;

    return 0;

err_gsc_config:
    if (mGscHandle)
        destroyMPP(mGscHandle);
    mGscHandle = NULL;
    libmpp = NULL;
err_alloc:
    if (src_img.acquireFenceFd >= 0)
        close(src_img.acquireFenceFd);
#ifdef USES_VIRTUAL_DISPLAY
    if (isNeedBufferAlloc) {
#endif
    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
       if (mDstBuffers[i]) {
           android::Mutex::Autolock lock(mMutex);
           mFreedBuffers.push_back(mDstBuffers[i]);
           mDstBuffers[i] = NULL;
       }
       if (mDstBufFence[i] >= 0) {
           close(mDstBufFence[i]);
           mDstBufFence[i] = -1;
       }
       if (mMidBuffers[i]) {
           android::Mutex::Autolock lock(mMutex);
           mFreedBuffers.push_back(mMidBuffers[i]);
           mMidBuffers[i] = NULL;
       }
       if (mMidBufFence[i] >= 0) {
           close(mMidBufFence[i]);
           mMidBufFence[i] = -1;
       }
    }
#ifdef USES_VIRTUAL_DISPLAY
    } else {
        if (mMidBuffers[0]) {
            android::Mutex::Autolock lock(mMutex);
            mFreedBuffers.push_back(mMidBuffers[0]);
            mMidBuffers[0] = NULL;
        }
        if (mMidBufFence[0] >= 0) {
            close(mMidBufFence[0]);
            mMidBufFence[0] = -1;
        }
    }
#endif
    {
        android::Mutex::Autolock lock(mMutex);
        mBufferFreeThread->mCondition.signal();
    }
    memset(&mSrcConfig, 0, sizeof(mSrcConfig));
    memset(&mDstConfig, 0, sizeof(mDstConfig));
    memset(&mMidConfig, 0, sizeof(mMidConfig));
    return ret;
}

void ExynosMPP::cleanupM2M()
{
    cleanupM2M(false);
}

void ExynosMPP::cleanupM2M(bool noFenceWait)
{
    ATRACE_CALL();
    if (!mGscHandle)
        return;

    ALOGV("closing gscaler %u", AVAILABLE_GSC_UNITS[mIndex]);

    if (!noFenceWait) {
        for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
#ifndef FORCEFB_YUVLAYER
            if (mDstBufFence[i] >= 0)
                if (sync_wait(mDstBufFence[i], 1000) < 0)
                    ALOGE("sync_wait error");
#endif
            if (mMidBufFence[i] >= 0)
                if (sync_wait(mMidBufFence[i], 1000) < 0)
                    ALOGE("sync_wait error");
        }
    }

    if (mGscHandle) {
        stopMPP(mGscHandle);
        destroyMPP(mGscHandle);
    }
    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
        if (mDstBuffers[i]) {
            android::Mutex::Autolock lock(mMutex);
            mFreedBuffers.push_back(mDstBuffers[i]);
        }
        if (mDstBufFence[i] >= 0)
            close(mDstBufFence[i]);
        if (mMidBuffers[i]) {
            android::Mutex::Autolock lock(mMutex);
            mFreedBuffers.push_back(mMidBuffers[i]);
            mMidBuffers[i] = NULL;
        }
        if (mMidBufFence[i] >= 0)
            close(mMidBufFence[i]);
    }
    {
        android::Mutex::Autolock lock(mMutex);
        mBufferFreeThread->mCondition.signal();
    }

    mGscHandle = NULL;
    libmpp = NULL;
    memset(&mSrcConfig, 0, sizeof(mSrcConfig));
    memset(&mMidConfig, 0, sizeof(mMidConfig));
    memset(&mDstConfig, 0, sizeof(mDstConfig));
    memset(mDstBuffers, 0, sizeof(mDstBuffers));
    memset(mMidBuffers, 0, sizeof(mMidBuffers));
    mCurrentBuf = 0;
    mGSCMode = 0;
    mLastGSCLayerHandle = 0;
    
    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
        mDstBufFence[i] = -1;
        mMidBufFence[i] = -1;
    }
}

void ExynosMPP::cleanupOTF()
{
    if (mGscHandle) {
        stopMPP(mGscHandle);
        freeMPP(mGscHandle);
    }

    mNeedReqbufs = false;
    mWaitVsyncCount = 0;
    mDisplay->mOtfMode = OTF_OFF;
    mGscHandle = NULL;
    libmpp = NULL;
    memset(&mSrcConfig, 0, sizeof(mSrcConfig));
    memset(&mMidConfig, 0, sizeof(mMidConfig));
    memset(&mDstConfig, 0, sizeof(mDstConfig));
    memset(mDstBuffers, 0, sizeof(mDstBuffers));
    memset(mMidBuffers, 0, sizeof(mMidBuffers));
    mCurrentBuf = 0;
    mGSCMode = 0;
    mLastGSCLayerHandle = 0;

    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
        mDstBufFence[i] = -1;
        mMidBufFence[i] = -1;
    }
}

bool ExynosMPP::rotationSupported(bool rotation)
{
    return !rotation;
}

bool ExynosMPP::paritySupported(int w, int h)
{
    return (w % 2 == 0) && (h % 2 == 0);
}

bool ExynosMPP::isDstConfigChanged(exynos_mpp_img &c1, exynos_mpp_img &c2)
{
    return c1.x != c2.x ||
            c1.y != c2.y ||
            c1.w != c2.w ||
            c1.h != c2.h ||
            c1.format != c2.format ||
            c1.rot != c2.rot ||
            c1.narrowRgb != c2.narrowRgb ||
            c1.cacheable != c2.cacheable ||
            c1.pre_multi != c2.pre_multi ||
            c1.drmMode != c2.drmMode;
}

bool ExynosMPP::isSourceCropChanged(exynos_mpp_img &c1, exynos_mpp_img &c2)
{
    return false;
}

bool ExynosMPP::isPerFrameSrcChanged(exynos_mpp_img &c1, exynos_mpp_img &c2)
{
    return false;
}

bool ExynosMPP::isPerFrameDstChanged(exynos_mpp_img &c1, exynos_mpp_img &c2)
{
    return false;
}

bool ExynosMPP::isReallocationRequired(int w, int h, exynos_mpp_img &c1, exynos_mpp_img &c2)
{
    return ALIGN(w, GSC_W_ALIGNMENT) != ALIGN(c2.fw, GSC_W_ALIGNMENT) ||
            ALIGN(h, GSC_H_ALIGNMENT) != ALIGN(c2.fh, GSC_H_ALIGNMENT) ||
            c1.format != c2.format ||
            c1.drmMode != c2.drmMode;
}

uint32_t ExynosMPP::halFormatToMPPFormat(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return HAL_PIXEL_FORMAT_BGRA_8888;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return HAL_PIXEL_FORMAT_RGBA_8888;
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
        return HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;
    default:
        return format;
    }
}

int ExynosMPP::minWidth(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (handle->format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
        return isRotated(layer) ? 16 : 32;
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    default:
        return isRotated(layer) ? 32 : 64;
    }
}

int ExynosMPP::minHeight(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (handle->format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
        return isRotated(layer) ? 32 : 16;
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    default:
        return isRotated(layer) ? 64 : 32;
    }
}

int ExynosMPP::maxWidth(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    if (isUsingMSC())
        return 8192;

    switch (handle->format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
        return 4800;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    default:
        return 2047;
    }
}

int ExynosMPP::maxHeight(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    if (isUsingMSC())
        return 8192;

    switch (handle->format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
        return 3344;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    default:
        return 2047;
    }
}

int ExynosMPP::srcXOffsetAlign(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (handle->format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
        return 1;
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    default:
        return 2;
    }
}

int ExynosMPP::srcYOffsetAlign(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (handle->format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
        return 1;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    default:
        return isRotated(layer) ? 2 : 1;
    }
}

void *ExynosMPP::createMPP(int id, int mode, int outputMode, int drm)
{
    ATRACE_CALL();
    mppFact = new MppFactory();
    libmpp = mppFact->CreateMpp(id, mode, outputMode, drm);

    return reinterpret_cast<void *>(libmpp);
}

int ExynosMPP::configMPP(void *handle, exynos_mpp_img *src, exynos_mpp_img *dst)
{
    ATRACE_CALL();
    return libmpp->ConfigMpp(handle, src, dst);
}

int ExynosMPP::runMPP(void *handle, exynos_mpp_img *src, exynos_mpp_img *dst)
{
    ATRACE_CALL();
    return libmpp->RunMpp(handle, src, dst);
}

int ExynosMPP::stopMPP(void *handle)
{
    ATRACE_CALL();
    return libmpp->StopMpp(handle);
}

void ExynosMPP::destroyMPP(void *handle)
{
    ATRACE_CALL();
    libmpp->DestroyMpp(handle);
    delete(mppFact);
}

int ExynosMPP::setCSCProperty(void *handle, unsigned int eqAuto, unsigned int fullRange, unsigned int colorspace)
{
    return libmpp->SetCSCProperty(handle, eqAuto, fullRange, colorspace);
}

int ExynosMPP::freeMPP(void *handle)
{
    ATRACE_CALL();
    return libmpp->FreeMpp(handle);
}

int ExynosMPP::setInputCrop(void *handle, exynos_mpp_img *src, exynos_mpp_img *dst)
{
    return libmpp->SetInputCrop(handle, src, dst);
}

bool ExynosMPP::bufferChanged(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    return mSrcConfig.fw != (uint32_t)handle->stride ||
        mSrcConfig.fh != (uint32_t)handle->vstride ||
        mSrcConfig.format != (uint32_t)handle->format ||
        mDstConfig.rot != (uint32_t)layer.transform;
}

bool ExynosMPP::needsReqbufs()
{
    return mNeedReqbufs;
}

bool ExynosMPP::inUse()
{
    return mGscHandle != NULL;
}

void ExynosMPP::freeBuffers()
{
    alloc_device_t* alloc_device = mDisplay->mAllocDevice;
    android::List<buffer_handle_t >::iterator it;
    android::List<buffer_handle_t >::iterator end;
    {
        android::Mutex::Autolock lock(mMutex);
        it = mFreedBuffers.begin();
        end = mFreedBuffers.end();
    }
    while (it != end) {
        buffer_handle_t buffer = (buffer_handle_t)(*it);
        alloc_device->free(alloc_device, buffer);
        {
            android::Mutex::Autolock lock(mMutex);
            it = mFreedBuffers.erase(it);
        }
    }
}

bool BufferFreeThread::threadLoop()
{
    while(mRunning) {
        {
            android::Mutex::Autolock lock(mExynosMPP->mMutex);
            while(mExynosMPP->mFreedBuffers.size() == 0) {
                mCondition.wait(mExynosMPP->mMutex);
            }
        }
        mExynosMPP->freeBuffers();
    }
    return true;
}

bool ExynosMPP::isSrcCropAligned(hwc_layer_1_t &layer, bool rotation)
{
    return !rotation ||
        ((uint32_t)layer.sourceCropf.left % srcXOffsetAlign(layer) == 0 &&
         (uint32_t)layer.sourceCropf.top % srcYOffsetAlign(layer) == 0);
}
