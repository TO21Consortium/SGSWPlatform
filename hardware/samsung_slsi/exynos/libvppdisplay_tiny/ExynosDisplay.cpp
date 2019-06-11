#define ATRACE_TAG ATRACE_TAG_GRAPHICS

//#define LOG_NDEBUG 0
#define LOG_TAG "display"
#include "ExynosDisplay.h"

#include <utils/misc.h>
#include <utils/Trace.h>
#include <inttypes.h>

#define DISPLAY_LOGD(msg, ...) ALOGD("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGV(msg, ...) ALOGV("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGI(msg, ...) ALOGI("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGW(msg, ...) ALOGW("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGE(msg, ...) ALOGE("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)

uint8_t formatToBpp(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
#ifdef EXYNOS_SUPPORT_BGRX_8888
    case HAL_PIXEL_FORMAT_BGRX_8888:
#endif
        return 32;
    case HAL_PIXEL_FORMAT_RGB_565:
        return 16;
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
        return 12;
    default:
        ALOGW("unrecognized pixel format %u", format);
        return 0;
    }
}

bool frameChanged(decon_frame *f1, decon_frame *f2)
{
    return f1->x != f2->x ||
            f1->y != f2->y ||
            f1->w != f2->w ||
            f1->h != f2->h ||
            f1->f_w != f2->f_w ||
            f1->f_h != f2->f_h;
}

bool winConfigChanged(decon_win_config *c1, decon_win_config *c2)
{
    return c1->state != c2->state ||
            c1->fd_idma[0] != c2->fd_idma[0] ||
            c1->fd_idma[1] != c2->fd_idma[1] ||
            c1->fd_idma[2] != c2->fd_idma[2] ||
            frameChanged(&c1->src, &c2->src) ||
            frameChanged(&c1->dst, &c2->dst) ||
            c1->format != c2->format ||
            c1->blending != c2->blending ||
            c1->plane_alpha != c2->plane_alpha;
}

void ExynosDisplay::dumpConfig(decon_win_config &c)
{
    DISPLAY_LOGV("\tstate = %u", c.state);
    if (c.state == c.DECON_WIN_STATE_BUFFER) {
        DISPLAY_LOGV("\t\tfd = %d, dma = %u "
                "src_f_w = %u, src_f_h = %u, src_x = %d, src_y = %d, src_w = %u, src_h = %u, "
                "dst_f_w = %u, dst_f_h = %u, dst_x = %d, dst_y = %d, dst_w = %u, dst_h = %u, "
                "format = %u, blending = %u, protection = %u",
                c.fd_idma[0], c.idma_type,
                c.src.f_w, c.src.f_h, c.src.x, c.src.y, c.src.w, c.src.h,
                c.dst.f_w, c.dst.f_h, c.dst.x, c.dst.y, c.dst.w, c.dst.h,
                c.format, c.blending, c.protection);
    }
    else if (c.state == c.DECON_WIN_STATE_COLOR) {
        DISPLAY_LOGV("\t\tcolor = %u", c.color);
    }
}

enum decon_pixel_format halFormatToS3CFormat(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return DECON_PIXEL_FORMAT_RGBA_8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return DECON_PIXEL_FORMAT_RGBX_8888;
    case HAL_PIXEL_FORMAT_RGB_565:
        return DECON_PIXEL_FORMAT_RGB_565;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return DECON_PIXEL_FORMAT_BGRA_8888;
#ifdef EXYNOS_SUPPORT_BGRX_8888
    case HAL_PIXEL_FORMAT_BGRX_8888:
        return DECON_PIXEL_FORMAT_BGRX_8888;
#endif
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
        return DECON_PIXEL_FORMAT_YVU420M;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        return DECON_PIXEL_FORMAT_YUV420M;
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
        return DECON_PIXEL_FORMAT_NV21M;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        return DECON_PIXEL_FORMAT_NV21;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        return DECON_PIXEL_FORMAT_NV12M;
    default:
        return DECON_PIXEL_FORMAT_MAX;
    }
}

