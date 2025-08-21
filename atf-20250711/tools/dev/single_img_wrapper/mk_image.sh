#!/bin/bash

# pip install: ubi_reader, yq

#global variables:
gpt_start_dec=0
gpt_start_hex=0
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
sdmmc_extracted_folder=""

ws_folder="./tmp"

## We set default values for some arguments
bl2_image="bl2.img"
bl2_default=1
fip_image="fip.bin"
fip_default=1
partition_config=""
partition_config_default=1
dual_image_mode=0
new_ubi_image="new_ubi.bin"
part_type="default"
yq="yq_linux_amd64"

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
'             -u <Enable ubi image>\n'\
'             -uk <ubi kernel image>\n'\
'             -ur <ubi rootfs image>\n'\
'             -uo <ubi output image name\n>'\
'  example:\n'\
'[eMMC single image]\n'\
'    ./mk_image.sh -p mt7986 -d emmc \\\n'\
'                  -g GPT_EMMC-iap-20220125 \\\n'\
'                  -f fip-iap-emmc-20220125.bin \\\n'\
'                  -k OF_openwrt-mediatek-mt7986-mt7986a-ax6000-emmc-rfb-squashfs-sysupgrade.bin\n'\
'[SPIM-NAND single image]\n'\
'    ./mk_image.sh -p mt7986 -d spim-nand \\\n'\
'                  -b bl2-iap-snand-20220114.img \\\n'\
'                  -f fip-snand-20220114.bin \\\n'\
'                  -k OF_openwrt-mediatek-mt7986-mt7986a-ax6000-spim-nand-rfb-squashfs-factory.bin\n'\
'[SPIM-NAND dual image mode]\n'\
'    ./mk_image.sh -p mt7986 -d spim-nand \\\n'\
'                  -b bl2.img -f fip.bin \\\n'\
'                  -k OF_openwrt-mediatek-mt7986-mt7986a-ax6000-spim-nand-rfb-squashfs-factory.bin \\\n'\
'                  --dual-image\n'
	exit 0
}

## Enter current folder
DIR="$(cd "$(dirname "$0")" && pwd)"
cd ${DIR}
export PATH="$PATH:${DIR}/bin"

cmd_check_ok=1
commands=(
"awk" "tar" "sed" "dd"
${yq}
"ubireader_extract_images"
"ubireader_display_info"
"ubinize"
)
install=(
"check"
"check"
"check"
"check"
"download yq binary from https://github.com/mikefarah/yq/releases"
"install via python3 -m pip install ubi_reader"
"install via python3 -m pip install ubi_reader"
"install via apt install mtd-utils"
)
for ((i=0; i<${#commands[@]}; i++)); do
	if command -v ${commands[i]} >/dev/null 2>&1; then
		:
		#echo "- ${commands[i]} command is supported"
	else
		echo "- ${commands[i]} command is not supported, please ${install[i]}."
		cmd_check_ok=0
	fi
done

if [ ${cmd_check_ok} -eq 0 ]; then
	exit 0
fi

# If execute ./mk_image.sh only, enter this case
if [ $# -lt 1 ]
then
	usage
	exit 0
fi

while [ "$1" != "" ]; do
	case $1 in
	-h )
		usage
		;;
	-p )
		shift
		platform=$1
		;;
	-d | --device)
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
	-i | --dual_image)
		dual_image_mode=1
		part_type="dual-image"
		;;
	esac
	shift
