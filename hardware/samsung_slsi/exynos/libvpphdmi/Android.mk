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
LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libexynosutils \
  libexynosv4l2 libsync libhwcutils libdisplay libmpp

LOCAL_CFLAGS += -DLOG_TAG=\"hdmi\"

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../libhwcutils \
	$(LOCAL_PATH)/../libvppdisplay \
	$(LOCAL_PATH)/../libhwc \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/exynos/libexynosutils \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcmodule \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcutilsmodule \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libdisplaymodule \
	$(TOP)/hardware/samsung_slsi/exynos/libmpp \
	$(TOP)/system/core/libsync/include

ifeq ($(filter 3.10, $(TARGET_LINUX_KERNEL_VERSION)), 3.10)
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/kernel-3.10-headers
else
ifeq ($(filter 3.18, $(TARGET_LINUX_KERNEL_VERSION)), 3.18)
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/kernel-3.18-headers
else
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/kernel-3.4-headers
endif
endif

LOCAL_SRC_FILES := \
	ExynosExternalDisplay.cpp dv_timings.c

include $(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhdmimodule/Android.mk

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libhdmi

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

