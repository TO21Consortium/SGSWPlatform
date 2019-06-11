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

#################
# camera.exynos7570.so

include $(CLEAR_VARS)

######## System LSI ONLY ########
BOARD_CAMERA_GED_FEATURE := true
#################################

# HAL module implemenation stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so
LOCAL_MODULE_RELATIVE_PATH := hw

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libcamera \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libcamera/Vendor \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libcamera3 \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libion/include \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx/hal1 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx/hal3 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/SensorInfos \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Pipes2 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/MCPipes \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Activities \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Buffers \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Sec \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	frameworks/native/include \
	$(TOP)/external/libcxx/include \
	$(TOP)/bionic
else
LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libcameraSec \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libcameraSec/Vendor \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/libcamera3Sec \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libion/include \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx/hal1 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx/hal3 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/SensorInfos \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Pipes2 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/MCPipes \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Activities \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Buffers \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Sec \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	frameworks/native/include \
	$(TOP)/external/libcxx/include \
	$(TOP)/bionic \
	$(TOP)/hardware/camera/SensorListener \
	$(TOP)/hardware/camera/UniPlugin/include
endif

ifneq ($(LOCAL_PROJECT_DIR),)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcameraSec/$(LOCAL_PROJECT_DIR)
else
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcamera/SensorInfos
endif

ifeq ($(BOARD_CAMERA_HAL3_FEATURE), true)
LOCAL_SRC_FILES:= \
	../../exynos/libcamera/common_v2/ExynosCamera3Interface.cpp
else
LOCAL_SRC_FILES:= \
	../../exynos/libcamera/common_v2/ExynosCameraInterface.cpp
endif

LOCAL_CFLAGS += -DBACK_ROTATION=$(BOARD_BACK_CAMERA_ROTATION)
LOCAL_CFLAGS += -DFRONT_ROTATION=$(BOARD_FRONT_CAMERA_ROTATION)

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
LOCAL_CFLAGS += -DCAMERA_GED_FEATURE
endif
ifeq ($(BOARD_CAMERA_HAL3_FEATURE), true)
LOCAL_CFLAGS += -DUSE_CAMERA2_API_SUPPORT
endif

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware
LOCAL_SHARED_LIBRARIES += libexynosutils libhwjpeg libexynosv4l2 libcsc libion libcamera_metadata libexynoscamera

ifeq ($(BOARD_CAMERA_HAL3_FEATURE), true)
LOCAL_SHARED_LIBRARIES += libexynoscamera3
endif

$(foreach file,$(LOCAL_SRC_FILES),$(shell touch '$(LOCAL_PATH)/$(file)'))

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
LOCAL_MODULE := camera.$(TARGET_BOOTLOADER_BOARD_NAME)
else
LOCAL_MODULE := camera.$(TARGET_BOOTLOADER_BOARD_NAME)
# Temporary modified
#LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
endif
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -D$(shell echo $(project_camera) | tr a-z A-Z)_CAMERA

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

$(warning #####################################)
$(warning ########    libcamera I/F    ########)
$(warning #####################################)
