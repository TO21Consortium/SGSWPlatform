#include "ExynosMPPModule.h"
#include "ExynosExternalDisplay.h"

ExynosExternalDisplay::ExynosExternalDisplay(struct exynos5_hwc_composer_device_1_t *pdev) :
    ExynosDisplay(1),
    mFlagIONBufferAllocated(false)
{
    this->mHwc = pdev;
    mMPPs[0] = NULL;
    mEnabled = false;
    mBlanked = false;
    mUseSubtitles = false;

}

ExynosExternalDisplay::~ExynosExternalDisplay()
{
}

int ExynosExternalDisplay::prepare(__unused hwc_display_contents_1_t *contents)
{
    return 0;
}

int ExynosExternalDisplay::clearDisplay()
{
    return -1;
}

int ExynosExternalDisplay::set(__unused hwc_display_contents_1_t *contents)
{
    return 0;
}

int ExynosExternalDisplay::openHdmi()
{
    return 0;
}

void ExynosExternalDisplay::setHdmiStatus(__unused bool status)
{
}

bool ExynosExternalDisplay::isPresetSupported(__unused unsigned int preset)
{
    return false;
}

int ExynosExternalDisplay::getConfig()
{
    return 0;
}

int ExynosExternalDisplay::getDisplayConfigs(__unused uint32_t *configs, size_t *numConfigs)
{
    *numConfigs = 0;
    return -1;
}

int ExynosExternalDisplay::enableLayer(__unused hdmi_layer_t &hl)
{
    return 0;
}

void ExynosExternalDisplay::disableLayer(__unused hdmi_layer_t &hl)
{
}

int ExynosExternalDisplay::enable()
{
    return 0;
}

void ExynosExternalDisplay::disable()
{
}

int ExynosExternalDisplay::output(__unused hdmi_layer_t &hl, __unused hwc_layer_1_t &layer, __unused private_handle_t *h, __unused int acquireFenceFd, __unused int *releaseFenceFd)
{
    return 0;
}

void ExynosExternalDisplay::skipStaticLayers(__unused hwc_display_contents_1_t *contents, __unused int ovly_idx)
{
}

void ExynosExternalDisplay::setPreset(__unused int preset)
{
}

int ExynosExternalDisplay::convert3DTo2D(__unused int preset)
{
    return 0;
}

void ExynosExternalDisplay::calculateDstRect(__unused int src_w, __unused int src_h, __unused int dst_w, __unused int dst_h, __unused struct v4l2_rect *dst_rect)
{
}

void ExynosExternalDisplay::setHdcpStatus(__unused int status)
{
}

void ExynosExternalDisplay::setAudioChannel(__unused uint32_t channels)
{
}

uint32_t ExynosExternalDisplay::getAudioChannel()
{
    return 0;
}

int ExynosExternalDisplay::blank()
{
    return 0;
}

int ExynosExternalDisplay::waitForRenderFinish(__unused private_module_t *grallocModule, __unused buffer_handle_t *handle, __unused int buffers)
{
    return 0;
}
