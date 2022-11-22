# ATF and u-boot for GL.iNet products based on mt798x

![](/u-boot.gif)

# Quickstart

## Prepare

	sudo apt install gcc-aarch64-linux-gnu
	export CROSS_COMPILE=aarch64-linux-gnu-

## Build u-boot

	make gl_mt3000_defconfig
	make

## Build ATF

	cp ../uboot-mtk-20220606/u-boot.bin .
	make gl_mt3000_defconfig
	make