bool isFormatSupported(int format)
{
    return halFormatToS3CFormat(format) < DECON_PIXEL_FORMAT_MAX;
}

enum decon_blending halBlendingToS3CBlending(int32_t blending)
{
    switch (blending) {
    case HWC_BLENDING_NONE:
        return DECON_BLENDING_NONE;
    case HWC_BLENDING_PREMULT:
        return DECON_BLENDING_PREMULT;
    case HWC_BLENDING_COVERAGE:
        return DECON_BLENDING_COVERAGE;

    default:
        return DECON_BLENDING_MAX;
    }
}

bool isBlendingSupported(int32_t blending)
{
    return halBlendingToS3CBlending(blending) < DECON_BLENDING_MAX;
}

#define NUMA(a) (sizeof(a) / sizeof(a [0]))
const char *deconFormat2str(uint32_t format)
{
    android::String8 result;

    for (unsigned int n1 = 0; n1 < NUMA(deconFormat); n1++) {
        if (format == deconFormat[n1].format) {
            return deconFormat[n1].desc;
        }
    }

    result.appendFormat("? %08x", format);
    return result;
}

ExynosDisplay::ExynosDisplay(int numGSCs)
    :   mDisplayFd(-1),
        mType(0),
        mXres(0),
        mYres(0),
        mXdpi(0),
        mYdpi(0),
        mVsyncPeriod(0),
        mBlanked(true),
        mHwc(NULL)

{


}
ExynosDisplay::ExynosDisplay(uint32_t type, struct exynos5_hwc_composer_device_1_t *pdev)
    :   mDisplayFd(-1),
        mType(type),
        mXres(0),
        mYres(0),
        mXdpi(0),
        mYdpi(0),
        mVsyncPeriod(0),
        mBlanked(true),
        mHwc(pdev)

{

    switch (mType) {
    case EXYNOS_VIRTUAL_DISPLAY:
        mDisplayName = android::String8("VirtualDisplay");
        break;
    case EXYNOS_EXTERNAL_DISPLAY:
        mDisplayName = android::String8("ExternalDisplay");
        break;
    case EXYNOS_PRIMARY_DISPLAY:
    default:
        mDisplayName = android::String8("PrimaryDisplay");
    }

}

ExynosDisplay::~ExynosDisplay()
{
}


int ExynosDisplay::prepare(hwc_display_contents_1_t *contents)
{
    ATRACE_CALL();
    DISPLAY_LOGV("preparing %u layers for FIMD", contents->numHwLayers);

    mFbNeeded = false;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        //ALOGD("### ExynosDisplay::prepare [%d/%d] - compositionType %d", i, contents->numHwLayers, layer.compositionType);
        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            mFbNeeded = true;
        } else if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            ALOGD("### ExynosDisplay::prepare [%d/%d] - OVERLAY layer detected. changing into GLES", i, contents->numHwLayers);
            layer.compositionType = HWC_FRAMEBUFFER;
        }
    }


    return 0;
}

int ExynosDisplay::set(hwc_display_contents_1_t *contents)
{
    hwc_layer_1_t *fb_layer = NULL;
    int err = 0;

    if (mFbNeeded) {
        if (contents->numHwLayers >= 1 &&
                contents->hwLayers[contents->numHwLayers - 1].compositionType == HWC_FRAMEBUFFER_TARGET) {
            fb_layer = &contents->hwLayers[contents->numHwLayers - 1];
        }
        if (CC_UNLIKELY(!fb_layer)) {
            DISPLAY_LOGE("framebuffer target expected, but not provided");
            err = -EINVAL;
        } else {
            DISPLAY_LOGV("framebuffer target buffer:");
        }
    }


    int fence;
    if (!err) {
        fence = postFrame(contents);
        if (fence < 0)
            err = fence;
    }

    if (err)
        fence = clearDisplay();

    if (fence == 0) {
        /*
         * Only happens in mLocalExternalDisplayPause scenario, changing S3D mode.
         * not error
         */
        fence = -1;
    } else {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];


            if (!(layer.flags & HWC_SKIP_RENDERING) && (mFbNeeded == true && layer.compositionType == HWC_FRAMEBUFFER_TARGET)) {
                int dup_fd = dup(fence);
                if (dup_fd < 0)
                    ALOGD("release fence dup failed: %s", strerror(errno));
                layer.releaseFenceFd = dup_fd;
            }
        }
    }

    contents->retireFenceFd = fence;

    return err;
}

