#!/bin/bash

IMAGE_DIRS=${ANDROID_PRODUCT_OUT}

# Need to provide these two variables
#   The kernel to boot, and the dtb to use
#
# KERNEL_PATH=${HOME}/src-lcl/android-kernel/kernel/arch/arm/boot/zImage
# DTB_PATH=${HOME}/src-lcl/android-kernel/kernel/arch/arm/boot/dts/vexpress-v2p-ca15-tc1.dtb

[ -z "${KERNEL_PATH}" ] && exit 1;
[ -z "${DTB_PATH}" ] && exit 1;

arm-softmmu/qemu-system-arm \
	-serial mon:stdio \
	-s \
	-nodefaults \
	-cpu cortex-a15 \
	-machine vexpress-a15 \
	-smp 1 \
	-m 512 \
	-kernel ${KERNEL_PATH} \
	-dtb ${DTB_PATH} \
	-append "console=ttyAMA0,115200 androidboot.hardware=lionhead androidboot.serialno=123 debug user_debug=31" \
	-initrd ${ANDROID_PRODUCT_OUT}/ramdisk.img \
	-device virtio-scsi-device,id=scsi \
	-device scsi-hd,drive=system \
	-drive file=${ANDROID_PRODUCT_OUT}/system.img,if=none,id=system,format=raw \
	-device scsi-hd,drive=userdata \
	-drive file=${ANDROID_PRODUCT_OUT}/userdata.img,if=none,id=userdata,format=raw \
	-device scsi-hd,drive=cache \
	-drive file=${ANDROID_PRODUCT_OUT}/cache.img,if=none,id=cache,format=raw \
	$*