done

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
	gpt_start_hex=$(${yq} '."'${platform}'"."'${part_type}'".gpt.start' "${partition_config}")
	if [ -v $gpt_start_hex ]; then
		gpt_start_dec=`printf '%d' $gpt_start_hex`
		echo "GPT start: ${gpt_start_hex} ${gpt_start_dec}"
	fi

	bl2_start_hex=$(${yq} '."'${platform}'"."'${part_type}'".bl2.start' "${partition_config}")
	if [ "${bl2_start_hex}" != "null" ]; then
		bl2_start_dec=`printf '%d' $bl2_start_hex`
		echo "BL2 start: ${bl2_start_hex} ${bl2_start_dec}"
	fi

	rf_start_hex=$(${yq} '."'${platform}'"."'${part_type}'".factory.start' "${partition_config}")
	if [ "${rf_start_hex}" != "null" ]; then
		rf_start_dec=`printf '%d' $rf_start_hex`
		echo "RF start: ${rf_start_hex} ${rf_start_dec}"
	fi

	fip_start_hex=$(${yq} '."'${platform}'"."'${part_type}'".fip.start' "${partition_config}")
	if [ "${fip_start_hex}" != "null" ]; then
		fip_start_dec=`printf '%d' $fip_start_hex`
		echo "FIP start: ${fip_start_hex} ${fip_start_dec}"
	fi

	if [[ ${flash_type} != "spim-nand" ]] && [[ ${flash_type} != "snfi-nand" ]]
	then
		kernel_start_hex=$(${yq} e '."'${platform}'"."'${part_type}'".kernel.start' "${partition_config}")
		kernel_size_hex=$(${yq} e '."'${platform}'"."'${part_type}'".kernel.size' "${partition_config}")
		kernel2_size_hex=$(${yq} e '."'${platform}'"."'${part_type}'".kernel2.size' "${partition_config}")
	else
		kernel_start_hex=$(${yq} e '."'${platform}'"."'${part_type}'".ubi.kernel.start' "${partition_config}")
	fi
	if [ "${kernel_start_hex}" != "null" ]; then
		kernel_start_dec=`printf '%d' $kernel_start_hex`
		echo "Kernel start: ${kernel_start_hex} ${kernel_start_dec}"
	fi
	if [ "${kernel_size_hex}" != "null" ]; then
		kernel_size_dec=`printf '%d' $kernel_size_hex`
		echo "Kernel size: ${kernel_size_hex} ${kernel_size_dec}"
	fi
	if [ "${kernel2_size_hex}" != "null" ]; then
		kernel2_size_dec=`printf '%d' $kernel2_size_hex`
		echo "Kernel2 size: ${kernel2_size_hex} ${kernel2_size_dec}"
	fi

	if [[ ${flash_type} != "spim-nand" ]] && [[ ${flash_type} != "snfi-nand" ]]
	then
		rootfs_start_hex=$(${yq} e '."'${platform}'"."'${part_type}'".rootfs.start' "${partition_config}")
		rootfs_size_hex=$(${yq} e '."'${platform}'"."'${part_type}'".rootfs.size' "${partition_config}")
	else
		rootfs_start_hex=$(${yq} e '."'${platform}'"."'${part_type}'".ubi.rootfs.start' "${partition_config}")
	fi
	#rootfs_start_hex=$(${yq} '."'${platform}'"."'${part_type}'".*.rootfs.start' "${partition_config}" | grep -v 'null')
	if [ "${rootfs_start_hex}" != "null" ]; then
		rootfs_start_dec=`printf '%d' $rootfs_start_hex`
		echo "Rootfs start: ${rootfs_start_hex} ${rootfs_start_dec}"
	fi
	if [ "${rootfs_size_hex}" != "null" ]; then
		rootfs_size_dec=`printf '%d' $rootfs_size_hex`
		echo "Rootfs size: ${rootfs_size_hex} ${rootfs_size_dec}"
	fi
}

create_workspace() {
	mkdir -p ${ws_folder}
}

