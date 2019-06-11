#include "ExynosHWCUtils.h"
#include "ExynosHWCDebug.h"

int hwcDebug;

uint32_t hwcApiVersion(const hwc_composer_device_1_t* hwc) {
    uint32_t hwcVersion = hwc->common.version;
    return hwcVersion & HARDWARE_API_VERSION_2_MAJ_MIN_MASK;
}

uint32_t hwcHeaderVersion(const hwc_composer_device_1_t* hwc) {
    uint32_t hwcVersion = hwc->common.version;
    return hwcVersion & HARDWARE_API_VERSION_2_HEADER_MASK;
}

bool hwcHasApiVersion(const hwc_composer_device_1_t* hwc,
        uint32_t version) {
    return hwcApiVersion(hwc) >= (version & HARDWARE_API_VERSION_2_MAJ_MIN_MASK);
}
void dumpHandle(private_handle_t *h)
{
    ALOGV("\t\tformat = %d, width = %u, height = %u, stride = %u, vstride = %u",
            h->format, h->width, h->height, h->stride, h->vstride);
}

void dumpHandle(uint32_t type, private_handle_t *h)
{
    HDEBUGLOGD(type, "\t\tformat = %d, width = %u, height = %u, stride = %u, vstride = %u",
            h->format, h->width, h->height, h->stride, h->vstride);
}

void dumpLayer(hwc_layer_1_t const *l)
{
    ALOGV("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, "
            "{%7.1f,%7.1f,%7.1f,%7.1f}, {%d,%d,%d,%d}",
            l->compositionType, l->flags, l->handle, l->transform,
            l->blending,
            l->sourceCropf.left,
            l->sourceCropf.top,
            l->sourceCropf.right,
            l->sourceCropf.bottom,
            l->displayFrame.left,
            l->displayFrame.top,
            l->displayFrame.right,
            l->displayFrame.bottom);

    if(l->handle && !(l->flags & HWC_SKIP_LAYER))
        dumpHandle(private_handle_t::dynamicCast(l->handle));
}

void dumpLayer(uint32_t type, hwc_layer_1_t const *l)
{
    HDEBUGLOGD(type, "\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, "
            "{%7.1f,%7.1f,%7.1f,%7.1f}, {%d,%d,%d,%d}",
            l->compositionType, l->flags, l->handle, l->transform,
            l->blending,
            l->sourceCropf.left,
            l->sourceCropf.top,
            l->sourceCropf.right,
            l->sourceCropf.bottom,
            l->displayFrame.left,
            l->displayFrame.top,
            l->displayFrame.right,
            l->displayFrame.bottom);

    if(l->handle && !(l->flags & HWC_SKIP_LAYER))
        dumpHandle(type, private_handle_t::dynamicCast(l->handle));
}

void dumpMPPImage(exynos_mpp_img &c)
{
    ALOGV("\tx = %u, y = %u, w = %u, h = %u, fw = %u, fh = %u",
            c.x, c.y, c.w, c.h, c.fw, c.fh);
    ALOGV("\tf = %u", c.format);
    ALOGV("\taddr = {%d, %d, %d}, rot = %u, cacheable = %u, drmMode = %u",
            c.yaddr, c.uaddr, c.vaddr, c.rot, c.cacheable, c.drmMode);
    ALOGV("\tnarrowRgb = %u, acquireFenceFd = %d, releaseFenceFd = %d, mem_type = %u",
            c.narrowRgb, c.acquireFenceFd, c.releaseFenceFd, c.mem_type);
}

void dumpMPPImage(uint32_t type, exynos_mpp_img &c)
{
    HDEBUGLOGD(type, "\tx = %u, y = %u, w = %u, h = %u, fw = %u, fh = %u",
            c.x, c.y, c.w, c.h, c.fw, c.fh);
    HDEBUGLOGD(type, "\tf = %u", c.format);
    HDEBUGLOGD(type, "\taddr = {%d, %d, %d}, rot = %u, cacheable = %u, drmMode = %u",
            c.yaddr, c.uaddr, c.vaddr, c.rot, c.cacheable, c.drmMode);
    HDEBUGLOGD(type, "\tnarrowRgb = %u, acquireFenceFd = %d, releaseFenceFd = %d, mem_type = %u",
            c.narrowRgb, c.acquireFenceFd, c.releaseFenceFd, c.mem_type);
}

