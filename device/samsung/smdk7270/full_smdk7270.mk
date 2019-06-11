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
# This file is the build configuration for a full Android
# build for smdk7270 hardware. This cleanly combines a set of
# device-specific aspects (drivers) with a device-agnostic
# product configuration (apps). Except for a few implementation
# details, it only fundamentally contains two inherit-product
# lines, full and smdk7270, hence its name.
#

# Live Wallpapers
#
# 32-bit platform build flag. TARGET_PLATFORM_32BIT should be set to false for 64bit build
TARGET_PLATFORM_32BIT := true

PRODUCT_PACKAGES += \
        LiveWallpapers \
        LiveWallpapersPicker \
        MagicSmokeWallpapers \
        VisualizationWallpapers \
        librs_jni

PRODUCT_PROPERTY_OVERRIDES := \
        net.dns1=8.8.8.8 \
        net.dns2=8.8.4.4

# Inherit from those products. Most specific first.
$(call inherit-product, device/samsung/smdk7270/device.mk)
ifneq ($(TARGET_PLATFORM_32BIT), true)
$(call inherit-product, build/target/product/aosp_arm64.mk)
else
$(call inherit-product, build/target/product/aosp_arm.mk)
endif

PRODUCT_NAME := full_smdk7270
PRODUCT_DEVICE := smdk7270
PRODUCT_BRAND := Android
PRODUCT_MODEL := Full Android on SMDK7270
PRODUCT_MANUFACTURER := Samsung Electronics Co., Ltd.
