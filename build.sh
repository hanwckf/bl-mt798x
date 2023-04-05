#!/bin/sh

TOOLCHAIN=aarch64-linux-gnu-
UBOOT_DIR=uboot-mtk-20220606
ATF_DIR=atf-20220606-637ba581b

if [ -z "$SOC" ] || [ -z "$BOARD" ]; then
	echo "Usage: SOC=[mt7981|mt7986] BOARD=[xiaomi_wr30u|360t7|redmi_ax6000] FIXED_MTDPARTS=[1|0] $0"
	echo "eg: SOC=mt7981 BOARD=360t7 $0"
	echo "eg: SOC=mt7986 BOARD=redmi_ax6000 $0"
	exit 1
fi

echo "Trying cross compiler..."
command -v ${TOOLCHAIN}gcc
[ "$?" != "0" ] && { echo ${TOOLCHAIN}gcc not found!; exit 0; }
export CROSS_COMPILE=$TOOLCHAIN

# Build fixed-mtdparts by default
fixedparts=${FIXED_MTDPARTS:-1}

echo "Building for CPU: $SOC, BOARD: $BOARD, fixed-mtdparts: $fixedparts"

UBOOT_CFG="${SOC}_${BOARD}_defconfig"
if [ ! -f $UBOOT_DIR/configs/$UBOOT_CFG ]; then
	echo "$UBOOT_DIR/configs/$UBOOT_CFG not found!"
	exit 1
else
	echo "Build u-boot..."
	rm -f $UBOOT_DIR/u-boot.bin
	cp -f $UBOOT_DIR/configs/$UBOOT_CFG $UBOOT_DIR/.config
	if [ "$fixedparts" = "1" ]; then
		echo "Build u-boot with fixed-mtdparts!"
		echo "CONFIG_MEDIATEK_UBI_FIXED_MTDPARTS=y" >> $UBOOT_DIR/.config
	fi
	make -C $UBOOT_DIR olddefconfig all
	if [ -f "$UBOOT_DIR/u-boot.bin" ]; then
		cp -f $UBOOT_DIR/u-boot.bin $ATF_DIR/u-boot.bin
		echo "u-boot build done!"
	else
		echo "u-boot build fail!"
		exit 1
	fi
fi

ATF_CFG="${SOC}_${BOARD}_defconfig"
if [ ! -f $ATF_DIR/configs/$ATF_CFG ]; then
	echo "$ATF_DIR/configs/$ATF_CFG not found!"
	exit 1
else
	echo "Build atf..."
	make -C $ATF_DIR -f makefile $ATF_CFG CONFIG_CROSS_COMPILER=${TOOLCHAIN}
	make -C $ATF_DIR -f makefile clean CONFIG_CROSS_COMPILER=${TOOLCHAIN}
	rm -rf $ATF_DIR/build
	make -C $ATF_DIR -f makefile all CONFIG_CROSS_COMPILER=${TOOLCHAIN}
	if [ -f "$ATF_DIR/build/${SOC}/release/fip.bin" ]; then
		mkdir -p output
		if [ "$fixedparts" = "1" ]; then
			FIP_NAME="${SOC}_${BOARD}-fip-fixed-parts.bin"
		else
			FIP_NAME="${SOC}_${BOARD}-fip.bin"
		fi
		cp -f $ATF_DIR/build/${SOC}/release/fip.bin output/${FIP_NAME}
		echo "$FIP_NAME build done"
	else
		echo "fip build fail!"
		exit 1
	fi
fi
