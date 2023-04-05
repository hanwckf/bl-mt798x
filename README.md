# ATF and u-boot for GL.iNet products based on mt798x

![](/u-boot.gif)

# Build
```
Usage: SOC=[mt7981|mt7986] BOARD=[xiaomi_wr30u|360t7|redmi_ax6000] FIXED_MTDPARTS=[1|0] ./build.sh
eg: SOC=mt7981 BOARD=360t7 ./build.sh
eg: SOC=mt7986 BOARD=redmi_ax6000 ./build.sh
```

# Quickstart

## Prepare

	sudo apt install build-essential bison flex bc libssl-dev python-is-python3 device-tree-compiler gcc-aarch64-linux-gnu
	export CROSS_COMPILE=aarch64-linux-gnu-

## Build u-boot

	make gl_mt3000_defconfig
	make

## Build ATF

	cp ../uboot-mtk-20220606/u-boot.bin .
	make gl_mt3000_defconfig
	make
