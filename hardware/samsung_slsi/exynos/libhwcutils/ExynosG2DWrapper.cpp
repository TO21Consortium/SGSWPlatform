//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "ExynosG2DWrapper"
#include "ExynosG2DWrapper.h"
#include "ExynosHWCUtils.h"
#include "ExynosOverlayDisplay.h"
#ifdef USES_VIRTUAL_DISPLAY
#include "ExynosExternalDisplay.h"
#include "ExynosVirtualDisplay.h"
#else
#include "ExynosExternalDisplay.h"
#endif

#define FIMG2D_WFD_DEFAULT (G2D_LV1)

int formatValueHAL2G2D(int hal_format,
        color_format *g2d_format,
        pixel_order *g2d_order,
        uint32_t *g2d_bpp)
{
    *g2d_format = MSK_FORMAT_END;
    *g2d_order  = ARGB_ORDER_END;
    *g2d_bpp    = 0;

    switch (hal_format) {
    /* 16bpp */
    case HAL_PIXEL_FORMAT_RGB_565:
        *g2d_format = CF_RGB_565;
        *g2d_order  = AX_RGB;
        *g2d_bpp    = 2;
        break;
        /* 32bpp */
    case HAL_PIXEL_FORMAT_RGBX_8888:
        *g2d_format = CF_XRGB_8888;
        *g2d_order  = AX_BGR;
        *g2d_bpp    = 4;
        break;
#ifdef EXYNOS_SUPPORT_BGRX_8888
    case HAL_PIXEL_FORMAT_BGRX_8888:
        *g2d_format = CF_XRGB_8888;
        *g2d_order  = AX_RGB;
        *g2d_bpp    = 4;
        break;
#endif
    case HAL_PIXEL_FORMAT_BGRA_8888:
        *g2d_format = CF_ARGB_8888;
        *g2d_order  = AX_RGB;
        *g2d_bpp    = 4;
        break;
    case HAL_PIXEL_FORMAT_RGBA_8888:
        *g2d_format = CF_ARGB_8888;
        *g2d_order  = AX_BGR;
        *g2d_bpp    = 4;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
        *g2d_format = CF_ARGB_8888;
        *g2d_order  = BGR_AX;
        *g2d_bpp    = 4;
        break;
        /* 12bpp */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        *g2d_format = CF_YCBCR_420;
        *g2d_order  = P2_CRCB;
        *g2d_bpp    = 1;
        break;
    default:
        ALOGE("%s: no matching color format(0x%x): failed",
                __func__, hal_format);
        return -1;
        break;
    }
    return 0;
}

#ifdef USES_VIRTUAL_DISPLAY
ExynosG2DWrapper::ExynosG2DWrapper(ExynosOverlayDisplay *display,
                                   ExynosExternalDisplay *externalDisplay,
                                   ExynosVirtualDisplay *virtualDisplay)
#else
ExynosG2DWrapper::ExynosG2DWrapper(ExynosOverlayDisplay *display, ExynosExternalDisplay *hdmi)
#endif
{
    mDisplay = display;
#ifdef USES_VIRTUAL_DISPLAY
    mExternalDisplay = externalDisplay;
    mVirtualDisplay = virtualDisplay;
    mAllocSize = 0;
#else
    mExternalDisplay = hdmi;
#endif
}

ExynosG2DWrapper::~ExynosG2DWrapper()
{
}

