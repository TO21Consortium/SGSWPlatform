# Copyright (C) 2014 The Android Open Source Project
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
# Primary Audio HAL
#
LOCAL_PATH := $(call my-dir)
DEVICE_BASE_PATH := $(TOP)/device/samsung

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	audio_hw.c

ifeq ($(BOARD_USE_SIPC_RIL), true)
LOCAL_SRC_FILES += \
	sec/voice_manager.c
else
LOCAL_SRC_FILES += \
	ww/voice_manager.c
endif

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	external/tinycompress/include \
	external/kernel-headers/original/uapi/sound \
	$(call include-path-for, audio-route) \
	$(call include-path-for, audio-utils) \
	$(DEVICE_BASE_PATH)/$(TARGET_BOOTLOADER_BOARD_NAME)/conf \
	external/expat/lib

ifeq ($(BOARD_USE_SIPC_RIL), true)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/sec
else
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/ww
endif

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libtinyalsa \
	libtinycompress \
	libaudioroute \
	libaudioutils \
	libdl \
	libexpat

ifeq ($(BOARD_USE_SIPC_RIL), true)
LOCAL_CFLAGS += -DSUPPORT_SIPC_RIL
endif

LOCAL_MODULE := audio.primary.$(TARGET_BOOTLOADER_BOARD_NAME)
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
