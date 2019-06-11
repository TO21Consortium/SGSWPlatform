#ifndef HWC_DEBUG_H
#define HWC_DEBUG_H

enum {
    eDebugDefault           =   0x00000001,
    eDebugWindowUpdate      =   0x00000002,
    eDebugWinConfig         =   0x00000004,
    eDebugSkipStaicLayer    =   0x00000008,
    eDebugOverlaySupported  =   0x00000010,
    eDebugResourceAssigning =   0x00000020,
    eDebugFence             =   0x00000040,
    eDebugResourceManager   =   0x00000080,
    eDebugMPP               =   0x00000100,
};

inline bool hwcCheckDebugMessages(uint32_t type)
{
    return hwcDebug & type;
}

#if defined(DISABLE_HWC_DEBUG)
#define HDEBUGLOGD(...)
#define HDEBUGLOGV(...)
#define HDEBUGLOGE(...)
#else
#define HDEBUGLOGD(type, ...) \
    if (hwcCheckDebugMessages(type)) \
        ALOGD(__VA_ARGS__);
#define HDEBUGLOGV(type, ...) \
    if (hwcCheckDebugMessages(type)) \
        ALOGV(__VA_ARGS__);
#define HDEBUGLOGE(type, ...) \
    if (hwcCheckDebugMessages(type)) \
        ALOGE(__VA_ARGS__);
#endif

#if defined(DISABLE_HWC_DEBUG)
#define DISPLAY_LOGD(...)
#else
#define DISPLAY_LOGD(type, msg, ...) \
    if (hwcCheckDebugMessages(type)) \
        ALOGD("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#endif
#define DISPLAY_LOGV(msg, ...) ALOGV("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGI(msg, ...) ALOGI("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGW(msg, ...) ALOGW("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGE(msg, ...) ALOGE("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)

#endif
