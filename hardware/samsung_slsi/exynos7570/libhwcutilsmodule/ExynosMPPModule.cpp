#include "ExynosMPPModule.h"
#include "ExynosHWCUtils.h"

ExynosMPPModule::ExynosMPPModule()
    : ExynosMPP()
{
}

ExynosMPPModule::ExynosMPPModule(ExynosDisplay *display, int gscIndex)
    : ExynosMPP(display, gscIndex)
{
}

ExynosMPPModule::ExynosMPPModule(ExynosDisplay *display, unsigned int mppType, unsigned int mppIndex)
    : ExynosMPP(display, mppType, mppIndex)
{
}

int ExynosMPPModule::getBufferUsage(private_handle_t *srcHandle)
{
    int usage = GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_NOZEROED |
            GRALLOC_USAGE_HW_COMPOSER;

    if (getDrmMode(srcHandle->flags) == SECURE_DRM) {
        usage |= GRALLOC_USAGE_PROTECTED;
        usage &= ~GRALLOC_USAGE_PRIVATE_NONSECURE;
    } else if (getDrmMode(srcHandle->flags) == NORMAL_DRM) {
        usage |= GRALLOC_USAGE_PROTECTED;
        usage |= GRALLOC_USAGE_PRIVATE_NONSECURE;
    }
    if (srcHandle->flags & GRALLOC_USAGE_VIDEO_EXT)
        usage |= GRALLOC_USAGE_VIDEO_EXT;

    /* HACK: for distinguishing FIMD_VIDEO_region */
    if (!((usage & GRALLOC_USAGE_PROTECTED) &&
         !(usage & GRALLOC_USAGE_PRIVATE_NONSECURE) &&
         !(usage & GRALLOC_USAGE_VIDEO_EXT))) {
        usage |= (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER);
    }

    return usage;
}

bool ExynosMPPModule::isFormatSupportedByMPP(int format)
{
    switch (format) {
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_TILED:
            if (mType == MPP_MSC)
                return true;
            break;
    }
    return ExynosMPP::isFormatSupportedByMPP(format);
}