#ifndef USES_FIMC
void dumpBlendMPPImage(struct SrcBlendInfo &c)
{
    ALOGV("\tblop = %d, srcblendfmt = %u, srcblendpremulti = %u\n",
        c.blop, c.srcblendfmt, c.srcblendpremulti);
    ALOGV("\tx = %u, y = %u, w = %u, h = %u, fw = %u\n",
        c.srcblendhpos, c.srcblendvpos, c.srcblendwidth,
        c.srcblendheight, c.srcblendstride);
    ALOGV("\tglobalalpha = %d, tglobalalpha.val = %u, cscspec = %d, cscspec.space = %u, cscspec.wide = %u\n",
        c.globalalpha.enable, c.globalalpha.val, c.cscspec.enable,
        c.cscspec.space, c.cscspec.wide);
}
void dumpBlendMPPImage(uint32_t type, struct SrcBlendInfo &c)
{
    HDEBUGLOGD(type, "\tblop = %d, srcblendfmt = %u, srcblendpremulti = %u\n",
        c.blop, c.srcblendfmt, c.srcblendpremulti);
    HDEBUGLOGD(type, "\tx = %u, y = %u, w = %u, h = %u, fw = %u\n",
        c.srcblendhpos, c.srcblendvpos, c.srcblendwidth,
        c.srcblendheight, c.srcblendstride);
    HDEBUGLOGD(type, "\tglobalalpha = %d, tglobalalpha.val = %u, cscspec = %d, cscspec.space = %u, cscspec.wide = %u\n",
        c.globalalpha.enable, c.globalalpha.val, c.cscspec.enable,
        c.cscspec.space, c.cscspec.wide);
}
#endif

bool isDstCropWidthAligned(int dest_w)
{
    int dst_crop_w_alignement;

   /* GSC's dst crop size should be aligned 128Bytes */
    dst_crop_w_alignement = GSC_DST_CROP_W_ALIGNMENT_RGB888;

    return (dest_w % dst_crop_w_alignement) == 0;
}

bool isTransformed(const hwc_layer_1_t &layer)
{
    return layer.transform != 0;
}

bool isRotated(const hwc_layer_1_t &layer)
{
    return (layer.transform & HAL_TRANSFORM_ROT_90) ||
            (layer.transform & HAL_TRANSFORM_ROT_180);
}

bool isScaled(const hwc_layer_1_t &layer)
{
    return WIDTH(layer.displayFrame) != WIDTH(layer.sourceCropf) ||
            HEIGHT(layer.displayFrame) != HEIGHT(layer.sourceCropf);
}

bool isFormatRgb(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_BGRA_8888:
#ifdef EXYNOS_SUPPORT_BGRX_8888
    case HAL_PIXEL_FORMAT_BGRX_8888:
#endif
        return true;

    default:
        return false;
    }
}

bool isFormatYUV420(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_TILED:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
        return true;
    default:
        return false;
    }
}

bool isFormatYUV422(int __unused format)
{
    // Might add support later
    return false;
}

bool isFormatYCrCb(int format)
{
    return format == HAL_PIXEL_FORMAT_EXYNOS_YV12_M;
}

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
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_TILED:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
        return 12;
    default:
        ALOGW("unrecognized pixel format %u", format);
        return 0;
    }
}

int getDrmMode(int flags)
{
    if (flags & GRALLOC_USAGE_PROTECTED) {
        if (flags & GRALLOC_USAGE_PRIVATE_NONSECURE)
            return NORMAL_DRM;
        else
            return SECURE_DRM;
    } else {
        return NO_DRM;
    }
}

int halFormatToV4L2Format(int format)
{
#ifdef EXYNOS_SUPPORT_BGRX_8888
    if (format == HAL_PIXEL_FORMAT_BGRX_8888)
        return HAL_PIXEL_FORMAT_2_V4L2_PIX(HAL_PIXEL_FORMAT_RGBX_8888);
    else
#endif
        return HAL_PIXEL_FORMAT_2_V4L2_PIX(format);
}

