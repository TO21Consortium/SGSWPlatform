#
# Copyright (C) 2014 Samsung Electronics S.LSI
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

include $(CLEAR_VARS)
LOCAL_MODULE := libexynosart
LOCAL_SRC_FILES := libexynosart_arm32.a
LOCAL_MODULE_OWNER := samsung_arm
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_32_BIT_ONLY := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libexynosart
LOCAL_SRC_FILES := libexynosart_host32.a
LOCAL_MODULE_OWNER := samsung
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_IS_HOST_MODULE := true
LOCAL_32_BIT_ONLY := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libexynosartd
LOCAL_SRC_FILES := libexynosartd_host32.a
LOCAL_MODULE_OWNER := samsung
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_IS_HOST_MODULE := true
LOCAL_32_BIT_ONLY := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libexynosart
LOCAL_SRC_FILES := libexynosart_host64.a
LOCAL_MODULE_OWNER := samsung
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_IS_HOST_MODULE := true
LOCAL_32_BIT_ONLY := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libexynosartd
LOCAL_SRC_FILES := libexynosartd_host64.a
LOCAL_MODULE_OWNER := samsung
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_IS_HOST_MODULE := true
LOCAL_32_BIT_ONLY := false
include $(BUILD_PREBUILT)
