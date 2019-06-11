#ifndef EXYNOS_G2D_WRAPPER_H
#define EXYNOS_G2D_WRAPPER_H

#include "ExynosHWC.h"

#ifdef USES_VIRTUAL_DISPLAY
#include "FimgApi.h"
#include "sec_g2ddrm.h"
#endif

class ExynosOverlayDisplay;
class ExynosExternalDisplay;
#ifdef USES_VIRTUAL_DISPLAY
class ExynosVirtualDisplay;
#endif

inline rotation rotateValueHAL2G2D(unsigned char transform)
{
    int rotate_flag = transform & 0x7;

    switch (rotate_flag) {
    case HAL_TRANSFORM_ROT_90:  return ROT_90;
    case HAL_TRANSFORM_ROT_180: return ROT_180;
    case HAL_TRANSFORM_ROT_270: return ROT_270;
    }
    return ORIGIN;
}

int formatValueHAL2G2D(int hal_format, color_format *g2d_format, pixel_order *g2d_order, uint32_t *g2d_bpp);

class ExynosG2DWrapper {
    public:
#ifdef USES_VIRTUAL_DISPLAY
        ExynosG2DWrapper(ExynosOverlayDisplay *display,
                         ExynosExternalDisplay *externalDisplay,
                         ExynosVirtualDisplay *virtualDisplay = NULL);
#else
        ExynosG2DWrapper(ExynosOverlayDisplay *display, ExynosExternalDisplay *hdmi);
#endif
        ~ExynosG2DWrapper();

        int runCompositor(hwc_layer_1_t &src_layer, private_handle_t *dst_handle,
                uint32_t transform, uint32_t global_alpha, unsigned long solid,
                blit_op mode, bool force_clear, unsigned long srcAddress,
                unsigned long dstAddress, int is_lcd);
#ifdef USES_VIRTUAL_DISPLAY
        int runSecureCompositor(hwc_layer_1_t &src_layer, private_handle_t *dst_handle,
                private_handle_t *secure_handle, uint32_t global_alpha, unsigned long solid,
                blit_op mode, bool force_clear);
        bool InitSecureG2D();
        bool TerminateSecureG2D();
#endif
        void exynos5_cleanup_g2d(int force);
        int exynos5_g2d_buf_alloc(hwc_display_contents_1_t* contents);
        int exynos5_config_g2d(hwc_layer_1_t &layer, private_handle_t *dstHandle, s3c_fb_win_config &cfg, int win_idx_2d, int win_idx);

        ExynosOverlayDisplay *mDisplay;
        ExynosExternalDisplay *mExternalDisplay;
#ifdef USES_VIRTUAL_DISPLAY
        ExynosVirtualDisplay *mVirtualDisplay;
        int mAllocSize;
#endif
};

#endif
