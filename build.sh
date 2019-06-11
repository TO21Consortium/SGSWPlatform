#!/bin/bash

if [ "z${CPU_JOB_NUM}" == "z" ] ; then
	CPU_JOB_NUM=$(grep processor /proc/cpuinfo | awk '{field=$NF};END{print (field+1)/2}')
fi
CLIENT=$(whoami)

ROOT_DIR=$(pwd)

if [ $# -lt 1 ]
then
	echo "Usage: ./build.sh <PRODUCT> [ kernel | platform | all ] [ eng | user ]"
	exit 0
fi

if [ ! -f device/samsung/$1/build-info.sh ]
then
	echo "NO PRODUCT to build!!"
	exit 0
fi

source device/samsung/$1/build-info.sh
BUILD_OPTION=$2
BUILD_MODE=$3

if [ -z $BUILD_OPTION ]
then
	BUILD_OPTION=platform
fi

if [ -z $BUILD_MODE ]
then
	BUILD_MODE=eng
fi

OUT_DIR="$ROOT_DIR/out/target/product/$PRODUCT_BOARD"
OUT_HOSTBIN_DIR="$ROOT_DIR/vendor/samsung_slsi/script"
KERNEL_CROSS_COMPILE_PATH="$ROOT_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-"

function check_exit()
{
	if [ $? != 0 ]
	then
		exit $?
	fi
}

function build_kernel()
{
	echo
	echo '[[[[[[[ Build android kernel ]]]]]]]'
	echo

	START_TIME=`date +%s`
	pushd $KERNEL_DIR
	echo "set defconfig for $PRODUCT_BOARD $BUILD_MODE mode"
	echo
	ARCH=arm ./scripts/kconfig/merge_config.sh arch/arm/configs/"$PRODUCT_BOARD"_defconfig arch/arm/configs/exynos_"$BUILD_MODE"_defconfig
	check_exit
	echo "make"
	echo
	make -j$CPU_JOB_NUM > /dev/null ARCH=arm CROSS_COMPILE=$KERNEL_CROSS_COMPILE_PATH
	check_exit
	END_TIME=`date +%s`

	let "ELAPSED_TIME=$END_TIME-$START_TIME"
	echo "Total compile time is $ELAPSED_TIME seconds"

	popd
}

function build_android()
{
        echo
        echo '[[[[[[[ Build android platform ]]]]]]]'
        echo

        START_TIME=`date +%s`
        echo "source build/envsetup.sh"
        source build/envsetup.sh
        echo
        echo "lunch full_$PRODUCT_BOARD-$BUILD_MODE"
        lunch full_$PRODUCT_BOARD-$BUILD_MODE
        echo
        echo "make -j$CPU_JOB_NUM"
        echo
        make -j$CPU_JOB_NUM
        check_exit

        END_TIME=`date +%s`
        let "ELAPSED_TIME=$END_TIME-$START_TIME"
        echo "Total compile time is $ELAPSED_TIME seconds"
}

function make_uboot_img()
{
	pushd $OUT_DIR

	echo
	echo '[[[[[[[ Make ramdisk image for u-boot ]]]]]]]'
	echo

	mkimage -A arm -O linux -T ramdisk -C none -a 0x40800000 -n "ramdisk" -d ramdisk.img ramdisk-uboot.img
	check_exit

	rm -f ramdisk.img

	echo
	popd
}

function make_fastboot_img()
{
	echo
	echo '[[[[[[[ Make additional images for fastboot ]]]]]]]'
	echo

	if [ ! -f $KERNEL_DIR/arch/arm/boot/zImage ]
	then
		echo "No zImage is found at $KERNEL_DIR/arch/arm/boot"
		echo
		return
	fi

	echo 'boot.img ->' $OUT_DIR
	cp $KERNEL_DIR/arch/arm/boot/zImage $OUT_DIR/zImage
	$OUT_HOSTBIN_DIR/mkbootimg --kernel $OUT_DIR/zImage --ramdisk $OUT_DIR/ramdisk.img -o $OUT_DIR/boot.img
	check_exit

	echo 'update.zip ->' $OUT_DIR
	zip -j $OUT_DIR/update.zip $OUT_DIR/android-info.txt $OUT_DIR/boot.img $OUT_DIR/system.img
	check_exit

	echo
}

echo
echo '                Build android for '$PRODUCT_BOARD''
echo

case "$BUILD_OPTION" in
	kernel)
		build_kernel
		;;
	platform)
		build_android
		make_fastboot_img
		;;
	all)
		build_kernel
		build_android
		make_fastboot_img
		;;
	eng | user)
		BUILD_MODE=$2
		build_kernel
		build_android
		make_fastboot_img
		;;
	*)
		build_android
		make_fastboot_img
		;;
esac

echo ok success !!!

exit 0
