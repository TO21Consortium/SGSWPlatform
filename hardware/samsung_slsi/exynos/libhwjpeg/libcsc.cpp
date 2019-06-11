#include <linux/videodev2.h>

#include <ui/PixelFormat.h>

#include <exynos_format.h>

#include "hwjpeg-internal.h"
#include "hwjpeg-libcsc.h"

int V4L2FMT2HALFMT(int fmt)
{
    switch (fmt) {
        case V4L2_PIX_FMT_RGB32:
            return HAL_PIXEL_FORMAT_RGBA_8888;
        case V4L2_PIX_FMT_RGB24:
            return HAL_PIXEL_FORMAT_RGB_888;
        case V4L2_PIX_FMT_RGB565:
            return HAL_PIXEL_FORMAT_RGB_565;
        case V4L2_PIX_FMT_BGR32:
            return HAL_PIXEL_FORMAT_BGRA_8888;
        case V4L2_PIX_FMT_YVU420M:
            return HAL_PIXEL_FORMAT_EXYNOS_YV12_M;
        case V4L2_PIX_FMT_YUV420M:
            return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M;
        case V4L2_PIX_FMT_YVU420:
            return HAL_PIXEL_FORMAT_YV12;
        case V4L2_PIX_FMT_YUV420:
            return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P;
        case V4L2_PIX_FMT_NV16:
            return HAL_PIXEL_FORMAT_YCbCr_422_SP;
        case V4L2_PIX_FMT_NV12:
            return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP;
        case V4L2_PIX_FMT_NV12M:
            return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
        case V4L2_PIX_FMT_YUYV:
            return HAL_PIXEL_FORMAT_YCbCr_422_I;
        case V4L2_PIX_FMT_UYVY:
            return HAL_PIXEL_FORMAT_EXYNOS_CbYCrY_422_I;
        case V4L2_PIX_FMT_NV61:
            return HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_SP;
        case V4L2_PIX_FMT_NV21M:
            return HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;
        case V4L2_PIX_FMT_NV21:
            return HAL_PIXEL_FORMAT_YCrCb_420_SP;
        case V4L2_PIX_FMT_NV12MT_16X16:
            return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED;
        case V4L2_PIX_FMT_YVYU:
            return HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I;
        case V4L2_PIX_FMT_VYUY:
            return HAL_PIXEL_FORMAT_EXYNOS_CrYCbY_422_I;
        default:
            ALOGE("Unkown HAL format %d", fmt);
    }

    return -1;
}

#define CSC_ERR(ret) (ret != CSC_ErrorNone)

void CLibCSC::destroy()
{
    if (m_hdlLibCSC) {
        CSC_ERRORCODE ret = csc_deinit(m_hdlLibCSC);
        if (CSC_ERR(ret))
            ALOGE("Failed to deinit LibCSC: %d", ret);
        m_hdlLibCSC = NULL;
    }
}

bool CLibCSC::init(int devid)
{
    destroy();
    m_hdlLibCSC = csc_init(CSC_METHOD_HW);
    if (!m_hdlLibCSC) {
        ALOGE("Failed to create CSC handle");
        return false;
    }

    CSC_ERRORCODE ret;

    ret = csc_set_method(m_hdlLibCSC, CSC_METHOD_HW);
    if (CSC_ERR(ret)) {
        ALOGE("Failed to set CSC method to H/W");
        return false;
    }

    ret = csc_set_hw_property(m_hdlLibCSC, CSC_HW_PROPERTY_FIXED_NODE, CSC_HW_SC0 + devid);
    if (CSC_ERR(ret)) {
        ALOGE("Failed to set CSC H/W selection to Scaler%d", devid);
        return false;
    }

    return true;
}

bool CLibCSC::set_src_format(
        unsigned int width,
        unsigned int height,
        unsigned int crop_left,
        unsigned int crop_top,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int color_format)
{
    CSC_ERRORCODE ret;
    ret = csc_set_src_format(
            m_hdlLibCSC, width, height,
            crop_left, crop_top, crop_width, crop_height, color_format,
            false);
    if (CSC_ERR(ret)) {
        ALOGE("Failed to set source format: %d", ret);
        return false;
    }

    return true;
}

bool CLibCSC::set_dst_format(
        unsigned int width,
        unsigned int height,
        unsigned int crop_left,
        unsigned int crop_top,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int color_format)
{
    CSC_ERRORCODE ret;
    ret = csc_set_dst_format(
            m_hdlLibCSC, width, height,
            crop_left, crop_top, crop_width, crop_height, color_format,
            false);
    if (CSC_ERR(ret)) {
        ALOGE("Failed to set source format: %d", ret);
        return false;
    }

    return true;
}

bool CLibCSC::set_src_buffer(void *addrs[CSC_MAX_PLANES], int mem_type)
{
    CSC_ERRORCODE ret;
    ret = csc_set_src_buffer(m_hdlLibCSC, addrs, mem_type);
    if (CSC_ERR(ret)) {
        ALOGE("Failed to set source buffer: %d", ret);
        return false;
    }

    return true;
}

bool CLibCSC::set_dst_buffer(void *addrs[CSC_MAX_PLANES], int mem_type)
{
    CSC_ERRORCODE ret;
    ret = csc_set_dst_buffer(m_hdlLibCSC, addrs, mem_type);
    if (CSC_ERR(ret)) {
        ALOGE("Failed to set source buffer: %d", ret);
        return false;
    }

    return true;
}

bool CLibCSC::convert()
{
    CSC_ERRORCODE ret;
    ret = csc_convert(m_hdlLibCSC);
    if (CSC_ERR(ret)) {
        ALOGE("Failed to convert(): %d", ret);
        return false;
    }

    return true;
}
