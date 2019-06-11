# Copyright (C) 2013 The Android Open Source Project
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
LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libexynosutils libexynosv4l2

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/exynos/libexynosutils

LOCAL_SRC_FILES := libscaler.cpp libscaler-v4l2.cpp libscalerblend-v4l2.cpp libscaler-m2m1shot.cpp libscaler-swscaler.cpp
ifeq ($(BOARD_USES_SCALER_M2M1SHOT), true)
LOCAL_CFLAGS += -DSCALER_USE_M2M1SHOT
endif

# since 3.18 kernel
ifneq ($(filter 3.18, $(TARGET_LINUX_KERNEL_VERSION)),)
LOCAL_CFLAGS += -DSCALER_USE_LOCAL_CID
LOCAL_CFLAGS += -DSCALER_USE_PREMUL_FMT
endif

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libexynosscaler

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)
