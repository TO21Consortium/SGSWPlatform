# Copyright (C) 2012 The Android Open Source Project
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

ifeq ($(BOARD_USES_HWC_SERVICES),true)

LOCAL_PATH:= $(call my-dir)
# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware_legacy libutils libbinder \
			  libexynosv4l2 libhdmi libhwcutils libsync
LOCAL_CFLAGS += -DLOG_TAG=\"HWCService\"

LOCAL_C_INCLUDES := \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/exynos/libhwcutils \
	$(TOP)/hardware/samsung_slsi/exynos/libexynosutils \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcmodule \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libdisplaymodule

ifeq ($(BOARD_USES_VPP), true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvppdisplay \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libdisplaymodule
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

ifeq ($(BOARD_TV_PRIMARY), true)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libhwc_tvprimary
else
LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/exynos/libhwc
endif

ifeq ($(BOARD_USES_VIRTUAL_DISPLAY), true)
ifeq ($(BOARD_USES_VPP), true)
	LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/libvppvirtualdisplay
else
	LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/libvirtualdisplay
endif
ifeq ($(BOARD_USES_VIRTUAL_DISPLAY_DECON_EXT_WB), true)
	LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libvirtualdisplaymodule
endif
	LOCAL_SHARED_LIBRARIES += libvirtualdisplay
ifeq ($(BOARD_USES_VDS_BGRA8888), true)
LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcutilsmodule \
	$(TOP)/hardware/samsung_slsi/exynos/libmpp
endif
endif

LOCAL_SRC_FILES := ExynosHWCService.cpp IExynosHWC.cpp

LOCAL_MODULE := libExynosHWCService
LOCAL_MODULE_TAGS := optional

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

endif