bool isOffscreen(hwc_layer_1_t &layer, int xres, int yres)
{
    return layer.displayFrame.left < 0 ||
        layer.displayFrame.top < 0 ||
        layer.displayFrame.right > xres ||
        layer.displayFrame.bottom > yres;
}

bool isSrcCropFloat(hwc_frect &frect)
{
    return (frect.left != (int)frect.left) ||
        (frect.top != (int)frect.top) ||
        (frect.right != (int)frect.right) ||
        (frect.bottom != (int)frect.bottom);
}

bool isFloat(float f)
{
    return (f != floorf(f));
}

bool isUHD(const hwc_layer_1_t &layer)
{
    return (WIDTH(layer.sourceCropf) >= UHD_WIDTH &&
            HEIGHT(layer.sourceCropf) >= UHD_HEIGHT);
}

bool isFullRangeColor(const hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    return (handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL);
}

bool isCompressed(const hwc_layer_1_t &layer)
{
    if (layer.handle) {
        private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
        if (handle->internal_format & ((uint64_t)(1) << 32))
            return true;
    }
    return false;
}

bool compareYuvLayerConfig(int videoLayers, uint32_t index,
        hwc_layer_1_t &layer,
        video_layer_config *pre_src_data, video_layer_config *pre_dst_data)
{
    private_handle_t *src_handle = private_handle_t::dynamicCast(layer.handle);
    buffer_handle_t dst_buf;
    private_handle_t *dst_handle;
    int ret = 0;
    bool reconfigure = 1;

    video_layer_config new_src_cfg, new_dst_cfg;
    memset(&new_src_cfg, 0, sizeof(new_src_cfg));
    memset(&new_dst_cfg, 0, sizeof(new_dst_cfg));

    new_src_cfg.x = (int)layer.sourceCropf.left;
    new_src_cfg.y = (int)layer.sourceCropf.top;
    new_src_cfg.w = WIDTH(layer.sourceCropf);
    new_src_cfg.fw = src_handle->stride;
    new_src_cfg.h = HEIGHT(layer.sourceCropf);
    new_src_cfg.fh = src_handle->vstride;
    new_src_cfg.format = src_handle->format;
    new_src_cfg.drmMode = !!(getDrmMode(src_handle->flags) == SECURE_DRM);
    new_src_cfg.index = index;

    new_dst_cfg.x = layer.displayFrame.left;
    new_dst_cfg.y = layer.displayFrame.top;
    new_dst_cfg.w = WIDTH(layer.displayFrame);
    new_dst_cfg.h = HEIGHT(layer.displayFrame);
    new_dst_cfg.rot = layer.transform;
    new_dst_cfg.drmMode = new_src_cfg.drmMode;

    /* check to save previous yuv layer configration */
    if (pre_src_data && pre_dst_data) {
         reconfigure = yuvConfigChanged(new_src_cfg, pre_src_data[videoLayers]) ||
            yuvConfigChanged(new_dst_cfg, pre_dst_data[videoLayers]);
    } else {
        ALOGE("Invalid parameter");
        return reconfigure;
    }

    memcpy(&pre_src_data[videoLayers], &new_src_cfg, sizeof(new_src_cfg));
    memcpy(&pre_dst_data[videoLayers], &new_dst_cfg, sizeof(new_dst_cfg));

    return reconfigure;

}

size_t getRequiredPixels(hwc_layer_1_t &layer, int xres, int yres)
{
    uint32_t w = WIDTH(layer.displayFrame);
    uint32_t h = HEIGHT(layer.displayFrame);
    if (layer.displayFrame.left < 0) {
        unsigned int crop = -layer.displayFrame.left;
        w -= crop;
    }

    if (layer.displayFrame.right > xres) {
        unsigned int crop = layer.displayFrame.right - xres;
        w -= crop;
    }

    if (layer.displayFrame.top < 0) {
        unsigned int crop = -layer.displayFrame.top;
        h -= crop;
    }

    if (layer.displayFrame.bottom > yres) {
        int crop = layer.displayFrame.bottom - yres;
        h -= crop;
    }
    return w*h;
}

