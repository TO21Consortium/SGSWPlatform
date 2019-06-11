#ifndef __HARDWARE_EXYNOS_LIBHWJPEG_LIBCSC_H__
#define __HARDWARE_EXYNOS_LIBHWJPEG_LIBCSC_H__

#include <csc.h>

int V4L2FMT2HALFMT(int fmt);

class CLibCSC {
    void *m_hdlLibCSC;
public:
    CLibCSC(): m_hdlLibCSC(NULL) { }
    ~CLibCSC() { destroy(); }
    void destroy();
    bool init(int devid = 0);

    bool set_src_format(
            unsigned int width,
            unsigned int height,
            unsigned int crop_left,
            unsigned int crop_top,
            unsigned int crop_width,
            unsigned int crop_height,
            unsigned int color_format);

    bool set_dst_format(
            unsigned int width,
            unsigned int height,
            unsigned int crop_left,
            unsigned int crop_top,
            unsigned int crop_width,
            unsigned int crop_height,
            unsigned int color_format);

    bool set_src_format(
            unsigned int width,
            unsigned int height,
            unsigned int color_format) {
        return set_src_format(width, height, 0, 0, width, height, color_format);
    }

    bool set_dst_format(
            unsigned int width,
            unsigned int height,
            unsigned int color_format) {
        return set_dst_format(width, height, 0, 0, width, height, color_format);
    }

    bool set_src_buffer(void *addrs[CSC_MAX_PLANES], int mem_type);
    bool set_dst_buffer(void *addrs[CSC_MAX_PLANES], int mem_type);
    bool convert();
    bool convert(int rotation, int flip_horizontal, int flip_vertical);
};

#endif //__HARDWARE_EXYNOS_LIBHWJPEG_LIBCSC_H__