int ExynosG2DWrapper::runCompositor(hwc_layer_1_t &src_layer, private_handle_t *dst_handle,
        uint32_t transform, uint32_t global_alpha, unsigned long solid,
        blit_op mode, bool force_clear, unsigned long srcAddress,
        unsigned long dstAddress, int is_lcd)
{
    int ret = 0;
    unsigned long srcYAddress = 0;
    unsigned long srcCbCrAddress = 0;
    unsigned long dstYAddress = 0;
    unsigned long dstCbCrAddress =0;

    ExynosRect   srcImgRect, dstImgRect;

    fimg2d_blit  BlitParam;
    fimg2d_param g2d_param;
    rotation     g2d_rotation;

    fimg2d_addr  srcYAddr;
    fimg2d_addr  srcCbCrAddr;
    fimg2d_image srcImage;
    fimg2d_rect  srcRect;

    fimg2d_addr  dstYAddr;
    fimg2d_addr  dstCbCrAddr;
    fimg2d_image dstImage;
    fimg2d_rect  dstRect;

    fimg2d_scale  Scaling;
    fimg2d_repeat Repeat;
    fimg2d_bluscr Bluscr;
    fimg2d_clip   Clipping;

    pixel_order  g2d_order;
    color_format g2d_format;
    addr_space   addr_type = ADDR_USER;

    uint32_t srcG2d_bpp, dstG2d_bpp;
    uint32_t srcImageSize, dstImageSize;
    bool src_ion_mapped = false;
    bool dst_ion_mapped = false;

    private_handle_t *src_handle = private_handle_t::dynamicCast(src_layer.handle);

    if (!force_clear) {
        srcImgRect.x = src_layer.sourceCropf.left;
        srcImgRect.y = src_layer.sourceCropf.top;
        srcImgRect.w = WIDTH(src_layer.sourceCropf);
        srcImgRect.h = HEIGHT(src_layer.sourceCropf);
        srcImgRect.fullW = src_handle->stride;
        srcImgRect.fullH = src_handle->vstride;
        srcImgRect.colorFormat = src_handle->format;
    }

#ifndef USES_VIRTUAL_DISPLAY
    int w, h;
    {
        w = mExternalDisplay->mXres;
        h = mExternalDisplay->mYres;
    }
#endif

    if (is_lcd) {
        dstImgRect.x = 0;
        dstImgRect.y = 0;
        dstImgRect.w = WIDTH(src_layer.displayFrame);
        dstImgRect.h = HEIGHT(src_layer.displayFrame);
        dstImgRect.fullW = dst_handle->stride;
        dstImgRect.fullH = dst_handle->vstride;
        dstImgRect.colorFormat = dst_handle->format;
    } else {
        if (force_clear) {
            dstImgRect.x = 0;
            dstImgRect.y = 0;
            dstImgRect.w = dst_handle->width;
            dstImgRect.h = dst_handle->height;
        } else {
            dstImgRect.x = src_layer.displayFrame.left;
            dstImgRect.y = src_layer.displayFrame.top;
            dstImgRect.w = WIDTH(src_layer.displayFrame);
            dstImgRect.h = HEIGHT(src_layer.displayFrame);
        }
#ifdef USES_VIRTUAL_DISPLAY
        dstImgRect.fullW = dst_handle->stride;
        dstImgRect.fullH = dst_handle->vstride;
#else
        dstImgRect.fullW = w;
        dstImgRect.fullH = h;
#endif
        dstImgRect.colorFormat = dst_handle->format;
    }

    g2d_rotation = rotateValueHAL2G2D(transform);

    ALOGV("%s: \n"
            "s_fw %d s_fh %d s_w %d s_h %d s_x %d s_y %d s_f %x address %x \n"
            "d_fw %d d_fh %d d_w %d d_h %d d_x %d d_y %d d_f %x address %x \n rot %d ",
            __func__,
            srcImgRect.fullW, srcImgRect.fullH, srcImgRect.w, srcImgRect.h,
            srcImgRect.x, srcImgRect.y, srcImgRect.colorFormat, src_handle->fd,
            dstImgRect.fullW, dstImgRect.fullH, dstImgRect.w, dstImgRect.h,
            dstImgRect.x, dstImgRect.y, dstImgRect.colorFormat, dst_handle->fd, transform);

    if (!force_clear && src_handle->fd >= 0) {
        int rotatedDstW = dstImgRect.w;
        int rotatedDstH = dstImgRect.h;
        if ((g2d_rotation == ROT_90) || (g2d_rotation == ROT_270)) {
            if ((srcImgRect.w != dstImgRect.h) || (srcImgRect.h != dstImgRect.w)) {
                rotatedDstW = dstImgRect.h;
                rotatedDstH = dstImgRect.w;
            }
        } else {
            if ((srcImgRect.w != dstImgRect.w) || (srcImgRect.h != dstImgRect.h)) {
                rotatedDstW = dstImgRect.w;
                rotatedDstH = dstImgRect.h;
            }
        }

        if (formatValueHAL2G2D(srcImgRect.colorFormat, &g2d_format, &g2d_order, &srcG2d_bpp) < 0) {
            ALOGE("%s: formatValueHAL2G2D() failed", __func__);
            return -1;
        }
        srcImageSize = srcImgRect.fullW*srcImgRect.fullH;
        if (srcAddress) {
            srcYAddress = srcAddress;
        } else {
            srcYAddress = (unsigned long)mmap(NULL, srcImageSize*srcG2d_bpp, PROT_READ | PROT_WRITE, MAP_SHARED, src_handle->fd, 0);
            if (srcYAddress == (unsigned long)MAP_FAILED) {
                ALOGE("%s: failed to mmap for src-Y address", __func__);
                return -ENOMEM;
            }
            src_ion_mapped = true;
        }

        srcYAddr.type  = addr_type;
        srcYAddr.start = (unsigned long)srcYAddress;
        srcCbCrAddr.type = addr_type;
        srcCbCrAddr.start = 0;

        srcRect.x1 = srcImgRect.x;
        srcRect.y1 = srcImgRect.y;
        srcRect.x2 = srcImgRect.x + srcImgRect.w;
        srcRect.y2 = srcImgRect.y + srcImgRect.h;
        srcImage.width = srcImgRect.fullW;
        srcImage.height = srcImgRect.fullH;
        srcImage.stride = srcImgRect.fullW*srcG2d_bpp;
        srcImage.order = g2d_order;
        srcImage.fmt = g2d_format;
        srcImage.addr = srcYAddr;
        srcImage.plane2 = srcCbCrAddr;
        srcImage.rect = srcRect;
        srcImage.need_cacheopr = false;

        Scaling.mode = SCALING_BILINEAR;
        Scaling.src_w= srcImgRect.w;
        Scaling.src_h= srcImgRect.h;
        Scaling.dst_w= rotatedDstW;
        Scaling.dst_h= rotatedDstH;
    } else {
        memset(&srcImage, 0, sizeof(srcImage));
        Scaling.mode = NO_SCALING;
        Scaling.src_w= 0;
        Scaling.src_h= 0;
        Scaling.dst_w= 0;
        Scaling.dst_h= 0;
    }

    if (dst_handle->fd >= 0) {
        if (formatValueHAL2G2D(dstImgRect.colorFormat, &g2d_format, &g2d_order, &dstG2d_bpp) < 0) {
            ALOGE("%s: formatValueHAL2G2D() failed", __func__);
            return -1;
        }
        dstImageSize = dstImgRect.fullW*dstImgRect.fullH;
        if (dstAddress) {
            dstYAddress = dstAddress;
        } else {
#ifdef USES_VIRTUAL_DISPLAY
            if (mVirtualDisplay == NULL) {
                dstYAddress = (unsigned long)mmap(NULL, dstImageSize*dstG2d_bpp, PROT_READ | PROT_WRITE, MAP_SHARED, dst_handle->fd, 0);
                if (dstYAddress == (unsigned long)MAP_FAILED) {
                    ALOGE("%s: failed to mmap for dst-Y address", __func__);
                    return -ENOMEM;
                }
            } else {
                dstYAddress = (unsigned long)mmap(NULL, dstImageSize*dstG2d_bpp, PROT_READ | PROT_WRITE, MAP_SHARED, dst_handle->fd, 0);
                if (dstYAddress == (unsigned long)MAP_FAILED) {
                    ALOGE("%s: failed to mmap for dst-Y address", __func__);
                    return -ENOMEM;
                }

                if (dst_handle->fd1 > 0) {
                    dstCbCrAddress = (unsigned long)mmap(NULL, dstImageSize*dstG2d_bpp / 2, PROT_READ | PROT_WRITE, MAP_SHARED, dst_handle->fd1, 0);
                    if (dstCbCrAddress == (unsigned long)MAP_FAILED) {
                        ALOGE("%s: failed to mmap for dst-CbCr address", __func__);
                        munmap((void *)dstYAddress, dstImageSize*dstG2d_bpp);
                        return -ENOMEM;
                    }
                }
            }
#else
            dstYAddress = (unsigned long)mmap(NULL, dstImageSize*dstG2d_bpp, PROT_READ | PROT_WRITE, MAP_SHARED, dst_handle->fd, 0);
            if (dstYAddress == (unsigned long)MAP_FAILED) {
                ALOGE("%s: failed to mmap for dst-Y address", __func__);
                return -ENOMEM;
            }
#endif
            dst_ion_mapped = true;
        }

        dstYAddr.type  = addr_type;
        dstYAddr.start = (unsigned long)dstYAddress;
        dstCbCrAddr.type = addr_type;
        dstCbCrAddr.start = (unsigned long)dstCbCrAddress;

        if (force_clear) {
            dstRect.x1 = 0;
            dstRect.y1 = 0;
            dstRect.x2 = dstImgRect.fullW;
            dstRect.y2 = dstImgRect.fullH;
        } else {
            dstRect.x1 = dstImgRect.x;
            dstRect.y1 = dstImgRect.y;
            dstRect.x2 = dstImgRect.x + dstImgRect.w;
            dstRect.y2 = dstImgRect.y + dstImgRect.h;
        }

        dstImage.width = dstImgRect.fullW;
        dstImage.height = dstImgRect.fullH;
        dstImage.stride = dstImgRect.fullW*dstG2d_bpp;
        dstImage.order = g2d_order;
        dstImage.fmt = g2d_format;
        dstImage.addr = dstYAddr;
        dstImage.plane2 = dstCbCrAddr;
        dstImage.rect = dstRect;
        dstImage.need_cacheopr = false;
    } else {
        memset(&dstImage, 0, sizeof(dstImage));
    }

    Repeat.mode = NO_REPEAT;
    Repeat.pad_color = 0;
    Bluscr.mode = OPAQUE;
    Bluscr.bs_color  = 0;
    Bluscr.bg_color  = 0;
    Clipping.enable = false;
    Clipping.x1 = 0;
    Clipping.y1 = 0;
    Clipping.x2 = 0;
    Clipping.y2 = 0;

    g2d_param.solid_color = solid;
    g2d_param.g_alpha = global_alpha;
    g2d_param.dither = false;
    g2d_param.rotate = g2d_rotation;
    g2d_param.premult = PREMULTIPLIED;
    g2d_param.scaling = Scaling;
    g2d_param.repeat = Repeat;
    g2d_param.bluscr = Bluscr;
    g2d_param.clipping = Clipping;

    if (force_clear) {
        BlitParam.op = mode;
        BlitParam.param = g2d_param;
        BlitParam.src = NULL;
        BlitParam.msk = NULL;
        BlitParam.tmp = NULL;
        BlitParam.dst = &dstImage;
        BlitParam.sync = BLIT_SYNC;
        BlitParam.seq_no = 0;
	BlitParam.qos_lv = FIMG2D_WFD_DEFAULT;
    } else {
        BlitParam.op = mode;
        BlitParam.param = g2d_param;
        BlitParam.src = &srcImage;
        BlitParam.msk = NULL;
        BlitParam.tmp = NULL;
        BlitParam.dst = &dstImage;
        BlitParam.sync = BLIT_SYNC;
        BlitParam.seq_no = 0;
	BlitParam.qos_lv = FIMG2D_WFD_DEFAULT;
    }

    ret = stretchFimgApi(&BlitParam);

    if (src_ion_mapped)
        munmap((void *)srcYAddress, srcImageSize*srcG2d_bpp);

    if (dst_ion_mapped) {
#ifdef USES_VIRTUAL_DISPLAY
        if (mVirtualDisplay == NULL)
            munmap((void *)dstYAddress, dstImageSize*dstG2d_bpp);
        else {
            munmap((void *)dstYAddress, dstImageSize*dstG2d_bpp);
            munmap((void *)dstCbCrAddress, dstImageSize*dstG2d_bpp / 2);
        }
#else
        munmap((void *)dstYAddress, dstImageSize*dstG2d_bpp);
#endif
    }

    if (ret < 0) {
        ALOGE("%s: stretch failed", __func__);
        return -1;
    }

    return 0;
}

