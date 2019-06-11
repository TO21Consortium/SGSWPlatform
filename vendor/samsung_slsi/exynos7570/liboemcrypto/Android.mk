LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#ifeq ($(TARGET_ARCH), arm)
#ifneq (,$(filter exynos5, $(TARGET_DEVICE)))

# if we have the oemcrypto sources mapped in, then use those unless an
# override has been provided. To force using the prebuilt while having
# the source, set
# HAVE_SLSI_EXYNOS5_OEMCRYPTO_SRC=false
#

BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 1

_should_use_oemcrypto_prebuilt := true
ifeq ($(wildcard vendor/widevine),vendor/widevine)
ifeq ($(wildcard vendor/samsung_slsi/proprietary),vendor/samsung_slsi/proprietary)
ifneq ($(HAVE_SLSI_EXYNOS5_OEMCRYPTO_SRC),false)
_should_use_oemcrypto_prebuilt := false
endif
endif

ifeq ($(_should_use_oemcrypto_prebuilt),true)

###############################################################################
# liboemcrypto.so for Modular DRM

include $(CLEAR_VARS)

LOCAL_MODULE := liboemcrypto_modular
LOCAL_MODULE_STEM := liboemcrypto
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)$(LOCAL_MODULE_SUFFIX)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_OWNER := samsung

include $(BUILD_PREBUILT)

###############################################################################
# liboemcrypto.a

include $(CLEAR_VARS)
LOCAL_MODULE := liboemcrypto
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_MULTILIB := 32
include $(BUILD_PREBUILT)

###############################################################################
# libdrmwvmplugin.so

include $(CLEAR_VARS)

BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 1
-include $(TOP)/vendor/widevine/proprietary/drmwvmplugin/plugin-core.mk

LOCAL_MODULE := libdrmwvmplugin
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/drm
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES += liboemcrypto
LOCAL_SHARED_LIBRARIES += libMcClient libion
LOCAL_MODULE_OWNER := samsung
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_MULTILIB := 32

include $(BUILD_SHARED_LIBRARY)

###############################################################################
# libwvm.so

include $(CLEAR_VARS)

BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 1
-include $(TOP)/vendor/widevine/proprietary/wvm/wvm-core.mk

LOCAL_MODULE := libwvm
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES += liboemcrypto
LOCAL_SHARED_LIBRARIES += libMcClient libion
LOCAL_MODULE_OWNER := samsung
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_MULTILIB := 32

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

endif # _should_use_oemcrypto_prebuilt == true
endif # $(wildcard vendor/widevine) == vendor/widevine

#endif
#endif
