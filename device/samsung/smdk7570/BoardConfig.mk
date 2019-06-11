#
# Copyright (C) 2015 The Android Open-Source Project
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

TARGET_LINUX_KERNEL_VERSION := 3.18

TARGET_BOARD_INFO_FILE := device/samsung/smdk7570/board-info.txt

# HACK : To fix up after bring up multimedia devices.
TARGET_SOC := exynos7570

# pure 32-bit platform build
TARGET_PLATFORM_32BIT := true

ifneq ($(TARGET_PLATFORM_32BIT), true)
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_VARIANT := generic

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv7-a-neon
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := cortex-a15
else
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a15
endif

TARGET_CPU_SMP := true

TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := true
TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_PLATFORM := exynos5
TARGET_BOOTLOADER_BOARD_NAME := smdk7570
TARGET_AP_VER := evt1

# bionic libc options
BOARD_MEMCPY_AARCH32 := true

# SMDK common modules
BOARD_SMDK_COMMON_MODULES := liblight

#OVERRIDE_RS_DRIVER := libRSDriverArm.so
#BOARD_USES_HGL := true
USE_OPENGL_RENDERER := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3

BOARD_EGL_CFG := device/samsung/smdk7570/conf/egl.cfg

# Storage options
BOARD_USES_SDMMC_BOOT := false

TARGET_BOARD_PLATFORM := exynos5
TARGET_AP_VER := evt1

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 576716800
BOARD_USERDATAIMAGE_PARTITION_SIZE := 576716800
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_PARTITION_SIZE := 69206016
BOARD_FLASH_BLOCK_SIZE := 4096
BOARD_MOUNT_SDCARD_RW := true

# AUDIO
BOARD_USE_ALP_AUDIO := false
BOARD_USE_SEIREN_AUDIO := false
BOARD_USE_COMMON_AUDIOHAL := true
BOARD_USE_SIPC_RIL := false
BOARD_USE_OFFLOAD_AUDIO := false
BOARD_USE_OFFLOAD_EFFECT := false

# HWComposer
BOARD_USES_VPP := true

# HWC Bring up
BOARD_USES_HWC_TINY := false


# HWCServices
BOARD_USES_HWC_SERVICES := true

# SCALER
BOARD_USES_DEFAULT_CSC_HW_SCALER := true
BOARD_USES_SCALER_M2M1SHOT := true

# HDMI
BOARD_HDMI_INCAPABLE := true

# Gralloc
BOARD_USES_EXYNOS5_COMMON_GRALLOC := true

# Device Tree
BOARD_USES_DT := true

# PLATFORM LOG
TARGET_USES_LOGD := true

#FMP
#BOARD_USES_FMP_DM_CRYPT := true

# FIMG2D
BOARD_USES_SKIA_FIMGAPI := false

# Samsung exynos init.rc
BOARD_USE_EXYNOS_INIT_RC := true

# ART
BOARD_EXYNOS_ART_OPT := true
BOARD_EXYNOS_ART_OPTIMIZING_COMPILER_ON := true
BOARD_EXYNOS_ART_QUICK_COMPILER_ON := true
ADDITIONAL_BUILD_PROPERTIES += dalvik.vm.image-dex2oat-filter=speed
ADDITIONAL_BUILD_PROPERTIES += dalvik.vm.dex2oat-filter=speed

# CAMERA
BOARD_BACK_CAMERA_ROTATION := 90
BOARD_FRONT_CAMERA_ROTATION := 90
BOARD_BACK_CAMERA_SENSOR := SENSOR_NAME_S5K3M2
BOARD_FRONT_CAMERA_SENSOR := SENSOR_NAME_S5K5E2

# V8
BOARD_EXYNOS_V8_OPT := true

ifeq ($(TARGET_PLATFORM_32BIT), true)
# this macro should be changed depending on 32/64-bit kernel build
BOARD_USES_DECON_64BIT_ADDRESS := true
endif

# Samsung OpenMAX Video
BOARD_USE_STOREMETADATA := true
BOARD_USE_METADATABUFFERTYPE := true
BOARD_USE_DMA_BUF := true
BOARD_USE_ANB_OUTBUF_SHARE := true
BOARD_USE_IMPROVED_BUFFER := true
BOARD_USE_NON_CACHED_GRAPHICBUFFER := true
BOARD_USE_GSC_RGB_ENCODER := true
BOARD_USE_CSC_HW := false
BOARD_USE_QOS_CTRL := false
BOARD_USE_S3D_SUPPORT := false
BOARD_USE_TIMESTAMP_REORDER_SUPPORT := true
BOARD_USE_DEINTERLACING_SUPPORT := true
BOARD_USE_VP8ENC_SUPPORT := true
BOARD_USE_HEVCDEC_SUPPORT := true
BOARD_USE_HEVCENC_SUPPORT := true
BOARD_USE_HEVC_HWIP := false
BOARD_USE_VP9DEC_SUPPORT := false
BOARD_USE_VP9ENC_SUPPORT := false
BOARD_USE_CUSTOM_COMPONENT_SUPPORT := true
BOARD_USE_VIDEO_EXT_FOR_WFD_HDCP := true
BOARD_USE_SINGLE_PLANE_IN_DRM := false
BOARD_USE_SINGLE_DRM := true

# LIBHWJPEG
TARGET_USES_UNIVERSAL_LIBHWJPEG := true

#Keymaster
#BOARD_USES_KEYMASTER_VER1 := true

#RPMB
#BOARD_USES_MMC_RPMB := true

# WiFi related defines
#BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
#BOARD_HOSTAPD_DRIVER             := NL80211
#BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_slsi
#BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_slsi
#BOARD_HAS_SAMSUNG_WLAN           := true
#WPA_SUPPLICANT_VERSION           := VER_0_8_X
#BOARD_WLAN_DEVICE                := slsi
#WIFI_DRIVER_MODULE_ARG           := ""
#WLAN_VENDOR                      := 8

# Bluetooth related defines
#BOARD_HAVE_BLUETOOTH := true
#BOARD_HAVE_BLUETOOTH_SLSI := true
#BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := hardware/samsung_slsi/libbt/include
#BLUEDROID_HCI_VENDOR_STATIC_LINKING := false

# Enable BT/WIFI related code changes in Android source files
#CONFIG_SAMSUNG_SCSC_WIFIBT       := true

# CURL
BOARD_USES_CURL := true

# SELinux Policy DIR
BOARD_SEPOLICY_DIRS = device/samsung/smdk7570/sepolicy