#ifdef USES_VIRTUAL_DISPLAY
int ExynosG2DWrapper::runSecureCompositor(hwc_layer_1_t &src_layer,
        private_handle_t *dst_handle,
        private_handle_t *secure_handle,
        uint32_t global_alpha, unsigned long solid,
        blit_op mode, bool force_clear)
{
    int ret = 0;
    unsigned long srcYAddress = 0;

    ExynosRect   srcImgRect, dstImgRect;

    fimg2d_blit_raw  BlitParam;
    fimg2d_param g2d_param;
    rotation     g2d_rotation = ORIGIN;

    fimg2d_addr  srcYAddr;
    fimg2d_addr  srcCbCrAddr;
    fimg2d_image srcImage;
    fimg2d_rect  srcRect;

    fimg2d_addr  dstYAddr;
    fimg2d_addr  dstCbCrAddr;
    fimg2d_image dstImage;
    fimg2d_rect  dstRect;

    fimg2d_scale  Scaling;
    fimg2d_repeat Repeat;
    fimg2d_bluscr Bluscr;
    fimg2d_clip   Clipping;

    pixel_order  g2d_order;
    color_format g2d_format;
    addr_space   addr_type = ADDR_PHYS;

    uint32_t srcG2d_bpp, dstG2d_bpp;
    uint32_t srcImageSize, dstImageSize;
    bool src_ion_mapped = false;
    bool dst_ion_mapped = false;

    private_handle_t *src_handle = private_handle_t::dynamicCast(src_layer.handle);

    srcImgRect.x = src_layer.sourceCropf.left;
    srcImgRect.y = src_layer.sourceCropf.top;
    srcImgRect.w = WIDTH(src_layer.sourceCropf);
    srcImgRect.h = HEIGHT(src_layer.sourceCropf);
    srcImgRect.fullW = src_handle->stride;
    srcImgRect.fullH = src_handle->vstride;
    srcImgRect.colorFormat = src_handle->format;

    if (!secure_handle) {
        ALOGE("%s: secure g2d buffer handle is NULL", __func__);
        return -1;
    }

    if (force_clear) {
        dstImgRect.x = 0;
        dstImgRect.y = 0;
        dstImgRect.w = mVirtualDisplay->mWidth;
        dstImgRect.h = mVirtualDisplay->mHeight;
        dstImgRect.fullW = mVirtualDisplay->mWidth;
        dstImgRect.fullH = mVirtualDisplay->mHeight;
        dstImgRect.colorFormat = dst_handle->format;
    } else {
        dstImgRect.x = src_layer.displayFrame.left;
        dstImgRect.y = src_layer.displayFrame.top;
        dstImgRect.w = WIDTH(src_layer.displayFrame);
        dstImgRect.h = HEIGHT(src_layer.displayFrame);
        dstImgRect.fullW = mVirtualDisplay->mWidth;
        dstImgRect.fullH = mVirtualDisplay->mHeight;
        dstImgRect.colorFormat = dst_handle->format;
    }

    ALOGV("%s: \n"
            "s_fw %d s_fh %d s_w %d s_h %d s_x %d s_y %d s_f %x address %x \n"
            "d_fw %d d_fh %d d_w %d d_h %d d_x %d d_y %d d_f %x address %x \n rot %d ",
            __func__,
            srcImgRect.fullW, srcImgRect.fullH, srcImgRect.w, srcImgRect.h,
            srcImgRect.x, srcImgRect.y, srcImgRect.colorFormat, secure_handle->fd,
            dstImgRect.fullW, dstImgRect.fullH, dstImgRect.w, dstImgRect.h,
            dstImgRect.x, dstImgRect.y, dstImgRect.colorFormat, dst_handle->fd, g2d_rotation);

    if (secure_handle->fd >= 0) {
        int rotatedDstW = dstImgRect.w;
        int rotatedDstH = dstImgRect.h;

        if (formatValueHAL2G2D(srcImgRect.colorFormat, &g2d_format, &g2d_order, &srcG2d_bpp) < 0) {
            ALOGE("%s: formatValueHAL2G2D() failed", __func__);
            return -1;
        }
        srcImageSize = srcImgRect.fullW*srcImgRect.fullH;

        srcYAddr.type  = addr_type;
        srcYAddr.start = (unsigned long)secure_handle->fd;
        srcCbCrAddr.type = addr_type;
        srcCbCrAddr.start = 0;

        srcRect.x1 = srcImgRect.x;
        srcRect.y1 = srcImgRect.y;
        srcRect.x2 = srcImgRect.x + srcImgRect.w;
        srcRect.y2 = srcImgRect.y + srcImgRect.h;
        srcImage.width = srcImgRect.fullW;
        srcImage.height = srcImgRect.fullH;
        srcImage.stride = srcImgRect.fullW*srcG2d_bpp;
        srcImage.order = g2d_order;
        srcImage.fmt = g2d_format;
        srcImage.addr = srcYAddr;
        srcImage.plane2 = srcCbCrAddr;
        srcImage.rect = srcRect;
        srcImage.need_cacheopr = false;

        Scaling.mode = SCALING_BILINEAR;
        Scaling.src_w= srcImgRect.w;
        Scaling.src_h= srcImgRect.h;
        Scaling.dst_w= rotatedDstW;
        Scaling.dst_h= rotatedDstH;
    } else {
        memset(&srcImage, 0, sizeof(srcImage));
        Scaling.mode = NO_SCALING;
        Scaling.src_w= 0;
        Scaling.src_h= 0;
        Scaling.dst_w= 0;
        Scaling.dst_h= 0;
    }

    if (dst_handle->fd >= 0) {
        if (formatValueHAL2G2D(dstImgRect.colorFormat, &g2d_format, &g2d_order, &dstG2d_bpp) < 0) {
            ALOGE("%s: formatValueHAL2G2D() failed", __func__);
            return -1;
        }
        dstImageSize = dstImgRect.fullW*dstImgRect.fullH;
        dstYAddr.type  = addr_type;
        dstYAddr.start = (unsigned long)dst_handle->fd;
        dstCbCrAddr.type = addr_type;
        dstCbCrAddr.start = (unsigned long)dst_handle->fd1;

        if (force_clear) {
            dstRect.x1 = 0;
            dstRect.y1 = 0;
            dstRect.x2 = dstImgRect.fullW;
            dstRect.y2 = dstImgRect.fullH;
        } else {
            dstRect.x1 = dstImgRect.x;
            dstRect.y1 = dstImgRect.y;
            dstRect.x2 = dstImgRect.x + dstImgRect.w;
            dstRect.y2 = dstImgRect.y + dstImgRect.h;
        }

        dstImage.width = dstImgRect.fullW;
        dstImage.height = dstImgRect.fullH;
        dstImage.stride = dstImgRect.fullW*dstG2d_bpp;
        dstImage.order = g2d_order;
        dstImage.fmt = g2d_format;
        dstImage.addr = dstYAddr;
        dstImage.plane2 = dstCbCrAddr;
        dstImage.rect = dstRect;
        dstImage.need_cacheopr = false;
    } else {
        memset(&dstImage, 0, sizeof(dstImage));
    }

    Repeat.mode = NO_REPEAT;
    Repeat.pad_color = 0;
    Bluscr.mode = OPAQUE;
    Bluscr.bs_color  = 0;
    Bluscr.bg_color  = 0;
    Clipping.enable = false;
    Clipping.x1 = 0;
    Clipping.y1 = 0;
    Clipping.x2 = 0;
    Clipping.y2 = 0;

    g2d_param.solid_color = solid;
    g2d_param.g_alpha = global_alpha;
    g2d_param.dither = false;
    g2d_param.rotate = g2d_rotation;
    g2d_param.premult = PREMULTIPLIED;
    g2d_param.scaling = Scaling;
    g2d_param.repeat = Repeat;
    g2d_param.bluscr = Bluscr;
    g2d_param.clipping = Clipping;

    memset(&BlitParam, 0, sizeof(BlitParam));
    BlitParam.op = mode;
    memcpy(&BlitParam.param, &g2d_param, sizeof(g2d_param));
    memcpy(&BlitParam.src, &srcImage, sizeof(srcImage));
    memcpy(&BlitParam.dst, &dstImage, sizeof(dstImage));
    BlitParam.sync = BLIT_SYNC;
    BlitParam.seq_no = 0;
    BlitParam.qos_lv = FIMG2D_WFD_DEFAULT;

    ret = G2DDRM_Blit(&BlitParam);

    if (ret != 0) {
        ALOGE("%s: G2DDRM_Blit failed(ret=%d)", __func__, ret);
        return -1;
    }

    return 0;
}

