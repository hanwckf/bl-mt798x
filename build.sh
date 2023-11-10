#!/bin/sh

TOOLCHAIN=aarch64-linux-gnu-
#UBOOT_DIR=uboot-mtk-20220606
UBOOT_DIR=uboot-mtk-20230718-09eda825
#ATF_DIR=atf-20220606-637ba581b
ATF_DIR=atf-20231013-0ea67d76a

if [ -z "$SOC" ] || [ -z "$BOARD" ]; then
	echo "Usage: SOC=[mt7981|mt7986] BOARD=<board name> MULTI_LAYOUT=[0|1] $0"
	echo "eg: SOC=mt7981 BOARD=360t7 $0"
	echo "eg: SOC=mt7981 BOARD=wr30u MULTI_LAYOUT=1 $0"
	echo "eg: SOC=mt7986 BOARD=redmi_ax6000 $0"
	exit 1
fi

# Check if Python is installed on the system
command -v python3
[ "$?" != "0" ] && { echo "Error: Python is not installed on this system."; exit 0; }

echo "Trying cross compiler..."
command -v ${TOOLCHAIN}gcc
[ "$?" != "0" ] && { echo ${TOOLCHAIN}gcc not found!; exit 0; }
export CROSS_COMPILE=$TOOLCHAIN

# Build fixed-mtdparts by default
fixedparts=${FIXED_MTDPARTS:-1}
multilayout=${MULTI_LAYOUT:-0}

echo "Building for: ${SOC}_${BOARD}, fixed-mtdparts: $fixedparts, multi-layout: $multilayout"
echo "u-boot dir: $UBOOT_DIR"

UBOOT_CFG="${SOC}_${BOARD}_defconfig"
if [ "$multilayout" = "1" ]; then
	UBOOT_CFG="${SOC}_${BOARD}_multi_layout_defconfig"
fi

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
		echo "CONFIG_MTK_FIXED_MTD_MTDPARTS=y" >> $UBOOT_DIR/.config
	fi
	make -C $UBOOT_DIR olddefconfig
	make -C $UBOOT_DIR -j $(nproc) all
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
		FIP_NAME="${SOC}_${BOARD}-fip"
		if [ "$fixedparts" = "1" ]; then
			FIP_NAME="${FIP_NAME}-fixed-parts"
		fi
		if [ "$multilayout" = "1" ]; then
			FIP_NAME="${FIP_NAME}-multi-layout"
		fi
		cp -f $ATF_DIR/build/${SOC}/release/fip.bin output/${FIP_NAME}.bin
		echo "$FIP_NAME build done"
	else
		echo "fip build fail!"
		exit 1
	fi
fi
