# SPDX-License-Identifier: GPL-2.0

# example:
# obj-y += XXXX.o <- object file to generate
# FILE_XXXX.o := YYYY.html <- actual html file to be embedded
# FSPATH_XXXX.o := ZZZ/WWW.EXT <- virtual fs path to be used inside u-boot

obj-y += 404.o
FILE_404.o := 404.html
FSPATH_404.o := 404.html

obj-y += bl2.o
FILE_bl2.o := bl2.html
FSPATH_bl2.o := bl2.html

obj-y += booting.o
FILE_booting.o := booting.html
FSPATH_booting.o := booting.html

obj-y += fail.o
FILE_fail.o := fail.html
FSPATH_fail.o := fail.html

obj-y += flashing.o
FILE_flashing.o := flashing.html
FSPATH_flashing.o := flashing.html

obj-y += gpt.o
FILE_gpt.o := gpt.html
FSPATH_gpt.o := gpt.html

obj-y += index.o
FILE_index.o := index.html
FSPATH_index.o := index.html

obj-y += initramfs.o
FILE_initramfs.o := initramfs.html
FSPATH_initramfs.o := initramfs.html

obj-y += main.o
FILE_main.o := main.js
FSPATH_main.o := main.js

obj-y += style.o
FILE_style.o := style.css
FSPATH_style.o := style.css

obj-y += uboot.o
FILE_uboot.o := uboot.html
FSPATH_uboot.o := uboot.html

# customized build rules
strip_path = $(subst /,_,$(subst -,_,$(subst .,_,$(1))))

quiet_cmd_as_o_html = AS      $@
cmd_as_o_html = $(CC) $(a_flags) -c -o $@ \
		-DDATA_SECT_NAME="\".rodata.fsdata.$(call strip_path,$(FSPATH_$(@F)))\"" \
		-DDATA_OBJ_FILE="\"$(srctree)/$(obj)/$(FILE_$(@F))\"" \
		-DDATA_OBJ_NAME="fsdata_$(call strip_path,$(FSPATH_$(@F)))" \
		-DDATA_SIZE_NAME="fsdata_size_$(call strip_path,$(FSPATH_$(@F)))" \
		-DDATA_SECT_FSPATH_NAME="\".rodata.fsdata.path.$(call strip_path,$(FSPATH_$(@F)))\"" \
		-DDATA_OBJ_FSPATH_NAME="fsdata_path_$(call strip_path,$(FSPATH_$(@F)))" \
		-DDATA_OBJ_FSPATH="\"$(FSPATH_$(@F))\"" \
		-DUBOOT_LIST_SECT_NAME="\"__u_boot_list_2_fs_2_$(call strip_path,$(FSPATH_$(@F)))\"" \
		-DUBOOT_LIST_NAME="fsdata_ulist_$(call strip_path,$(FSPATH_$(@F)))" \
		$(srctree)/$(src)/embed.S

$(obj)/%.o: $(src)/%.html FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_dep,as_o_html)

$(obj)/%.o: $(src)/%.css FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_dep,as_o_html)

$(obj)/%.o: $(src)/%.js FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_dep,as_o_html)
