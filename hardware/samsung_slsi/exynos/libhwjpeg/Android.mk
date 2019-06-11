# Copyright (C) 2015 The Android Open Source Project
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

LOCAL_CFLAGS += -DLOG_TAG=\"exynos-libhwjpeg\"

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libcsc libion

LOCAL_C_INCLUDES := $(TOP)/hardware/samsung_slsi/exynos/include \
                    $(TOP)/system/core/libion/include

LOCAL_SRC_FILES := hwjpeg-base.cpp hwjpeg-v4l2.cpp libhwjpeg-exynos.cpp
ifeq ($(TARGET_USES_UNIVERSAL_LIBHWJPEG), true)
LOCAL_SRC_FILES += ExynosJpegEncoder.cpp libcsc.cpp AppMarkerWriter.cpp ExynosJpegEncoderForCamera.cpp
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libhwjpeg

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)
