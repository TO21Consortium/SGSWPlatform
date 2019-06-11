ifeq ($(BOARD_USE_OFFLOAD_EFFECT),true)
LOCAL_PATH:= $(call my-dir)

# music nxp offload bundle wrapper
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	Bundle/exynos_effectbundle.cpp

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	$(LOCAL_PATH)/Bundle \
	$(call include-path-for, audio-effects)

LOCAL_CFLAGS += -fvisibility=hidden

LOCAL_MODULE:= libhwnxpbundlewrapper

LOCAL_MODULE_RELATIVE_PATH := soundfx

LOCAL_SHARED_LIBRARIES := \
     libcutils \
     libdl \
     libtinyalsa

include $(BUILD_SHARED_LIBRARY)

# music nxp offload reverb wrapper
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	Reverb/exynos_effectReverb.cpp

LOCAL_CFLAGS += -fvisibility=hidden

LOCAL_MODULE:= libhwnxpreverbwrapper

LOCAL_MODULE_RELATIVE_PATH := soundfx

LOCAL_SHARED_LIBRARIES := \
	 libcutils \
	 libdl \
	 libtinyalsa

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	$(LOCAL_PATH)/Reverb \
	$(LOCAL_PATH)/Bundle \
	$(call include-path-for, audio-effects)

include $(BUILD_SHARED_LIBRARY)
endif