bool ExynosG2DWrapper::InitSecureG2D()
{
    if (mVirtualDisplay == NULL)
        return false;

    int ret = 0;
    if (mVirtualDisplay->mPhysicallyLinearBuffer == NULL) {
        ALOGI("initialize secure G2D");
        ret = G2DDRM_Initialize();
        if (ret != 0) {
            ALOGE("%s: G2DDRM_Initialize failed, ret %d", __func__, ret);
            return false;
        }

        int dst_stride;
        int usage = GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_HW_COMPOSER |
            GRALLOC_USAGE_PROTECTED |
            GRALLOC_USAGE_PHYSICALLY_LINEAR |
            GRALLOC_USAGE_PRIVATE_NONSECURE;
        alloc_device_t* allocDevice = mVirtualDisplay->mAllocDevice;
        int ret = allocDevice->alloc(allocDevice,
                mVirtualDisplay->mWidth, mVirtualDisplay->mHeight,
                mVirtualDisplay->mGLESFormat, usage, &mVirtualDisplay->mPhysicallyLinearBuffer,
                &dst_stride);
        if (ret < 0) {
            ALOGE("failed to allocate secure g2d buffer: %s", strerror(-ret));
            G2DDRM_Terminate();
            return false;
        } else {
            mAllocSize = mVirtualDisplay->mWidth * mVirtualDisplay->mHeight * 4;
            private_handle_t *handle = private_handle_t::dynamicCast(
                                              mVirtualDisplay->mPhysicallyLinearBuffer);
            mVirtualDisplay->mPhysicallyLinearBufferAddr = (unsigned long)mmap(NULL, mAllocSize, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);
            if (mVirtualDisplay->mPhysicallyLinearBufferAddr == (unsigned long)MAP_FAILED) {
                ALOGE("%s: failed to mmap for virtual display buffer", __func__);
                return -ENOMEM;
            }
            ALOGI("allocated secure g2d input buffer: 0x%x", mVirtualDisplay->mPhysicallyLinearBufferAddr);
        }
    }
    return true;

}

bool ExynosG2DWrapper::TerminateSecureG2D()
{
    if (mVirtualDisplay == NULL)
        return false;

    int ret = 0;
    if (mVirtualDisplay->mPhysicallyLinearBuffer) {
        ALOGI("free g2d input buffer: 0x%x", mVirtualDisplay->mPhysicallyLinearBufferAddr);
        munmap((void *)mVirtualDisplay->mPhysicallyLinearBufferAddr, mAllocSize);
        mVirtualDisplay->mPhysicallyLinearBufferAddr = 0;

        alloc_device_t* allocDevice = mVirtualDisplay->mAllocDevice;
        allocDevice->free(allocDevice, mVirtualDisplay->mPhysicallyLinearBuffer);
        mVirtualDisplay->mPhysicallyLinearBuffer = NULL;

        ALOGI("terminate secure G2D");
        ret = G2DDRM_Terminate();
        if (ret != 0) {
            ALOGE("%s: G2DDRM_Terminate failed, ret %d", __func__, ret);
        }
    }
    return 0;
}
#endif

void ExynosG2DWrapper::exynos5_cleanup_g2d(int force)
{
#ifdef G2D_COMPOSITION
    exynos5_g2d_data_t &mG2d = mDisplay->mG2d;

    if (!mDisplay->mG2dMemoryAllocated && !force)
        return;

    for (int i = 0; i < (int)NUM_HW_WIN_FB_PHY; i++) {
        mDisplay->mG2dCurrentBuffer[i] = 0;
        mDisplay->mLastG2dLayerHandle[i] = 0;
        for (int j = 0; j < NUM_GSC_DST_BUFS; j++) {
            if (mDisplay->mWinBufFence[i][j] >= 0) {
                sync_wait(mDisplay->mWinBufFence[i][j], 1000);
                close(mDisplay->mWinBufFence[i][j]);
                mDisplay->mWinBufFence[i][j] = -1;
            }
        }
    }

    memset(&mG2d, 0, sizeof(mG2d));
    mDisplay->mG2dLayers = 0;
    mDisplay->mG2dComposition = 0;

    for (int i = 0; i < mDisplay->mAllocatedLayers; i++) {
        for (int j = 0; j < (int)NUM_GSC_DST_BUFS; j++) {
            if (mDisplay->mWinBufVirtualAddress[i][j]) {
                munmap((void *)mDisplay->mWinBufVirtualAddress[i][j], mDisplay->mWinBufMapSize[i]);
                mDisplay->mWinBufVirtualAddress[i][j] = 0;
            }
        }
    }

    for (int i = 0; i < mDisplay->mAllocatedLayers; i++) {
        for (int j = 0; j < (int)NUM_GSC_DST_BUFS; j++) {
            if (mDisplay->mWinBuf[i][j]) {
                mDisplay->mAllocDevice->free(mDisplay->mAllocDevice, mDisplay->mWinBuf[i][j]);
                mDisplay->mWinBuf[i][j] = NULL;
            }
        }
    }
    mDisplay->mAllocatedLayers = 0;
    mDisplay->mG2dMemoryAllocated = 0;
#endif
}

int ExynosG2DWrapper::exynos5_g2d_buf_alloc(hwc_display_contents_1_t* contents)
{
#ifdef G2D_COMPOSITION
    int w, h;
    int dst_stride;
    int format = HAL_PIXEL_FORMAT_RGBX_8888;
    int usage;

    if (mDisplay->mG2dMemoryAllocated)
        return 0;

    usage = GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER | GRALLOC_USAGE_PHYSICALLY_LINEAR |
            GRALLOC_USAGE_HW_COMPOSER;
    usage |= GRALLOC_USAGE_PROTECTED;
    usage &= ~GRALLOC_USAGE_PRIVATE_NONSECURE;

    for (int i = 0; i < mDisplay->mG2dLayers; i++) {
        int lay_idx = mDisplay->mG2d.ovly_lay_idx[i];
        hwc_layer_1_t &layer = contents->hwLayers[lay_idx];
        w = WIDTH(layer.displayFrame);
        h = HEIGHT(layer.displayFrame);

        for (int j = 0; j < (int)NUM_GSC_DST_BUFS; j++) {
            int ret = mDisplay->mAllocDevice->alloc(mDisplay->mAllocDevice, w, h,
                    format, usage, &mDisplay->mWinBuf[i][j],
                    &dst_stride);
            if (ret < 0) {
                ALOGE("failed to allocate win %d buf %d buffer [w %d h %d f %x]: %s",
                        i, j, w, h, format, strerror(-ret));
                goto G2D_BUF_ALLOC_FAIL;
            }
        }
        mDisplay->mWinBufMapSize[i] = dst_stride * h * 4 + 0x8000;
    }
    mDisplay->mAllocatedLayers = mDisplay->mG2dLayers;

    unsigned long vir_addr;
    for (int i = 0; i < mDisplay->mG2dLayers; i++) {
        for (int j = 0; j < (int)NUM_GSC_DST_BUFS; j++) {
            mDisplay->mWinBufFence[i][j] = -1;
            private_handle_t *buf_handle = private_handle_t::dynamicCast(mDisplay->mWinBuf[i][j]);
            vir_addr = (unsigned long) mmap(NULL, mDisplay->mWinBufMapSize[i], PROT_READ | PROT_WRITE, MAP_SHARED, buf_handle->fd, 0);
            if (vir_addr != (unsigned long)MAP_FAILED) {
                mDisplay->mWinBufVirtualAddress[i][j] = vir_addr;
            } else {
                ALOGE("Failed to map win %d buf %d buffer", i, j);
                goto G2D_BUF_ALLOC_FAIL;
            }
        }
    }

    mDisplay->mG2dMemoryAllocated = 1;
    return 0;

G2D_BUF_ALLOC_FAIL:
    mDisplay->mG2dMemoryAllocated = 1;
    /* memory released from the caller */

#endif
    return 1;
}

