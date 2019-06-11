LOCAL_PATH := $(call my-dir)
EXYNOS_AUDIO_MODE := $(LOCAL_PATH)

include $(CLEAR_VARS)

ifeq ($(BOARD_USE_SEIREN_AUDIO), true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=  dec/seiren_hw.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include

LOCAL_MODULE := libseirenhw

LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES :=

include $(BUILD_STATIC_LIBRARY)
endif
