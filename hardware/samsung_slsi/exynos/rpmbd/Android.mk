#
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
#

LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_USES_RPMB), true)
include $(CLEAR_VARS)

LOCAL_MODULE := rpmbd
LOCAL_SRC_FILES := \
		rpmbd.c
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_MODULE_TAGS := optional
ifeq ($(BOARD_USES_MMC_RPMB), true)
LOCAL_CFLAGS := -DUSE_MMC_RPMB
endif

ifeq ($(TARGET_IS_64_BIT), true)
LOCAL_CFLAGS += -DTARGET_IS_64_BIT
endif

include $(BUILD_EXECUTABLE)

endif
