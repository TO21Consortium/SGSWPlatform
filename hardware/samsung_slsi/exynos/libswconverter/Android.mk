LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include frameworks/av/media/libstagefright/codecs/common/Config.mk

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	swconvertor.c

ifeq ($(VOTT), v7)
LOCAL_CFLAGS += -DNEON_SUPPORT
LOCAL_SRC_FILES += \
	csc_interleave_memcpy_neon.s \
	csc_BGRA8888_to_YUV420SP_NEON.s \
	csc_RGBA8888_to_YUV420SP_NEON.s \
	csc_BGRA8888_to_RGBA8888_NEON.s

ifeq ($(BOARD_USE_NV12T_128X64), true)
LOCAL_SRC_FILES += \
	csc_linear_to_tiled_crop_neon.s \
	csc_linear_to_tiled_interleave_crop_neon.s \
	csc_tiled_to_linear_crop_neon.s \
	csc_tiled_to_linear_deinterleave_crop_neon.s
LOCAL_CFLAGS += -DUSE_NV12T_128X64
else
LOCAL_SRC_FILES += \
	csc_tiled_to_linear_y_neon.s \
	csc_tiled_to_linear_uv_neon.s \
	csc_tiled_to_linear_uv_deinterleave_neon.s
endif

LOCAL_CFLAGS += -Werror
endif

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(TOP)/hardware/samsung_slsi/exynos/include

LOCAL_MODULE := libswconverter

LOCAL_PRELINK_MODULE := false

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_STATIC_LIBRARY)
