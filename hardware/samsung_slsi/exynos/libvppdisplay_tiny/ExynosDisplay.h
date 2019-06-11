#ifndef EXYNOS_DISPLAY_H
#define EXYNOS_DISPLAY_H

#include <utils/Mutex.h>
#include <utils/Vector.h>
#include <utils/String8.h>
#include "ExynosHWC.h"

inline int WIDTH(const hwc_rect &rect) { return rect.right - rect.left; }
inline int HEIGHT(const hwc_rect &rect) { return rect.bottom - rect.top; }
inline int WIDTH(const hwc_frect_t &rect) { return (int)(rect.right - rect.left); }
inline int HEIGHT(const hwc_frect_t &rect) { return (int)(rect.bottom - rect.top); }


#define HWC_SKIP_RENDERING 0x80000000

const struct deconFormat {
    uint32_t format;
    const char *desc;
} deconFormat[] = {
    {DECON_PIXEL_FORMAT_ARGB_8888, "ARGB8888"},
    {DECON_PIXEL_FORMAT_ABGR_8888, "ABGR8888"},
    {DECON_PIXEL_FORMAT_RGBA_8888, "RGBA8888"},
    {DECON_PIXEL_FORMAT_BGRA_8888, "BGRA8888"},
    {DECON_PIXEL_FORMAT_XRGB_8888, "XRGB8888"},
    {DECON_PIXEL_FORMAT_XBGR_8888, "XBGR8888"},
    {DECON_PIXEL_FORMAT_RGBX_8888, "RGBX8888"},
    {DECON_PIXEL_FORMAT_BGRX_8888, "BGRX8888"},
    {DECON_PIXEL_FORMAT_RGBA_5551, "RGBA5551"},
    {DECON_PIXEL_FORMAT_RGB_565,   "RGB565"},
    {DECON_PIXEL_FORMAT_NV16, "FORMATNV16"},
    {DECON_PIXEL_FORMAT_NV61, "FORMATNV61"},
    {DECON_PIXEL_FORMAT_YVU422_3P, "YVU4223P"},
    {DECON_PIXEL_FORMAT_NV12, "FORMATNV12"},
    {DECON_PIXEL_FORMAT_NV21, "FORMATNV21"},
    {DECON_PIXEL_FORMAT_NV12M, "FORMATNV12M"},
    {DECON_PIXEL_FORMAT_NV21M, "FORMATNV21M"},
    {DECON_PIXEL_FORMAT_YUV420, "FORMATYUV420"},
    {DECON_PIXEL_FORMAT_YVU420, "FORMATYVU420"},
    {DECON_PIXEL_FORMAT_YUV420M, "FORMATYUV420M"},
    {DECON_PIXEL_FORMAT_YVU420M, "FORMATYVU420M"},
};

enum {
    EXYNOS_PRIMARY_DISPLAY      =       0,
#if defined(USES_DUAL_DISPLAY)
    EXYNOS_SECONDARY_DISPLAY,
#endif
    EXYNOS_EXTERNAL_DISPLAY,
    EXYNOS_VIRTUAL_DISPLAY
};

class ExynosLayerInfo {
    public:
        int32_t         compositionType;
        uint32_t        mCheckOverlayFlag;
        uint32_t        mCheckMPPFlag;
        int32_t         mWindowIndex;
        uint32_t        mDmaType;
};


enum decon_pixel_format halFormatToS3CFormat(int format);
bool isFormatSupported(int format);
enum decon_blending halBlendingToS3CBlending(int32_t blending);
bool isBlendingSupported(int32_t blending);
const char *deconFormat2str(uint32_t format);
bool winConfigChanged(decon_win_config *c1, decon_win_config *c2);
bool frameChanged(decon_frame *f1, decon_frame *f2);

class ExynosDisplay {
    public:
        /* Methods */
        ExynosDisplay(int numMPPs);
        ExynosDisplay(uint32_t type, struct exynos5_hwc_composer_device_1_t *pdev);
        virtual ~ExynosDisplay();

        virtual int prepare(hwc_display_contents_1_t *contents);
        virtual int set(hwc_display_contents_1_t *contents);
        virtual void dump(android::String8& result);
        virtual void dumpLayerInfo(android::String8& result);

        virtual int clearDisplay();
        virtual int32_t getDisplayAttributes(const uint32_t attribute);
        void dumpConfig(decon_win_config &c);

        /* Fields */
        int                     mDisplayFd;
        uint32_t                mType;
        int32_t                 mXres;
        int32_t                 mYres;
        int32_t                 mXdpi;
        int32_t                 mYdpi;
        int32_t                 mVsyncPeriod;
        bool                    mBlanked;
        struct exynos5_hwc_composer_device_1_t *mHwc;



        bool                     mFbNeeded;

        android::String8         mDisplayName;



    protected:
        /* Methods */
        virtual void configureHandle(private_handle_t *handle, size_t index, hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg);
        virtual void configureOverlay(hwc_layer_1_t *layer, size_t index, decon_win_config &cfg);

        virtual int postFrame(hwc_display_contents_1_t *contents);
        virtual int winconfigIoctl(decon_win_config_data *win_data);
};
#endif
