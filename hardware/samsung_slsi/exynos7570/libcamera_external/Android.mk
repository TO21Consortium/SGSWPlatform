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

LOCAL_PATH:= $(call my-dir)

###################
# libexynoscameraexternal

soc_prj_list := swatch
project_camera := $(strip $(foreach p,$(soc_prj_list),$(if $(findstring $(p),$(TARGET_PRODUCT)),$(p))))

include $(CLEAR_VARS)

######## System LSI ONLY ########
BOARD_CAMERA_GED_FEATURE := true
############################

LOCAL_PRELINK_MODULE := false

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libion/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera_external \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2 \
	$(TOP)/hardware/libhardware/include/hardware \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/vendor/samsung/feature/CscFeature/libsecnativefeature \
	$(TOP)/bionic \
	$(TOP)/external/expat/lib \
	frameworks/native/include

LOCAL_SRC_FILES:= \
	../../exynos/libcamera_external/Exif.cpp \
	../../exynos/libcamera_external/SecCameraParameters.cpp \
	../../exynos/libcamera_external/ISecCameraHardware.cpp \
	../../exynos/libcamera_external/SecCameraHardware.cpp \
	../../exynos/libcamera_external/SecCameraHardware1MetadataConverter.cpp \

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware libgui
LOCAL_SHARED_LIBRARIES += libexynosutils libhwjpeg libexynosv4l2 libcsc libion libcamera_metadata
ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
else
LOCAL_SHARED_LIBRARIES += libsecnativefeature libexpat
endif

ifeq ($(TARGET_USES_UNIVERSAL_LIBHWJPEG), true)
	LOCAL_CFLAGS += -DUSES_UNIVERSAL_LIBHWJPEG
endif

LOCAL_CFLAGS += -DGAIA_FW_BETA
LOCAL_CFLAGS += -DBACK_CAMERA_SENSOR_NAME=$(BOARD_BACK_CAMERA_SENSOR)
LOCAL_CFLAGS += -DFRONT_CAMERA_SENSOR_NAME=$(BOARD_FRONT_CAMERA_SENSOR)
LOCAL_CFLAGS += -DBACK_ROTATION=$(BOARD_BACK_CAMERA_ROTATION)
LOCAL_CFLAGS += -DFRONT_ROTATION=$(BOARD_FRONT_CAMERA_ROTATION)

ifeq ($(BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA), true)
	LOCAL_CFLAGS += -DBOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA
endif

ifeq ($(BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA), true)
	LOCAL_CFLAGS += -DBOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA
endif

ifeq ($(BOARD_FRONT_CAMERA_ONLY_USE), true)
	LOCAL_CFLAGS += -DBOARD_FRONT_CAMERA_ONLY_USE
endif

ifeq ($(BOARD_USE_MHB_ION), true)
LOCAL_CFLAGS += -DBOARD_USE_MHB_ION
endif

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
LOCAL_CFLAGS += -DCAMERA_GED_FEATURE
endif

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
else
LOCAL_CFLAGS += -DUSE_CSC_FEATURE
endif

LOCAL_CFLAGS += -D$(shell echo $(project_camera) | tr a-z A-Z)_CAMERA


LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libexynoscameraexternal

include $(BUILD_SHARED_LIBRARY)


#################
# camera.exynos7570.so

include $(CLEAR_VARS)

######## System LSI ONLY ########
BOARD_CAMERA_GED_FEATURE := true
############################

# HAL module implemenation stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libion/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera_external \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2 \
	$(TOP)/hardware/libhardware/include/hardware \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	frameworks/native/include \

LOCAL_SRC_FILES:= \
	../../exynos/libcamera_external/SecCameraInterface.cpp

LOCAL_CFLAGS += -DGAIA_FW_BETA
LOCAL_CFLAGS += -DBACK_CAMERA_SENSOR_NAME=$(BOARD_BACK_CAMERA_SENSOR)
LOCAL_CFLAGS += -DFRONT_CAMERA_SENSOR_NAME=$(BOARD_FRONT_CAMERA_SENSOR)
LOCAL_CFLAGS += -DBACK_ROTATION=$(BOARD_BACK_CAMERA_ROTATION)
LOCAL_CFLAGS += -DFRONT_ROTATION=$(BOARD_FRONT_CAMERA_ROTATION)


ifeq ($(BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA), true)
	LOCAL_CFLAGS += -DBOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA
endif

ifeq ($(BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA), true)
	LOCAL_CFLAGS += -DBOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA
endif

ifeq ($(BOARD_FRONT_CAMERA_ONLY_USE), true)
	LOCAL_CFLAGS += -DBOARD_FRONT_CAMERA_ONLY_USE
endif

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
LOCAL_CFLAGS += -DCAMERA_GED_FEATURE
endif

ifeq ($(TARGET_USES_UNIVERSAL_LIBHWJPEG), true)
	LOCAL_CFLAGS += -DUSES_UNIVERSAL_LIBHWJPEG
endif

LOCAL_CFLAGS += -D$(shell echo $(project_camera) | tr a-z A-Z)_CAMERA

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware
LOCAL_SHARED_LIBRARIES += libexynosutils libhwjpeg libexynosv4l2 libcsc libion libexynoscameraexternal

LOCAL_MODULE := camera.$(TARGET_BOOTLOADER_BOARD_NAME)

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
