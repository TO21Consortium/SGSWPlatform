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

ifeq ($(TARGET_PREBUILT_KERNEL),)
LOCAL_KERNEL := device/samsung/smdk7570/kernel
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

include $(LOCAL_PATH)/BoardConfig.mk
# These are for the multi-storage mount.
DEVICE_PACKAGE_OVERLAYS := \
	device/samsung/smdk7570/overlay

# Init files
PRODUCT_COPY_FILES += \
	device/samsung/smdk7570/conf/init.smdk7570.rc:root/init.samsungexynos7570.rc \
	device/samsung/smdk7570/conf/init.smdk7570.usb.rc:root/init.samsungexynos7570.usb.rc \
	device/samsung/smdk7570/conf/fstab.smdk7570:root/fstab.samsungexynos7570

PRODUCT_COPY_FILES += \
	device/samsung/smdk7570/conf/ueventd.smdk7570.rc:root/ueventd.samsungexynos7570.rc

# Filesystem management tools
PRODUCT_PACKAGES += \
	e2fsck

# Audio
PRODUCT_PACKAGES += \
	audio.primary.smdk7570 \
	audio.a2dp.default \
	audio.usb.default \
	audio.r_submix.default \

PRODUCT_COPY_FILES += \
	device/samsung/smdk7570/audio_policy.conf:system/etc/audio_policy.conf \
	device/samsung/smdk7570/mixer_paths.xml:system/etc/mixer_paths.xml


# Libs
PRODUCT_PACKAGES += \
	com.android.future.usb.accessory

# for now include gralloc here. should come from hardware/samsung_slsi/exynos5
PRODUCT_PACKAGES += \
	gralloc.exynos5

PRODUCT_PACKAGES += \
	libion

PRODUCT_PACKAGES += \
	libstlport

# RPMB
#PRODUCT_PACKAGES += \
       rpmbd \
       tlrpmb

# Power HAL
PRODUCT_PACKAGES += \
	power.smdk7570

# MobiCore setup
#PRODUCT_PACKAGES += \
	libMcClient \
	libMcRegistry \
	libgdmcprov \
	mcDriverDaemon

# WideVine DASH modules
#PRODUCT_PACKAGES += \
	libwvdrmengine

#Gatekeeper
#PRODUCT_PACKAGES += \
       gatekeeper.exynos7570

# WideVine modules
#PRODUCT_PACKAGES += \
	com.google.widevine.software.drm.xml \
	com.google.widevine.software.drm \
	WidevineSamplePlayer \
	libdrmwvmplugin \
	libwvm \
	libWVStreamControlAPI_L1 \
	libwvdrm_L1

# SecureDRM modules
PRODUCT_PACKAGES += \
	secdrv \
#	tlwvdrm \
	tlsecdrm \
	liboemcrypto_modular

# KeyManager/AES modules
PRODUCT_PACKAGES += \
	tlkeyman

# Keymaster
#PRODUCT_PACKAGES += \
	keystore.exynos7570 \
	tlkeymasterM

PRODUCT_PACKAGES += \
	camera.smdk7570

# OpenMAX IL configuration files
PRODUCT_COPY_FILES += \
	frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
	device/samsung/smdk7570/media_profiles.xml:system/etc/media_profiles.xml \
	device/samsung/smdk7570/media_codecs_performance.xml:system/etc/media_codecs_performance.xml \
	device/samsung/smdk7570/media_codecs.xml:system/etc/media_codecs.xml

PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
	frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \

# Copy FIMC-IS-FW
PRODUCT_COPY_FILES += \
       device/samsung/smdk7570/firmware/fimc_is_lib.bin:system/vendor/firmware/fimc_is_lib.bin \
       device/samsung/smdk7570/firmware/setfile_3m2.bin:system/vendor/firmware/setfile_3m2.bin \
       device/samsung/smdk7570/firmware/setfile_5e2.bin:system/vendor/firmware/setfile_5e2.bin \

# Camera configuration files
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
	frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \

# WLAN configuration
# device specific wpa_supplicant.conf
PRODUCT_COPY_FILES += \
        device/samsung/smdk7570/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf

# Bluetooth configuration
PRODUCT_COPY_FILES += \
       frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
       frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
       hardware/samsung_slsi/libbt/conf/bt_vendor.conf:system/etc/bluetooth/bt_vendor.conf \
       hardware/samsung_slsi/libbt/conf/bt_did.conf:system/etc/bluetooth/bt_did.conf

PRODUCT_PROPERTY_OVERRIDES := \
	ro.opengles.version=196609 \
	ro.sf.lcd_density=320 \
	debug.hwc.otf=1 \
	debug.hwc.winupdate=1 \
	debug.hwc.nodirtyregion=1

PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0

# WideVine DRM setup
#PRODUCT_PROPERTY_OVERRIDES += \
	drm.service.enabled = true

# CP Booting daemon
#PRODUCT_PACKAGES += \
	cbd

# Packages needed for WLAN
PRODUCT_PACKAGES += \
    dhcpcd.conf \
    hostapd \
    wpa_supplicant.conf \
    wpa_supplicant

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mtp

PRODUCT_CHARACTERISTICS := phone

PRODUCT_AAPT_CONFIG := normal hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# setup dalvik vm configs.
$(call inherit-product, frameworks/native/build/phone-xhdpi-1024-dalvik-heap.mk)

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_COPY_FILES += \
       device/samsung/smdk7570/conf/init.rc:root/init.rc \

MODEM_USE_ONECHIP := true
MODEM_USE_GPT := true
#MODEM_USE_UFS := true
#MODEM_NOT_USE_FINAL_CMD := true

$(call inherit-product, hardware/samsung_slsi/exynos5/exynos5.mk)
$(call inherit-product-if-exists, vendor/samsung_slsi/exynos7570/exynos7570-vendor.mk)
$(call inherit-product, hardware/samsung_slsi/exynos7570/exynos7570.mk)
#$(call inherit-product, device/samsung/smdk7570/gnss_binaries/gnss_binaries.mk)
