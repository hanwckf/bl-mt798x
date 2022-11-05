#!/bin/sh

TOOLCHAIN=aarch64-linux-gnu-
command -v ${TOOLCHAIN}gcc
[ "$?" != "0" ] && { echo ${TOOLCHAIN}gcc not found!; exit 0; }

export CROSS_COMPILE=$TOOLCHAIN

BOARD=360t7

UBOOT_DIR=uboot-mtk-20220606
UBOOT_CFG=mt7981_360t7_defconfig

ATF_DIR=atf-20220606-637ba581b
ATF_CFG=mt7981_360t7_defconfig

echo "build u-boot..."

cp -f $UBOOT_DIR/configs/$UBOOT_CFG $UBOOT_DIR/.config
if [ "$1" = "fixedparts" ]; then
	echo "CONFIG_MEDIATEK_UBI_FIXED_MTDPARTS=y" >> $UBOOT_DIR/.config
	echo "Build uboot with fixed-mtdparts!"
fi

make -C $UBOOT_DIR olddefconfig all
if [ -f "$UBOOT_DIR/u-boot.bin" ]; then
	cp -f $UBOOT_DIR/u-boot.bin $ATF_DIR/u-boot.bin
	echo "u-boot.bin done"
else
	echo "u-boot.bin fail!"
	exit 1
fi

echo "build atf..."
make -C $ATF_DIR -f makefile $ATF_CFG all CONFIG_CROSS_COMPILER=${TOOLCHAIN}
if [ -f "$ATF_DIR/build/mt7981/release/fip.bin" ]; then
	cp -f $ATF_DIR/build/mt7981/release/fip.bin $BOARD-fip.bin
	echo "fip.bin done"
else
	echo "fip build fail!"
	exit 1
fi
