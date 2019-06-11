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
# Modified 2011 by InvenSense, Inc




LOCAL_PATH := $(call my-dir)

ifneq ($(TARGET_SIMULATOR),true)

include $(CLEAR_VARS)

# HAL module implemenation, not prelinked, and stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so

LOCAL_MULTILIB := first

LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_RELATIVE_PATH := hw


LOCAL_C_INCLUDES += $(LOCAL_PATH)/

LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_SRC_FILES := 						\
				sensors.cpp 			\
				SensorBase.cpp			\
				InputEventReader.cpp		\
				Stm_sensors_hub_Sensor.cpp


LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libdl libc

include $(BUILD_SHARED_LIBRARY)

endif
