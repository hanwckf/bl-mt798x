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
	echo "eg: SOC=mt7981 BOARD=cmcc_rax3000m-emmc $0"
	echo "eg: SOC=mt7986 BOARD=redmi_ax6000 MULTI_LAYOUT=1 $0"
	echo "eg: SOC=mt7986 BOARD=jdcloud_re-cp-03 $0"
	exit 1
fi

# Check if Python is installed on the system
command -v python3
[ "$?" != "0" ] && { echo "Error: Python is not installed on this system."; exit 0; }

echo "Trying cross compiler..."
command -v "${TOOLCHAIN}gcc"
[ "$?" != "0" ] && { echo "${TOOLCHAIN}gcc not found!"; exit 0; }
export CROSS_COMPILE="$TOOLCHAIN"

ATF_CFG="${SOC}_${BOARD}_defconfig"
UBOOT_CFG="${SOC}_${BOARD}_defconfig"
for file in "$ATF_DIR/configs/$ATF_CFG" "$UBOOT_DIR/configs/$UBOOT_CFG"; do
	if [ ! -f "$file" ]; then
		echo "$file not found!"
		exit 1
	fi
done

if grep -q "CONFIG_FLASH_DEVICE_EMMC=y" $ATF_DIR/configs/$ATF_CFG ; then
	# No fixed-mtdparts or multilayout for EMMC
	fixedparts=0
	multilayout=0
else
	# Build fixed-mtdparts by default for NAND
	fixedparts=${FIXED_MTDPARTS:-1}
	multilayout=${MULTI_LAYOUT:-0}
	if [ "$multilayout" = "1" ]; then
		UBOOT_CFG="${SOC}_${BOARD}_multi_layout_defconfig"
	fi
fi
echo "Building for: ${SOC}_${BOARD}, fixed-mtdparts: $fixedparts, multi-layout: $multilayout"
echo "u-boot dir: $UBOOT_DIR"
echo "atf dir: $ATF_DIR"

echo "Build u-boot..."
rm -f "$UBOOT_DIR/u-boot.bin"
cp -f "$UBOOT_DIR/configs/$UBOOT_CFG" "$UBOOT_DIR/.config"
if [ "$fixedparts" = "1" ]; then
	echo "Build u-boot with fixed-mtdparts!"
	echo "CONFIG_MEDIATEK_UBI_FIXED_MTDPARTS=y" >> "$UBOOT_DIR/.config"
	echo "CONFIG_MTK_FIXED_MTD_MTDPARTS=y" >> "$UBOOT_DIR/.config"
fi
make -C "$UBOOT_DIR" olddefconfig
make -C "$UBOOT_DIR" -j $(nproc) all
if [ -f "$UBOOT_DIR/u-boot.bin" ]; then
	cp -f "$UBOOT_DIR/u-boot.bin" "$ATF_DIR/u-boot.bin"
	echo "u-boot build done!"
else
	echo "u-boot build fail!"
	exit 1
fi

echo "Build atf..."
make -C "$ATF_DIR" -f makefile "$ATF_CFG" CONFIG_CROSS_COMPILER="${TOOLCHAIN}"
make -C "$ATF_DIR" -f makefile clean CONFIG_CROSS_COMPILER="${TOOLCHAIN}"
rm -rf "$ATF_DIR/build"
make -C "$ATF_DIR" -f makefile all CONFIG_CROSS_COMPILER="${TOOLCHAIN}"

mkdir -p "output"
if [ -f "$ATF_DIR/build/${SOC}/release/fip.bin" ]; then
	FIP_NAME="${SOC}_${BOARD}-fip"
	if [ "$fixedparts" = "1" ]; then
		FIP_NAME="${FIP_NAME}-fixed-parts"
	fi
	if [ "$multilayout" = "1" ]; then
		FIP_NAME="${FIP_NAME}-multi-layout"
	fi
	cp -f "$ATF_DIR/build/${SOC}/release/fip.bin" "output/${FIP_NAME}.bin"
	echo "$FIP_NAME build done"
else
	echo "fip build fail!"
	exit 1
fi
if grep -q "CONFIG_TARGET_ALL_NO_SEC_BOOT=y" "$ATF_DIR/configs/$ATF_CFG"; then
	if [ -f "$ATF_DIR/build/${SOC}/release/bl2.img" ]; then
		BL2_NAME="${SOC}_${BOARD}-bl2"
		cp -f "$ATF_DIR/build/${SOC}/release/bl2.img" "output/${BL2_NAME}.bin"
		echo "$BL2_NAME build done"
	else
		echo "bl2 build fail!"
		exit 1
	fi
fi