pad_output_image() {
	# Pad empty bytes to single image first
	if [[ -z $kernel_image ]]
	then
		if [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
		then
			dd if=/dev/zero ibs=$fip_start_dec count=1 \
				> $single_image
		else
			dd if=/dev/zero ibs=$fip_start_dec count=1 \
				| tr "\000" "\377" > $single_image
		fi
	else
		if [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
		then
			dd if=/dev/zero ibs=$kernel_start_dec count=1 \
				> $single_image 2>&1
		else
			dd if=/dev/zero ibs=$kernel_start_dec count=1 \
				| tr "\000" "\377" > $single_image 2>&1
		fi
	fi
}

extract_sdmmc_kernel() {
	output=`tar -xvf $kernel_image -C ${ws_folder}/ | awk {'print $1'}`
	IFS=$'\n' read -d "\034" -r -a output_arr <<< "$output"

	#For debugging
	#echo "There are ${#output_arr[*]}" next lines in the output.

	sdmmc_extracted_folder="${ws_folder}/${output_arr[0]}"
	echo ${sdmmc_extracted_folder}

	for filename in "${output_arr[@]}";
	do
		filename="${ws_folder}/${filename}"
		if [[ "$filename" == *"kernel" ]]
		then
			kernel_image=$filename
		elif [[ "$filename" == *"root" ]]
		then
			rootfs_image=$filename
		fi
	done
}

recreate_ubimage() {
	ubinize_config="ubi_temp.cfg"

	ubireader_extract_images -v ${kernel_image} -o ${ws_folder}/ > /dev/null 2>&1
	# Remove tail 0xff paddings
	sed -i '$ s/\xff*$//' ${ws_folder}/${kernel_image}/*kernel.ubifs
	sed -i '$ s/\xff*$//' ${ws_folder}/${kernel_image}/*rootfs.ubifs

	peb_size=`ubireader_display_info ${kernel_image} | grep "PEB Size: " | awk '{print $3}'`
	min_io_size=`ubireader_display_info ${kernel_image} | grep "Min I/O: " | awk '{print $3}'`
	ec=`ubireader_display_info ${kernel_image} | grep "Unknown Block Count: " | awk '{print $4}'`
	ubi_volumes=($(${yq} .${platform}.${part_type}.ubi ${partition_config} | ${yq} 'keys' - | sed 's/^- //'))
	rootfs_size=($(${yq} .${platform}.${part_type}.ubi.rootfs.size ${partition_config}))
	rootfs_size_dec=$((rootfs_size))
	rootfs_data_size=($(${yq} .${platform}.${part_type}.ubi.rootfs_data.size ${partition_config}))
	rootfs_data_size_dec=$((rootfs_data_size))
	uboot_env_size=($(${yq} .${platform}.${part_type}.ubi."u-boot-env".size ${partition_config}))
	uboot_env_size_dec=$((uboot_env_size))
	ubi_volume_num=`${yq} .${platform}.${part_type}.ubi ${partition_config} | ${yq} 'keys' - | wc -l`

	i=0
	for i in $(seq 0 $((${ubi_volume_num}-1)) ); do
		volume_name=${ubi_volumes[${i}]}
		cat << EOF >> ${ubinize_config}
[${volume_name}]
mode=ubi
vol_id=${i}
vol_type=dynamic
vol_name=${volume_name}
EOF
		if [[ ${volume_name} == "kernel" ]] || [[ ${volume_name} == "kernel2" ]]; then
			kernel_file=(${ws_folder}/${kernel_image}/*kernel.ubifs)
			echo "image=${kernel_file}" >> ${ubinize_config}
		elif [[ ${volume_name} == "rootfs" ]] || [[ ${volume_name} == "rootfs2" ]]; then
			rootfs_file=(${ws_folder}/${kernel_image}/*rootfs.ubifs)
			echo "image=${rootfs_file}" >> ${ubinize_config}
			echo "vol_size=${rootfs_size_dec}" >> ${ubinize_config}
		elif [[ ${volume_name} == "rootfs_data" ]]; then
			echo "vol_size=${rootfs_data_size_dec}" >> ${ubinize_config}
		elif [[ ${volume_name} == "u-boot-env" ]]; then
			echo "vol_size=${uboot_env_size_dec}" >> ${ubinize_config}
		fi
	done
	#echo "ubinize -o ${ws_folder}/${new_ubi_image} -p ${peb_size} -m ${min_io_size} -e ${ec} ${ubinize_config}"
	ubinize -o ${ws_folder}/${new_ubi_image} -p ${peb_size} -m ${min_io_size} -e ${ec} ${ubinize_config}
	rm ${ubinize_config}
	kernel_image=${ws_folder}/${new_ubi_image}
}

recreate_sdmmc_kernel() {
	kernel_single=${ws_folder}/kernel_single.bin
	dd if=/dev/zero ibs=$kernel_size_dec count=1 > $kernel_single
	dd if=$kernel_image of=$kernel_single bs=512 seek=0 conv=notrunc
	dd if=$rootfs_image of=$kernel_single bs=512 \
		seek=$(( ($kernel_size_dec/512) )) conv=notrunc
	kernel_image=${kernel_single}
}

recreate_sdmmc_kernel_dual() {
	kernel_dual=${ws_folder}/kernel_dual.bin
	pad_length=$(( ${kernel_size_dec} + ${rootfs_size_dec} + ${kernel2_size_dec}))
	dd if=/dev/zero ibs=$pad_length count=1 > $kernel_dual
	dd if=$kernel_image of=$kernel_dual bs=512 seek=0 conv=notrunc
	dd if=$rootfs_image of=$kernel_dual bs=512 \
		seek=$(( ($kernel_size_dec/512) )) conv=notrunc
	dd if=$kernel_image of=$kernel_dual bs=512 \
		seek=$(( ($kernel_size_dec + $rootfs_size_dec)/512 )) conv=notrunc
	dd if=$rootfs_image of=$kernel_dual bs=512 \
		seek=$(( ($kernel_size_dec + $rootfs_size_dec + $kernel2_size_dec)/512 )) conv=notrunc
	kernel_image=${kernel_dual}
}

start_wrapping() {
	if [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
	then
		printf "[wrapping GPT......]\n"
		dd if=$gpt of=$single_image bs=512 seek=0 conv=notrunc
	fi

	if [[ $flash_type != "emmc" ]]
	then
		printf "[wrapping BL2 image......]\n"
		dd if=$bl2_image of=$single_image bs=512 \
			seek=$(( ($bl2_start_dec/512) )) conv=notrunc
	fi

	if [[ -n $rf_image ]]
	then
		printf "[wrapping RF image......]\n"
		dd if=$rf_image of=$single_image bs=512 \
			seek=$(( ($rf_start_dec/512) )) conv=notrunc
	fi

	printf "[wrapping FIP image......]\n"
	dd if=$fip_image of=$single_image bs=512 \
		seek=$(( ($fip_start_dec/512) )) conv=notrunc

	if [[ -n $kernel_image ]]
	then
		printf "[wrapping kernel image......]\n"
		dd if=$kernel_image of=$single_image bs=512 \
				seek=$(( ($kernel_start_dec/512) )) conv=notrunc
	fi
}

destroy_workspace() {
	rm -r ${ws_folder}
}

######## Check if variables are valid ########
if ! [[ $platform =~ ^(mt7981|mt7986|mt7988|mt7987)$ ]]; then
	printf "${RED}Platform must be in mt7981|mt7986|mt7988|mt7987\n${NC}"
	usage
	exit 1
fi
if ! [[ $flash_type =~ ^(snfi-nand|spim-nand|spim-nor|emmc|sd)$ ]]; then
	printf "${RED}Flash type must be in snfi-nand|spim-nand|spim-nor|emmc|sd\n${NC}"
	usage
	exit 1
fi

if [[ $partition_config_default -eq 1 ]]; then
	partition_config="./partitions/${flash_type}.yml"
fi

# For SPIM-NAND/SPIM-NOR/SNFI-NAND:
# There must at least be bl2.img & fip.bin
# For eMMC:
# There must at least be GPT & fip.bin
# For SD:
# There must at least be GPT & bl2.img & fip.bin
if [[ $flash_type =~ ^(emmc|sd)$ ]] && [[ -z $gpt ]]; then
	printf "${RED}GPT table must be provided if flash type is emmc or sd\n${NC}"
	exit 1
fi

if [[ $flash_type =~ ^(emmc)$ ]]; then
	if [[ $bl2_default -eq 0 ]]; then
		printf "${RED}eMMC single image doesn't contain bl2 image, please don't specify bl2\n${NC}"
		exit 1
	fi
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
	if ! [[ $ubi_enbable -eq 1 ]]; then
		printf "${RED}Kernel image provided doesn't exist.\n${NC}"
		exit 1
	fi
fi
##############################################

######## Check if partition configs are valid ########
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

if ! [[ -f $bl2_image ]] && [[ $flash_type != "emmc" ]]
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

check_ok=1
######################################################

if [[ $check_ok == 1 ]]; then
	echo "========================================="
	create_workspace
	load_partition
	pad_output_image

	if [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
	then
		extract_sdmmc_kernel
		recreate_sdmmc_kernel
	fi

	if [[ ${dual_image_mode} -eq 1 ]]; then
		#create_workspace
		if [[ $flash_type == "spim-nand" ]] || [[ $flash_type == "snfi-nand" ]]
		then
			recreate_ubimage
		elif [[ $flash_type == "emmc" ]] || [[ $flash_type == "sd" ]]
		then
			recreate_sdmmc_kernel_dual
		fi
	fi

	start_wrapping
	destroy_workspace
fi
