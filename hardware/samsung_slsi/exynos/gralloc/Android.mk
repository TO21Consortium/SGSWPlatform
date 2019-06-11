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


LOCAL_PATH := $(call my-dir)

# HAL module implemenation stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH:= hw
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog libcutils libion libutils

ifneq ($(TARGET_SOC), exynos5420)
LOCAL_CFLAGS := -DUSES_EXYNOS_COMMON_GRALLOC
endif

MALI_AFBC_GRALLOC := 1

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/exynos5/include

LOCAL_SRC_FILES := 	\
	format_chooser.cpp \
	gralloc.cpp 	\
	framebuffer.cpp \
	mapper.cpp

LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM)
LOCAL_CFLAGS += -DLOG_TAG=\"gralloc\" -Wno-missing-field-initializers -DMALI_AFBC_GRALLOC=$(MALI_AFBC_GRALLOC)

ifeq ($(BOARD_USES_EXYNOS5_GRALLOC_RANGE_FLUSH), true)
LOCAL_CFLAGS += -DGRALLOC_RANGE_FLUSH
endif

ifeq ($(BOARD_USES_EXYNOS5_CRC_BUFFER_ALLOC), true)
LOCAL_CFLAGS += -DUSES_EXYNOS_CRC_BUFFER_ALLOC
endif

include $(BUILD_SHARED_LIBRARY)
