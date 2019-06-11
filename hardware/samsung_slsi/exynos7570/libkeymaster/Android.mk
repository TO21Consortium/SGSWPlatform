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

LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

MOBICORE_PATH := hardware/samsung_slsi/$(TARGET_SOC)/mobicore

LOCAL_MODULE := keystore.exynos7570
LOCAL_MODULE_RELATIVE_PATH := hw

ifeq ($(BOARD_USES_KEYMASTER_VER1), true)
LOCAL_CPPFLAGS := -Wall
LOCAL_CPPFLAGS += -Wextra
LOCAL_CPPFLAGS += -Werror

ALL_SRC_FILES := $(wildcard ${LOCAL_PATH}/ver1/src/*.cpp \
                            ${LOCAL_PATH}/ver1/src/*.c)
LOCAL_SRC_FILES := $(ALL_SRC_FILES:$(LOCAL_PATH)/ver1/%=ver1/%)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/ver1/include
else
LOCAL_SRC_FILES := \
	ver0/keymaster_mobicore.cpp \
	ver0/tlcTeeKeymaster_if.c
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/ver0 \
	$(MOBICORE_PATH)/daemon/ClientLib/public \
	$(MOBICORE_PATH)/common/MobiCore/inc/
LOCAL_C_FLAGS = -fvisibility=hidden -Wall -Werror

ifeq ($(BOARD_USES_KEYMASTER_VER0_3), true)
	LOCAL_CFLAGS += -DKEYMASTER_VER0_3
endif
endif

LOCAL_SHARED_LIBRARIES := libcrypto liblog libMcClient libcutils
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)