void ExynosDisplay::dump(android::String8& result)
{
    return;
}


void ExynosDisplay::dumpLayerInfo(android::String8& result)
{
    result.append("\n");
}


int ExynosDisplay::clearDisplay()
{
    struct decon_win_config_data win_data;
    memset(&win_data, 0, sizeof(win_data));

    int ret = ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, &win_data);
    LOG_ALWAYS_FATAL_IF(ret < 0,
            "%s ioctl S3CFB_WIN_CONFIG failed to clear screen: %s",
            mDisplayName.string(), strerror(errno));
    // the causes of an empty config failing are all unrecoverable

    return win_data.fence;
}


int32_t ExynosDisplay::getDisplayAttributes(const uint32_t attribute)
{
    switch(attribute) {
    case HWC_DISPLAY_VSYNC_PERIOD:
        return this->mVsyncPeriod;

    case HWC_DISPLAY_WIDTH:
        return this->mXres;

    case HWC_DISPLAY_HEIGHT:
        return this->mYres;

    case HWC_DISPLAY_DPI_X:
        return this->mXdpi;

    case HWC_DISPLAY_DPI_Y:
        return this->mYdpi;

    default:
        DISPLAY_LOGE("unknown display attribute %u", attribute);
        return -EINVAL;
    }
}

void ExynosDisplay::configureHandle(private_handle_t *handle, size_t index,
        hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg)
{
    hwc_frect_t &sourceCrop = layer.sourceCropf;
    hwc_rect_t &displayFrame = layer.displayFrame;
    int32_t blending = layer.blending;
    int32_t planeAlpha = layer.planeAlpha;
    uint32_t x, y;
    uint32_t w = WIDTH(displayFrame);
    uint32_t h = HEIGHT(displayFrame);
    uint8_t bpp = formatToBpp(handle->format);
    uint32_t offset = ((uint32_t)sourceCrop.top * handle->stride + (uint32_t)sourceCrop.left) * bpp / 8;
    if (displayFrame.left < 0) {
        unsigned int crop = -displayFrame.left;
        DISPLAY_LOGV("layer off left side of screen; cropping %u pixels from left edge",
                crop);
        x = 0;
        w -= crop;
        offset += crop * bpp / 8;
    } else {
        x = displayFrame.left;
    }

    if (displayFrame.right > this->mXres) {
        unsigned int crop = displayFrame.right - this->mXres;
        DISPLAY_LOGV("layer off right side of screen; cropping %u pixels from right edge",
                crop);
        w -= crop;
    }

    if (displayFrame.top < 0) {
        unsigned int crop = -displayFrame.top;
        DISPLAY_LOGV("layer off top side of screen; cropping %u pixels from top edge",
                crop);
        y = 0;
        h -= crop;
        offset += handle->stride * crop * bpp / 8;
    } else {
        y = displayFrame.top;
    }

    if (displayFrame.bottom > this->mYres) {
        int crop = displayFrame.bottom - this->mYres;
        DISPLAY_LOGV("layer off bottom side of screen; cropping %u pixels from bottom edge",
                crop);
        h -= crop;
    }
    cfg.state = cfg.DECON_WIN_STATE_BUFFER;
    cfg.fd_idma[0] = handle->fd;
    cfg.fd_idma[1] = handle->fd1;
    cfg.fd_idma[2] = handle->fd2;
    cfg.idma_type = IDMA_G1;
    cfg.dst.x = x;
    cfg.dst.y = y;
    cfg.dst.w = w;
    cfg.dst.h = h;
    cfg.dst.f_w = w;
    cfg.dst.f_h = h;
    cfg.format = halFormatToS3CFormat(handle->format);

    cfg.src.f_w = handle->stride;
    cfg.src.f_h = handle->vstride;
    cfg.src.x = (int)sourceCrop.left;
    cfg.src.y = (int)sourceCrop.top;

    cfg.src.w = WIDTH(sourceCrop) - (cfg.src.x - (uint32_t)sourceCrop.left);
    if (cfg.src.x + cfg.src.w > cfg.src.f_w)
        cfg.src.w = cfg.src.f_w - cfg.src.x;
    cfg.src.h = HEIGHT(sourceCrop) - (cfg.src.y - (uint32_t)sourceCrop.top);
    if (cfg.src.y + cfg.src.h > cfg.src.f_h)
        cfg.src.h = cfg.src.f_h - cfg.src.y;
    cfg.blending = halBlendingToS3CBlending(blending);
    cfg.fence_fd = fence_fd;
    cfg.plane_alpha = 255;
    if (planeAlpha && (planeAlpha < 255)) {
        cfg.plane_alpha = planeAlpha;
    }
}

