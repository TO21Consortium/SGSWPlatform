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
	libexynosv4l2 libhwcutils libsync libmpp

ifeq ($(BOARD_USES_FIMC), true)
LOCAL_SHARED_LIBRARIES += libexynosfimc
else
LOCAL_SHARED_LIBRARIES += libexynosgscaler
endif

ifeq ($(BOARD_USES_FB_PHY_LINEAR),true)
	LOCAL_SHARED_LIBRARIES += libfimg
	LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi/exynos/libfimg4x
endif

LOCAL_CFLAGS += -DLOG_TAG=\"display\"
LOCAL_CFLAGS += -DHLOG_CODE=1
LOCAL_C_INCLUDES := \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../libhwc \
	$(LOCAL_PATH)/../libhwcutils \
	$(TOP)/hardware/samsung_slsi/exynos/libexynosutils \
	$(TOP)/hardware/samsung_slsi/exynos/libmpp \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcmodule \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libhwcutilsmodule

LOCAL_SRC_FILES := \
	ExynosDisplay.cpp \
	ExynosOverlayDisplay.cpp

include $(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libdisplaymodule/Android.mk

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libdisplay

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

