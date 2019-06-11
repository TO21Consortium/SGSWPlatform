#ifndef EXYNOS_DISPLAY_H
#define EXYNOS_DISPLAY_H

#include <utils/Mutex.h>
#include <utils/Vector.h>
#include "ExynosHWC.h"

#ifndef MAX_BUF_STRIDE
#define MAX_BUF_STRIDE  4096
#endif

class ExynosMPPModule;

enum {
    eSkipLayer                    =     0x00000001,
    eUnsupportedPlaneAlpha        =     0x00000002,
    eInvalidHandle                =     0x00000004,
    eHasFloatSrcCrop              =     0x00000008,
    eUnsupportedDstWidth          =     0x00000010,
    eUnsupportedCoordinate        =     0x00000020,
    eUnsupportedFormat            =     0x00000040,
    eUnsupportedBlending          =     0x00000080,
    eDynamicRecomposition         =     0x00000100,
    eForceFbEnabled               =     0x00000200,
    eSandwitchedBetweenGLES       =     0x00000400,
    eHasPopupVideo                =     0x00000800,
    eHasDRMVideo                  =     0x00001000,
    eInsufficientBandwidth        =     0x00002000,
    eInsufficientOverlapCount     =     0x00004000,
    eInsufficientWindow           =     0x00008000,
    eInsufficientMPP              =     0x00010000,
    eSwitchingLocalPath           =     0x00020000,
    eRGBLayerDuringVideoPlayback  =     0x00040000,
    eSkipStaticLayer              =     0x00080000,
    eNotAlignedDstPosition        =     0x00100000,
    eUnSupportedUseCase           =     0x00200000,
    eExceedHStrideMaximum         =     0x00400000,
    eMPPUnsupported               =     0x40000000,
    eUnknown                      =     0x80000000,
};

const struct s3cFormat {
    uint32_t format;
    const char *desc;
} s3cFormat[] = {
    {S3C_FB_PIXEL_FORMAT_RGBA_8888, "RGBA8888"},
    {S3C_FB_PIXEL_FORMAT_RGBX_8888, "RGBX8888"},
    {S3C_FB_PIXEL_FORMAT_RGBA_5551, "RGBA5551"},
    {S3C_FB_PIXEL_FORMAT_RGB_565,   "RGB565"},
    {S3C_FB_PIXEL_FORMAT_BGRA_8888, "BGRA8888"},
    {S3C_FB_PIXEL_FORMAT_BGRX_8888, "BGRX8888"},
};

class ExynosLayerInfo {
    public:
        int32_t         compositionType;
        uint32_t        mCheckOverlayFlag;
        uint32_t        mCheckMPPFlag;
};

bool winConfigChanged(s3c_fb_win_config *c1, s3c_fb_win_config *c2);
void dumpConfig(s3c_fb_win_config &c);
enum s3c_fb_pixel_format halFormatToS3CFormat(int format);
bool isFormatSupported(int format);
enum s3c_fb_blending halBlendingToS3CBlending(int32_t blending);
bool isBlendingSupported(int32_t blending);
const char *s3cFormat2str(uint32_t format);

class ExynosDisplay {
    public:
        /* Methods */
        ExynosDisplay(int numGSCs);
        virtual ~ExynosDisplay();

        virtual int getDeconWinMap(int overlayIndex, int totalOverlays);
        virtual int inverseWinMap(int windowIndex, int totalOverlays);
        virtual int prepare(hwc_display_contents_1_t *contents);
        virtual int set(hwc_display_contents_1_t *contents);
        virtual void dump(android::String8& result);
        virtual void freeMPP();
        virtual void allocateLayerInfos(hwc_display_contents_1_t* contents);
        virtual void dumpLayerInfo(android::String8& result);
        virtual int32_t getDisplayAttributes(const uint32_t __unused attribute, uint32_t __unused config = 0) {return 0;};
        virtual int getActiveConfig() {return 0;};
        virtual int setActiveConfig(int __unused index) {return 0;};

        /* Fields */
        int                     mDisplayFd;
        int32_t                 mXres;
        int32_t                 mYres;

        int32_t                 mXdpi;
        int32_t                 mYdpi;
        int32_t                 mVsyncPeriod;

        int                     mOtfMode;
        bool                    mHasDrmSurface;
        alloc_device_t          *mAllocDevice;
        int                     mNumMPPs;
        android::Mutex          mLayerInfoMutex;
        android::Vector<ExynosLayerInfo *> mLayerInfos;

        struct exynos5_hwc_composer_device_1_t *mHwc;
};

#endif
