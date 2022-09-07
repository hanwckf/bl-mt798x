#!/bin/bash
# Copyright (C) 2021-2022 SkyLake Huang
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Use this script to compose single image.
# Author: SkyLake Huang
# Email: skylake.huang@mediatek.com

#global variables:
bl2_start_dec=0
bl2_start_hex=0x0
rf_start_dec=0
rf_start_hex=0x0
fip_start_dec=0
fip_start_hex=0x0
kernel_start_dec=0
kernel_start_hex=0x0
rootfs_start_dec=0
rootfs_start_hex=0x0
rootfs_image=""

# Regular Colors
RED='\033[0;31m'
NC='\033[0m'

usage() {
	printf 'Usage:\n'\
'  ./mk_image -p <platform>\n'\
'             -d <flash device type>\n'\
'             -c <partition config>\n'\
'             -b <BL2 image>, default=bl2.img\n'\
'             -r <RF image>\n'\
'             -f <FIP image>, default=fip.bin\n'\
'             -k <kernel image>\n'\
'             -g <GPT table>\n'\
'             -h <usage menu>\n'\
'             -o <single image name>\n'\
'  example:\n'\
'    ./mk_image.sh -p mt7986a -d emmc \\\n'\
'                  -g GPT_EMMC-iap-20220125 \\\n'\
'                  -f fip-iap-emmc-20220125.bin \\\n'\
'                  -k OF_openwrt-mediatek-mt7986-mt7986a-ax6000-emmc-rfb-squashfs-sysupgrade.bin\n'\
'    ./mk_image.sh -p mt7986a -d spim-nand \\\n'\
'                  -b bl2-iap-snand-20220114.img \\\n'\
'                  -f fip-snand-20220114.bin \\\n'\
'                  -k OF_openwrt-mediatek-mt7986-mt7986a-ax6000-spim-nand-rfb-squashfs-factory.bin \\\n'
	exit
}

parse_yaml() {
	local prefix=$2
	local s='[[:space:]]*' w='[a-zA-Z0-9_]*' fs=$(echo @|tr @ '\034')
	sed -ne "s|^\($s\):|\1|" \
		-e "s|^\($s\)\($w\)$s:$s[\"']\(.*\)[\"']$s\$|\1$fs\2$fs\3|p" \
		-e "s|^\($s\)\($w\)$s:$s\(.*\)$s\$|\1$fs\2$fs\3|p"  $1 |
	awk -F$fs '{
		indent = length($1)/2;
		vname[indent] = $2;
		for (i in vname) {if (i > indent) {delete vname[i]}}
		if (length($3) > 0) {
			vn=""; for (i=0; i<indent; i++) {vn=(vn)(vname[i])("_")}
			printf("%s=%s\n", $2, $3);
		}
	}'
}

load_partition() {
	IFS=$'\r\n'
	for line in `parse_yaml $partition_config`; do
		IFS='=' read -ra arr <<< "$line"
		case "${arr[0]}" in
			bl2_start )
				bl2_start_hex=${arr[1]}
				bl2_start_dec=`printf '%d' $bl2_start_hex`
				;;
			rf_start )
				rf_start_hex=${arr[1]}
				rf_start_dec=`printf '%d' $rf_start_hex`
				;;
			fip_start )
				fip_start_hex=${arr[1]}
				fip_start_dec=`printf '%d' $fip_start_hex`
				;;
			kernel_start )
				kernel_start_hex=${arr[1]}
				kernel_start_dec=`printf '%d' $kernel_start_hex`
				;;
			rootfs_start )
				rootfs_start_hex=${arr[1]}
				rootfs_start_dec=`printf '%d' $rootfs_start_hex`
		esac
	done
}