int ExynosG2DWrapper::exynos5_config_g2d(hwc_layer_1_t &layer, private_handle_t *dst_handle, s3c_fb_win_config &cfg, int win_idx_2d, int win_idx)
{
#ifdef G2D_COMPOSITION
    int ret = 0;
    int cur_buf = mDisplay->mG2dCurrentBuffer[win_idx_2d];
    int prev_buf;
    private_handle_t *handle;
    int fence = -1;
    private_handle_t *src_handle = private_handle_t::dynamicCast(layer.handle);

    if (mDisplay->mLastG2dLayerHandle[win_idx_2d] == (uint32_t)layer.handle) {
        prev_buf = (cur_buf + NUM_GSC_DST_BUFS - 1) % NUM_GSC_DST_BUFS;
        if (mDisplay->mWinBufFence[win_idx_2d][prev_buf] >= 0) {
            close(mDisplay->mWinBufFence[win_idx_2d][prev_buf] );
            mDisplay->mWinBufFence[win_idx_2d][prev_buf]  = -1;
        }
        mDisplay->mG2dCurrentBuffer[win_idx_2d] = prev_buf;
        handle = private_handle_t::dynamicCast(mDisplay->mWinBuf[win_idx_2d][prev_buf]);
        memcpy(dst_handle, handle, sizeof(*dst_handle));
        dst_handle->format = src_handle->format;
    } else {
        handle = private_handle_t::dynamicCast(mDisplay->mWinBuf[win_idx_2d][cur_buf]);
        memcpy(dst_handle, handle, sizeof(*dst_handle));
        dst_handle->format = src_handle->format;
        if (mDisplay->mWinBufFence[win_idx_2d][cur_buf] >= 0) {
            sync_wait(mDisplay->mWinBufFence[win_idx_2d][cur_buf], 1000);
            close(mDisplay->mWinBufFence[win_idx_2d][cur_buf] );
            mDisplay->mWinBufFence[win_idx_2d][cur_buf]  = -1;
        }

        if (layer.acquireFenceFd >= 0) {
            sync_wait(layer.acquireFenceFd, 1000);
        }

        if (dst_handle->format == HAL_PIXEL_FORMAT_RGBX_8888) {
            ret = runCompositor(layer, dst_handle, 0, 0xff, 0, BLIT_OP_SRC, false, 0,
                    mDisplay->mWinBufVirtualAddress[win_idx_2d][cur_buf] + 0x8000, 1);
        } else {
            ret = runCompositor(layer, dst_handle, 0, 0xff, 0, BLIT_OP_SRC, false, 0,
                    mDisplay->mWinBufVirtualAddress[win_idx_2d][cur_buf], 1);
        }
        if (ret < 0)
            ALOGE("%s:runCompositor: Failed", __func__);
    }

    mDisplay->mLastG2dLayerHandle[win_idx_2d] = (uint32_t)layer.handle;

    if (layer.acquireFenceFd >= 0)
        close(layer.acquireFenceFd);

    return fence;
#else
    return -1;
#endif
}