/* OFF_Screen to ON_Screen changes */
void recalculateDisplayFrame(hwc_layer_1_t &layer, int xres, int yres)
{
    uint32_t x, y;
    uint32_t w = WIDTH(layer.displayFrame);
    uint32_t h = HEIGHT(layer.displayFrame);

    if (layer.displayFrame.left < 0) {
        unsigned int crop = -layer.displayFrame.left;
        ALOGV("layer off left side of screen; cropping %u pixels from left edge",
                crop);
        HDEBUGLOGD(eDebugDefault, "layer off left side of screen; cropping %u pixels from left edge",
                crop);
        x = 0;
        w -= crop;
    } else {
        x = layer.displayFrame.left;
    }

    if (layer.displayFrame.right > xres) {
        unsigned int crop = layer.displayFrame.right - xres;
        ALOGV("layer off right side of screen; cropping %u pixels from right edge",
                crop);
        HDEBUGLOGD(eDebugDefault, "layer off right side of screen; cropping %u pixels from right edge",
                crop);
        w -= crop;
    }

    if (layer.displayFrame.top < 0) {
        unsigned int crop = -layer.displayFrame.top;
        ALOGV("layer off top side of screen; cropping %u pixels from top edge",
                crop);
        HDEBUGLOGD(eDebugDefault, "layer off top side of screen; cropping %u pixels from top edge",
                crop);
        y = 0;
        h -= crop;
    } else {
        y = layer.displayFrame.top;
    }

    if (layer.displayFrame.bottom > yres) {
        int crop = layer.displayFrame.bottom - yres;
        ALOGV("layer off bottom side of screen; cropping %u pixels from bottom edge",
                crop);
        HDEBUGLOGD(eDebugDefault, "layer off bottom side of screen; cropping %u pixels from bottom edge",
                crop);
        h -= crop;
    }

    layer.displayFrame.left = x;
    layer.displayFrame.top = y;
    layer.displayFrame.right = w + x;
    layer.displayFrame.bottom = h + y;
}

int getS3DFormat(int preset)
{
    switch (preset) {
#ifndef HDMI_INCAPABLE
    case V4L2_DV_720P60_SB_HALF:
    case V4L2_DV_720P50_SB_HALF:
    case V4L2_DV_1080P60_SB_HALF:
    case V4L2_DV_1080P30_SB_HALF:
        return S3D_SBS;
    case V4L2_DV_720P60_TB:
    case V4L2_DV_720P50_TB:
    case V4L2_DV_1080P60_TB:
    case V4L2_DV_1080P30_TB:
        return S3D_TB;
#endif
    default:
        return S3D_ERROR;
    }
}

void adjustRect(hwc_rect_t &rect, int32_t width, int32_t height)
{
    if (rect.left < 0)
        rect.left = 0;
    if (rect.left > width)
        rect.left = width;
    if (rect.top < 0)
        rect.top = 0;
    if (rect.top > height)
        rect.top = height;
    if (rect.right < rect.left)
        rect.right = rect.left;
    if (rect.right > width)
        rect.right = width;
    if (rect.bottom < rect.top)
        rect.bottom = rect.top;
    if (rect.bottom > height)
        rect.bottom = height;
}

uint32_t halDataSpaceToV4L2ColorSpace(uint32_t data_space)
{
    switch (data_space) {
    case HAL_DATASPACE_BT2020:
    case HAL_DATASPACE_BT2020_FULL:
        return V4L2_COLORSPACE_BT2020;
    case HAL_DATASPACE_DCI_P3:
    case HAL_DATASPACE_DCI_P3_FULL:
        return V4L2_COLORSPACE_DCI_P3;
    default:
        return V4L2_COLORSPACE_DEFAULT;
    }
    return V4L2_COLORSPACE_DEFAULT;
}

unsigned int isNarrowRgb(int format, uint32_t data_space)
{
    if (format == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL)
        return 0;
    else {
        if (isFormatRgb(format))
            return 0;
        else {
            if ((data_space == HAL_DATASPACE_BT2020_FULL) ||
                (data_space == HAL_DATASPACE_DCI_P3_FULL))
                return 0;
            else
                return 1;
        }
    }
}