prepare_image() {
	# Pad empty bytes to single image first
	if [[ -z $kernel_image ]]
	then
		if [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
		then
			dd if=/dev/zero ibs=$fip_start_dec count=1 status=none \
				> $single_image
		else
			dd if=/dev/zero ibs=$fip_start_dec count=1 status=none \
				| tr "\000" "\377" > $single_image
		fi
	else
		if [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
		then
			dd if=/dev/zero ibs=$kernel_start_dec count=1 status=none \
				> $single_image 2>&1
		else
			dd if=/dev/zero ibs=$kernel_start_dec count=1 status=none \
				| tr "\000" "\377" > $single_image 2>&1
		fi
	fi
}

extract_sdmmc_kernel() {
	output=`tar -xvf $kernel_image | awk {'print $1'}`
	IFS=$'\n' read -d "\034" -r -a output_arr <<< "$output"

	#For debugging
	#echo "There are ${#output_arr[*]}" next lines in the output.

	for filename in "${output_arr[@]}";
	do
		if [[ "$filename" == *"kernel" ]]
		then
			kernel_image=$filename
		elif [[ "$filename" == *"root" ]]
		then
			rootfs_image=$filename
		fi
	done
}

start_wrapping() {
	printf "[Start wrapping %s single image......]\n" $flash_type

	if [[ $flash_type != "emmc" ]]
	then
		printf "[wrapping BL2 image......]\n"
		dd if=$bl2_image of=$single_image bs=512 \
			seek=$(( ($bl2_start_dec/512) )) conv=notrunc status=none
	fi

	if [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
	then
		printf "[wrapping GPT......]\n"
		dd if=$gpt of=$single_image bs=512 seek=0 conv=notrunc status=none
	fi

	if [[ -n $rf_image ]]
	then
		printf "[wrapping RF image......]\n"
		dd if=$rf_image of=$single_image bs=512 \
			seek=$(( ($rf_start_dec/512) )) conv=notrunc status=none
	fi

	printf "[wrapping FIP image......]\n"
	dd if=$fip_image of=$single_image bs=512 \
		seek=$(( ($fip_start_dec/512) )) conv=notrunc status=none

	if [[ -n $kernel_image ]]
	then
		printf "[wrapping kernel image......]\n"
		if [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
		then
			extract_sdmmc_kernel
			dd if=$kernel_image of=$single_image bs=512 \
				seek=$(( ($kernel_start_dec/512) )) conv=notrunc status=none
			printf "[wrapping rootfs image......]\n"
			dd if=$rootfs_image of=$single_image bs=512 \
				seek=$(( ($rootfs_start_dec/512) )) conv=notrunc status=none
		else
			dd if=$kernel_image of=$single_image bs=512 \
				seek=$(( ($kernel_start_dec/512) )) conv=notrunc status=none
		fi
	fi
}

if [ $# -lt 1 ]
then
	usage
	exit 1
fi

## We set default values for some arguments
bl2_image="bl2.img"
bl2_default=1
fip_image="fip.bin"
fip_default=1
partition_config=""
partition_config_default=1

while [ "$1" != "" ]; do
	case $1 in
	-h )
		usage
		;;
	-p )
		shift
		platform=$1
		;;
	-d )
		shift
		flash_type=$1
		;;
	-c )
		shift
		partition_config=$1
		partition_config_default=0
		;;
	-b )
		shift
		bl2_image=$1
		bl2_default=0
		;;
	-f )
		shift
		fip_image=$1
		fip_default=0
		;;
	-k )
		shift
		kernel_image=$1
		;;
	-g )
		shift
		gpt=$1
		;;
	-o )
		shift
		single_image=$1
		;;
	-r )
		shift
		rf_image=$1
		;;
	esac
	shift
done


######## Check if variables are valid ########
check_ok=1
if ! [[ $platform =~ ^(mt7981abd|mt7981c|mt7986a|mt7986b)$ ]]; then
	printf "${RED}Platform must be in mt7981abd|mt7981c|mt7986a|mt7986b\n${NC}"
	usage
	exit 1
fi
if ! [[ $flash_type =~ ^(snfi-nand|spim-nand|spim-nor|emmc|sd)$ ]]; then
	printf "${RED}Flash type must be in snfi-nand|spim-nand|spim-nor|emmc|sd\n${NC}"
	usage
	exit 1
fi

if [[ $partition_config_default -eq 1 ]]; then
	partition_config="partitions/${flash_type}-default.yml"
fi

if [[ $flash_type =~ ^(emmc|sd)$ ]] && [[ -z $gpt ]]; then
	printf "${RED}GPT table must be provided if flash type is emmc or sd\n${NC}"
	usage
	exit 1
fi

if [[ -n $gpt ]] && ! [[ -f $gpt ]]; then
	printf "${RED}GPT table provided doesn't exist.\n${NC}"
	exit 1
fi
if [[ -n $rf_image ]] && ! [[ -f $rf_image ]]; then
	printf "${RED}RF image provided doesn't exist.\n${NC}"
	exit 1
fi
if [[ -n $kernel_image ]] && ! [[ -f $kernel_image ]]; then
	printf "${RED}Kernel image provided doesn't exist.\n${NC}"
	exit 1
fi

##############################################
if ! [[ -f $partition_config ]]
then
	if [[ $partition_config_default -eq 1 ]]
	then
		printf "${RED}Default partition config${NC}"
	else
		printf "${RED}Partition config provided${NC}"
	fi
	printf "${RED} doesn't exist: %s\n${NC}" $partition_config
	exit 1
fi
printf "* Partition config: %s\n" $partition_config

if ! [[ -f $bl2_image ]]
then
	if [[ $bl2_default -eq 1 ]]
	then
		printf "${RED}Default BL2 image${NC}"
	else
		printf "${RED}BL2 image provided${NC}"
	fi
	printf "${RED} doesn't exist: %s\n${NC}" $bl2_image
	exit 1
fi
printf "* BL2 image name: %s\n" $bl2_image

if ! [[ -f $fip_image ]]
then
	if [[ $fip_default -eq 1 ]]
	then
		printf "${RED}Default FIP image"
	else
		printf "${RED}FIP image provided"
	fi
	printf "${RED} doesn't exist: %s\n${NC}" $fip_image
	exit 1
fi
printf "* FIP image name: %s\n" $fip_image

if [[ -z $single_image ]]
then
	single_image="$platform-$flash_type-$(date +%Y%m%d)-single-image.bin"
	printf "* Single image name: %s\n" $single_image
fi

if [[ $check_ok == 1 ]]; then
	#printf "./mk_image -p %s -d %s\n" $platform $flash_type
	load_partition
	prepare_image
	start_wrapping
fi
