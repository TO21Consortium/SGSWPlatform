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
# libexynoscamera

include $(CLEAR_VARS)

######## System LSI ONLY ########
BOARD_CAMERA_GED_FEATURE := true
#################################

LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware libui
LOCAL_SHARED_LIBRARIES += libexynosutils libhwjpeg libexynosv4l2 libexynosgscaler libion libcsc
LOCAL_SHARED_LIBRARIES += libexpat libc++
LOCAL_SHARED_LIBRARIES += libpower
LOCAL_SHARED_LIBRARIES += libgui

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
else
LOCAL_SHARED_LIBRARIES += libsecnativefeature
LOCAL_SHARED_LIBRARIES += libliveframework
endif

LOCAL_CFLAGS += -DGAIA_FW_BETA
LOCAL_CFLAGS += -DMAIN_CAMERA_SENSOR_NAME=$(BOARD_BACK_CAMERA_SENSOR)
LOCAL_CFLAGS += -DFRONT_CAMERA_SENSOR_NAME=$(BOARD_FRONT_CAMERA_SENSOR)
LOCAL_CFLAGS += -DUSE_CAMERA_ESD_RESET
LOCAL_CFLAGS += -DBACK_ROTATION=$(BOARD_BACK_CAMERA_ROTATION)
LOCAL_CFLAGS += -DFRONT_ROTATION=$(BOARD_FRONT_CAMERA_ROTATION)
ifeq ($(TARGET_BOOTLOADER_BOARD_NAME), universal7570)
#LOCAL_CFLAGS += -DUNIVERSAL_CAMERA
endif

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
LOCAL_CFLAGS += -DCAMERA_GED_FEATURE
endif

LOCAL_CFLAGS += -D$(shell echo $(project_camera) | tr a-z A-Z)_CAMERA

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../libcamera \
	$(LOCAL_PATH)/../libcamera/SensorInfos \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libion/include \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx/hal1 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/SensorInfos \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Pipes2 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/MCPipes \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Activities \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Buffers \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Ged \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/libcamera \
	$(TOP)/hardware/samsung_slsi/exynos7570/libcamera \
	$(TOP)/hardware/libhardware_legacy/include/hardware_legacy \
	$(TOP)/vendor/samsung/feature/CscFeature/libsecnativefeature \
	$(TOP)/bionic \
	$(TOP)/external/expat/lib \
	$(TOP)/external/libcxx/include \
	$(TOP)/frameworks/native/include \
	$(TOP)/hardware/camera/UniPlugin/include

ifneq ($(LOCAL_PROJECT_DIR),)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcameraSec/$(LOCAL_PROJECT_DIR)
else
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcamera/SensorInfos
endif

LOCAL_SRC_FILES:= \
	../../exynos/libcamera/common_v2/ExynosCameraFrame.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraMemory.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraFrameManager.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraUtils.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraNode.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraNodeJpegHAL.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraFrameSelector.cpp \
	../../exynos/libcamera/common_v2/ExynosCamera1MetadataConverter.cpp \
	../../exynos/libcamera/common_v2/SensorInfos/ExynosCameraSensorInfoBase.cpp \
	../../exynos/libcamera/common_v2/SensorInfos/ExynosCameraSensorInfo2P8.cpp \
	../../exynos/libcamera/common_v2/MCPipes/ExynosCameraMCPipe.cpp \
	../../exynos/libcamera/common_v2/Pipes2/ExynosCameraPipe.cpp \
	../../exynos/libcamera/common_v2/Pipes2/ExynosCameraPipeFlite.cpp \
	../../exynos/libcamera/common_v2/Pipes2/ExynosCameraPipeVRA.cpp \
	../../exynos/libcamera/common_v2/Pipes2/ExynosCameraPipeGSC.cpp \
	../../exynos/libcamera/common_v2/Pipes2/ExynosCameraPipeJpeg.cpp \
	../../exynos/libcamera/common_v2/Buffers/ExynosCameraBufferManager.cpp \
	../../exynos/libcamera/common_v2/Activities/ExynosCameraActivityBase.cpp \
	../../exynos/libcamera/common_v2/Activities/ExynosCameraActivityAutofocus.cpp \
	../../exynos/libcamera/common_v2/Activities/ExynosCameraActivityFlash.cpp \
	../../exynos/libcamera/common_v2/Activities/ExynosCameraActivitySpecialCapture.cpp \
	../../exynos/libcamera/common_v2/Activities/ExynosCameraActivityUCTL.cpp \
	../../exynos/libcamera/common_v2/Ged/ExynosCameraActivityAutofocusVendor.cpp \
	../../exynos/libcamera/common_v2/Ged/ExynosCameraActivityFlashVendor.cpp \
	../../exynos/libcamera/common_v2/Ged/ExynosCameraFrameSelectorVendor.cpp \
	../../exynos/libcamera/34xx/ExynosCameraUtilsModule.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraSizeControl.cpp \
	../../exynos/libcamera/34xx/ExynosCameraActivityControl.cpp\
	../../exynos/libcamera/34xx/ExynosCameraScalableSensor.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCamera.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCamera1Parameters.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameFactory.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameFactoryPreview.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameFactory3aaIspM2M.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameFactory3aaIspM2MTpu.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameFactory3aaIspOtf.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameFactory3aaIspOtfTpu.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameReprocessingFactory.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameReprocessingFactoryNV21.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameFactoryVision.cpp \
	../../exynos/libcamera/34xx/hal1/ExynosCameraFrameFactoryFront.cpp \
	../../exynos/libcamera/34xx/hal1/Ged/ExynosCameraVendor.cpp \
	../../exynos/libcamera/34xx/hal1/Ged/ExynosCamera1ParametersVendor.cpp

ifneq ($(LOCAL_PROJECT_DIR),)
LOCAL_SRC_FILES += ../libcameraSec/$(LOCAL_PROJECT_DIR)/ExynosCameraSensorInfo.cpp
else
LOCAL_SRC_FILES += ../libcamera/SensorInfos/ExynosCameraSensorInfo.cpp
endif

$(foreach file,$(LOCAL_SRC_FILES),$(shell touch '$(LOCAL_PATH)/$(file)'))

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libexynoscamera

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

$(warning #####################################)
$(warning ########    libcamera 1.0    ########)
$(warning #####################################)
