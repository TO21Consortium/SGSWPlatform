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
# libexynoscamera3

include $(CLEAR_VARS)

######## System LSI ONLY ########
BOARD_CAMERA_GED_FEATURE := true
#################################

LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware libui
LOCAL_SHARED_LIBRARIES += libexynosutils libhwjpeg libexynosv4l2 libexynosgscaler libion libcsc
LOCAL_SHARED_LIBRARIES += libexpat libc++
LOCAL_SHARED_LIBRARIES += libpower

ifeq ($(BOARD_CAMERA_GED_FEATURE), true)
else
LOCAL_SHARED_LIBRARIES += libsecnativefeature
LOCAL_SHARED_LIBRARIES += libuniplugin
endif

ifeq ($(BOARD_CAMERA_SW_VDIS), true)
LOCAL_SHARED_LIBRARIES += libvdis
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

LOCAL_CFLAGS += -DUSE_CAMERA2_API_SUPPORT

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../libcamera3 \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libion/include \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/SensorInfos \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Pipes2 \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/MCPipes \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Activities \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Buffers \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/common_v2/Ged \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx \
	$(TOP)/hardware/samsung_slsi/exynos/libcamera/34xx/hal3 \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/libcamera3 \
	$(TOP)/hardware/libhardware_legacy/include/hardware_legacy \
	$(TOP)/vendor/samsung/feature/CscFeature/libsecnativefeature \
	$(TOP)/bionic \
	$(TOP)/external/expat/lib \
	$(TOP)/external/libcxx/include \
	$(TOP)/frameworks/native/include \
	$(TOP)/hardware/camera/SensorListener \
	$(TOP)/hardware/camera/UniPlugin/include

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcamera/SensorInfos

LOCAL_SRC_FILES:= \
	../../exynos/libcamera/common_v2/ExynosCameraFrame.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraMemory.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraFrameManager.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraUtils.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraNode.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraNodeJpegHAL.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraFrameSelector.cpp \
	../../exynos/libcamera/common_v2/SensorInfos/ExynosCameraSensorInfoBase.cpp \
	../../exynos/libcamera/common_v2/SensorInfos/ExynosCamera3SensorInfoBase.cpp \
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
	../../exynos/libcamera/common_v2/ExynosCameraRequestManager.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraStreamManager.cpp \
	../../exynos/libcamera/common_v2/ExynosCameraMetadataConverter.cpp \
	../../exynos/libcamera/common_v2/Ged/ExynosCameraActivityAutofocusVendor.cpp \
	../../exynos/libcamera/common_v2/Ged/ExynosCameraActivityFlashVendor.cpp \
	../../exynos/libcamera/common_v2/Ged/ExynosCameraFrameSelectorVendor.cpp \
	../../exynos/libcamera/34xx/ExynosCameraActivityControl.cpp\
	../../exynos/libcamera/34xx/ExynosCameraScalableSensor.cpp \
	../../exynos/libcamera/34xx/ExynosCameraUtilsModule.cpp \
#HAL 3.0 source
LOCAL_SRC_FILES += \
	../../exynos/libcamera/34xx/hal3/ExynosCameraSizeControl.cpp \
	../../exynos/libcamera/34xx/hal3/ExynosCamera3.cpp \
	../../exynos/libcamera/34xx/hal3/ExynosCamera3Parameters.cpp \
	../../exynos/libcamera/34xx/hal3/ExynosCamera3FrameFactory.cpp \
	../../exynos/libcamera/34xx/hal3/ExynosCamera3FrameFactoryPreview.cpp \
	../../exynos/libcamera/34xx/hal3/ExynosCamera3FrameReprocessingFactory.cpp \

LOCAL_SRC_FILES += ../libcamera/SensorInfos/ExynosCamera3SensorInfo.cpp

$(foreach file,$(LOCAL_SRC_FILES),$(shell touch '$(LOCAL_PATH)/$(file)'))

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libexynoscamera3

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

$(warning ##########################################)
$(warning ##########################################)
$(warning ########     libcamera 3     #############)
$(warning ##########################################)
$(warning ##########################################)