void ExynosDisplay::configureOverlay(hwc_layer_1_t *layer, size_t index, decon_win_config &cfg)
{
    if (layer->compositionType == HWC_BACKGROUND) {
        hwc_color_t color = layer->backgroundColor;
        cfg.state = cfg.DECON_WIN_STATE_COLOR;
        cfg.color = (color.r << 16) | (color.g << 8) | color.b;
        cfg.dst.x = 0;
        cfg.dst.y = 0;
        cfg.dst.w = this->mXres;
        cfg.dst.h = this->mYres;
        return;
    }
    private_handle_t *handle = private_handle_t::dynamicCast(layer->handle);
    configureHandle(handle, index, *layer, layer->acquireFenceFd, cfg);
}


int ExynosDisplay::winconfigIoctl(decon_win_config_data *win_data)
{
    ATRACE_CALL();
    return ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, win_data);
}

int ExynosDisplay::postFrame(hwc_display_contents_1_t* contents)
{
    ATRACE_CALL();
    struct decon_win_config_data win_data;
    struct decon_win_config *config = win_data.config;
    int win_map = 0;
    memset(config, 0, sizeof(win_data.config));

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        config[i].fence_fd = -1;
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        size_t window_index = 0;


        if (layer.flags & HWC_SKIP_RENDERING) {
            if (layer.acquireFenceFd >= 0)
                close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
            layer.releaseFenceFd = -1;
            continue;
        }


        if (mFbNeeded == true &&  layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

            config[window_index].protection = 0;

            configureOverlay(&layer, i, config[window_index]);
        }
        if (window_index == 0 && config[window_index].blending != DECON_BLENDING_NONE) {
            DISPLAY_LOGV("blending not supported on window 0; forcing BLENDING_NONE");
            config[window_index].blending = DECON_BLENDING_NONE;
        }
    }

    int ret = winconfigIoctl(&win_data);
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        if (config[i].fence_fd != -1)
            close(config[i].fence_fd);

    if (ret < 0) {
        DISPLAY_LOGE("ioctl S3CFB_WIN_CONFIG failed: %s", strerror(errno));
        return ret;
    }
    if (contents->numHwLayers == 1) {
        hwc_layer_1_t &layer = contents->hwLayers[0];
        if (layer.acquireFenceFd >= 0)
            close(layer.acquireFenceFd);
        layer.acquireFenceFd = -1;
        layer.releaseFenceFd = -1;
    }


    return win_data.fence;
}

