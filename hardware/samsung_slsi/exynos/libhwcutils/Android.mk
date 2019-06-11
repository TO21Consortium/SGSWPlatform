# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
#LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libexynosutils \
#libexynosv4l2 libsync libion_exynos libmpp
LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libexynosutils \
			  libexynosv4l2 libsync libion libmpp

ifeq ($(BOARD_DISABLE_HWC_DEBUG), true)
	LOCAL_CFLAGS += -DDISABLE_HWC_DEBUG
endif

ifeq ($(BOARD_USES_FIMC), true)
        LOCAL_SHARED_LIBRARIES += libexynosfimc
else
        LOCAL_SHARED_LIBRARIES += libexynosgscaler
endif

ifeq ($(BOARD_USES_FB_PHY_LINEAR),true)
	LOCAL_SHARED_LIBRARIES += libfimg
	LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/libfimg4x
	LOCAL_SRC_FILES += ExynosG2DWrapper.cpp
endif
LOCAL_CFLAGS += -DLOG_TAG=\"hwcutils\"
LOCAL_CFLAGS += -DHLOG_CODE=4
LOCAL_C_INCLUDES := \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../libhwc \
	$(TOP)/hardware/samsung_slsi/exynos/libexynosutils \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcmodule \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcutilsmodule \
	$(TOP)/hardware/samsung_slsi/exynos/libmpp

ifeq ($(BOARD_USES_VPP), true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvppdisplay
else
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libdisplay
endif

ifeq ($(BOARD_HDMI_INCAPABLE), true)
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/libhdmi_dummy
else
ifeq ($(BOARD_USES_VPP), true)
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpphdmi
else
ifeq ($(BOARD_USES_NEW_HDMI), true)
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/libhdmi
else
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/libhdmi_legacy
endif
endif
endif

LOCAL_SRC_FILES += \
	ExynosHWCUtils.cpp

ifeq ($(BOARD_USES_VPP), true)
LOCAL_SRC_FILES += \
	ExynosMPPv2.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvppdisplay \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libdisplaymodule
else
LOCAL_SRC_FILES += \
	ExynosMPP.cpp
endif

ifeq ($(BOARD_USES_VIRTUAL_DISPLAY), true)
ifeq ($(BOARD_USES_VPP), true)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libvppvirtualdisplay
else
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libvirtualdisplay \
	$(TOP)/hardware/samsung_slsi/exynos/libfimg4x
LOCAL_SHARED_LIBRARIES += libfimg
LOCAL_SHARED_LIBRARIES += libMcClient
LOCAL_STATIC_LIBRARIES := libsecurepath
LOCAL_SRC_FILES += ExynosG2DWrapper.cpp
endif
endif

include $(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcutilsmodule/Android.mk

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libhwcutils

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

