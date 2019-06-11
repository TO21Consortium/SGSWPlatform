ifeq ($(BOARD_USE_OFFLOAD_EFFECT),true)
LOCAL_PATH:= $(call my-dir)

# Exynos Offload Visualizer library
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	exynos_visualizer.c

LOCAL_C_INCLUDES := \
	external/tinyalsa/include \
	$(call include-path-for, audio-effects)

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libtinyalsa

LOCAL_CFLAGS+= -O2 -fvisibility=hidden

LOCAL_MODULE:= libexynosvisualizer
LOCAL_MODULE_RELATIVE_PATH := soundfx

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif
