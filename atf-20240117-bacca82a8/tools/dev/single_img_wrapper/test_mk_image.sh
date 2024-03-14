#!/bin/bash

# Import the script
#source ./my_script.sh
debug_mode=1
data="./test_data"
script="mk_image.sh"
output_bl2_fip="${data}/bl2_fip.bin"
output_bl2_factory_fip="${data}/bl2_factory_fip.bin"
output_gpt_fip="${data}/gpt_fip.bin"
output_single_image="${data}/single_image.bin"
declare -a platforms=("mt7981" "mt7986" "mt7988")
declare -a nand_nor=("spim-nand" "snfi-nand" "spim-nor")
declare -a sdmmc=("emmc" "sd")

debug() {
	command=$1
	if [[ ${debug_mode} -eq 1 ]]; then
		echo "- [ CMD: ${command} ]"
	fi
}

############################## Default cases start here: ##############################
test_Normalcase_usage() {
	cmd="./${script}"
	debug "${cmd}"
	result=$(${cmd})
	expect=$(cat ${data}/usage.txt)
	assertEquals "${expect}" "${result}"
}

test_Normalcase_nand_nor_combine_bl2_fip() {
	for platform in "${platforms[@]}"
	do
		for flash in "${nand_nor[@]}"
		do
			cmd="./${script} -p ${platform} -d ${flash} \
-b ${data}/common-bl2.img \
-f ${data}/common-fip.bin \
-o ${output_bl2_fip}"
			debug "${cmd}"
			eval ${cmd} > /dev/null 2>&1

			diff ${output_bl2_fip} ${data}/${platform}-${flash}-bl2-fip.bin
			diff_exit_status=$?
			rm ${output_bl2_fip}
			assertEquals "Generated file differs from expected output." 0 ${diff_exit_status}
		done
	done
}

test_Normalcase_nand_nor_combine_bl2_factory_fip() {
	for platform in "${platforms[@]}"
	do
		for flash in "${nand_nor[@]}"
		do
			cmd="./${script} -p ${platform} -d ${flash} \
-b ${data}/common-bl2.img \
-r ${data}/common-factory.bin \
-f ${data}/common-fip.bin \
-o ${output_bl2_fip}"
			debug "${cmd}"
			eval ${cmd} > /dev/null 2>&1

			diff ${output_bl2_fip} ${data}/${platform}-${flash}-bl2-factory-fip.bin
			diff_exit_status=$?
			rm ${output_bl2_fip}
			assertEquals "Generated file differs from expected output." 0 ${diff_exit_status}
		done
	done
}

test_Normalcase_spim_nand_default_single_image() {
	for platform in "${platforms[@]}"
	do
		for flash in "${nand_nor[@]}"
		do
			cmd="./${script} -p ${platform} -d ${flash} \
-b ${data}/common-bl2.img \
-r ${data}/common-factory.bin \
-f ${data}/common-fip.bin \
-k ${data}/nand-kernel-factory.bin \
-o ${output_single_image}"
			debug "${cmd}"
			eval ${cmd} > /dev/null 2>&1

			diff ${output_single_image} ${data}/${platform}-${flash}-single-image.bin
			diff_exit_status=$?
			rm ${output_single_image}
			assertEquals "Generated file differs from expected output." 0 ${diff_exit_status}
		done
	done
}

: <<'END'
test_Normalcase_nand_dual_image() {
	for platform in "${platforms[@]}"
	do
		cmd="./${script} -p ${platform} -d spim-nand \
-b ${data}/common-bl2.img \
-r ${data}/common-factory.bin \
-f ${data}/common-fip.bin \
-k ${data}/nand-kernel-factory.bin \
--dual_image \
-o ${output_single_image}"

		debug "${cmd}"
		exit 0
		eval ${cmd} > /dev/null 2>&1

		diff ${output_single_image} ${data}/${platform}-${flash}-single-image.bin
		diff_exit_status=$?
		rm ${output_single_image}
		assertEquals "Generated file differs from expected output." 0 ${diff_exit_status}
	done
}
END

test_Normalcase_emmc_combine_gpt_fip() {
	for platform in "${platforms[@]}"
	do
		cmd="./${script} -p ${platform} -d emmc \
-g ${data}/common-GPT.bin \
-f ${data}/common-fip.bin \
-o ${output_gpt_fip}"
		debug "${cmd}"
		eval ${cmd} > /dev/null 2>&1

		diff ${output_gpt_fip} ${data}/${platform}-emmc-gpt-fip.bin
		diff_exit_status=$?
		rm ${output_gpt_fip}
		assertEquals "Generated file differs from expected output." 0 ${diff_exit_status}
	done
}

############################## Error cases start here: ##############################
test_Errorcase__sdmmc_without_gpt() {
	for platform in "${platforms[@]}"
	do
		# With BL2 specified
		# With BL2/factory specified
		# With BL2/factory/fip specified
		# With BL2/factory/fip/kernel specified
		cmd="./${script} -p ${platform} -d ${sdmmc} \
-b ${data}/common-bl2.img \
-f ${data}/common-fip.bin \
-o ${output_bl2_fip}"
		debug "${cmd}"
		output=$(${cmd})
		output=$(echo "$output" | sed -r "s/\x1B\[[0-9;]*[a-zA-Z]//g") #Remove color ASCII
		expected_output="GPT table must be provided if flash type is emmc or sd"

		assertEquals "Output message not expected" "${expected_output}" "${output}"
	done
}

# Load shunit2
. /usr/bin/shunit2
#. /usr/share/shunit2/shunit2
