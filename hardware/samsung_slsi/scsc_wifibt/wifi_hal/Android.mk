#############################################################################
#
# Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd
#
#############################################################################

ifeq ($(CONFIG_SAMSUNG_SCSC_WIFIBT),true)

LOCAL_PATH := $(call my-dir)

# Make the HAL library
# ============================================================
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wno-unused-parameter

LOCAL_C_INCLUDES += \
        external/libnl/include \
        $(call include-path-for, libhardware_legacy)/hardware_legacy \
        external/wpa_supplicant_8/src/drivers

LOCAL_SRC_FILES := \
	wifi_hal.cpp \
	rtt.cpp \
	common.cpp \
	cpp_bindings.cpp \
	gscan.cpp \
	link_layer_stats.cpp \
	wifi_offload.cpp

LOCAL_MODULE := libwifi-hal-slsi

include $(BUILD_STATIC_LIBRARY)

endif
