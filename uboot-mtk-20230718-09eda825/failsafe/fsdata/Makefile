# SPDX-License-Identifier: GPL-2.0

# example:
# obj-y += XXXX.o <- object file to generate
# FILE_XXXX.o := YYYY.html <- actual html file to be embedded
# FSPATH_XXXX.o := ZZZ/WWW.EXT <- virtual fs path to be used inside u-boot

obj-y += index.o
FILE_index.o := index.html
FSPATH_index.o := index.html

obj-y += upload.o
FILE_upload.o := upload.html
FSPATH_upload.o := upload.html

obj-y += flashing.o
FILE_flashing.o := flashing.html
FSPATH_flashing.o := flashing.html

obj-y += success.o
FILE_success.o := success.html
FSPATH_success.o := success.html

obj-y += fail.o
FILE_fail.o := fail.html
FSPATH_fail.o := fail.html

obj-y += validate_fail.o
FILE_validate_fail.o := validate_fail.html
FSPATH_validate_fail.o := validate_fail.html

obj-y += 404.o
FILE_404.o := 404.html
FSPATH_404.o := 404.html

obj-y += style.o
FILE_style.o := style.css
FSPATH_style.o := style.css

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
