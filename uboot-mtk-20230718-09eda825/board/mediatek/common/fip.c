// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <common.h>
#include <u-boot/sha256.h>
#include "firmware_image_package.h"
#include "fip.h"

#define SHA256_HASH_STRLEN	(SHA256_SUM_LEN * 2 + 1)
#define FIP_MIN_SIZE		(sizeof(struct fip_toc_header) + \
				 sizeof(struct fip_toc_entry))

/* The images used depends on the platform. */
static const struct toc_entry toc_entries[MAX_TOE_ENTRY] = {
	{ .desc = "SCP Firmware Updater Configuration FWU SCP_BL2U",
	  .uuid = UUID_TRUSTED_UPDATE_FIRMWARE_SCP_BL2U,
	  .name = "scp-fwu-cfg" },
	{ .desc = "AP Firmware Updater Configuration BL2U",
	  .uuid = UUID_TRUSTED_UPDATE_FIRMWARE_BL2U,
	  .name = "ap-fwu-cfg" },
	{ .desc = "Firmware Updater NS_BL2U",
	  .uuid = UUID_TRUSTED_UPDATE_FIRMWARE_NS_BL2U,
	  .name = "fwu" },
	{ .desc = "Non-Trusted Firmware Updater certificate",
	  .uuid = UUID_TRUSTED_FWU_CERT,
	  .name = "fwu-cert" },
	{ .desc = "Trusted Boot Firmware BL2",
	  .uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
	  .name = "tb-fw" },
	{ .desc = "SCP Firmware SCP_BL2",
	  .uuid = UUID_SCP_FIRMWARE_SCP_BL2,
	  .name = "scp-fw" },
	{ .desc = "EL3 Runtime Firmware BL31",
	  .uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
	  .name = "soc-fw" },
	{ .desc = "Secure Payload BL32 (Trusted OS)",
	  .uuid = UUID_SECURE_PAYLOAD_BL32,
	  .name = "tos-fw" },
	{ .desc = "Secure Payload BL32 Extra1 (Trusted OS Extra1)",
	  .uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA1,
	  .name = "tos-fw-extra1" },
	{ .desc = "Secure Payload BL32 Extra2 (Trusted OS Extra2)",
	  .uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA2,
	  .name = "tos-fw-extra2" },
	{ .desc = "Non-Trusted Firmware BL33",
	  .uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
	  .name = "nt-fw" },
	{ .desc = "Realm Monitor Management Firmware",
	  .uuid = UUID_REALM_MONITOR_MGMT_FIRMWARE,
	  .name = "rmm-fw" },

	/* Dynamic Configs */
	{ .desc = "FW_CONFIG",
	  .uuid = UUID_FW_CONFIG,
	  .name = "fw-config" },
	{ .desc = "HW_CONFIG",
	  .uuid = UUID_HW_CONFIG,
	  .name = "hw-config" },
	{ .desc = "TB_FW_CONFIG",
	  .uuid = UUID_TB_FW_CONFIG,
	  .name = "tb-fw-config" },
	{ .desc = "SOC_FW_CONFIG",
	  .uuid = UUID_SOC_FW_CONFIG,
	  .name = "soc-fw-config" },
	{ .desc = "TOS_FW_CONFIG",
	  .uuid = UUID_TOS_FW_CONFIG,
	  .name = "tos-fw-config" },
	{ .desc = "NT_FW_CONFIG",
	  .uuid = UUID_NT_FW_CONFIG,
	  .name = "nt-fw-config" },

	/* Key Certificates */
	{ .desc = "Root Of Trust key certificate",
	  .uuid = UUID_ROT_KEY_CERT,
	  .name = "rot-cert" },
	{ .desc = "Trusted key certificate",
	  .uuid = UUID_TRUSTED_KEY_CERT,
	  .name = "trusted-key-cert" },
	{ .desc = "SCP Firmware key certificate",
	  .uuid = UUID_SCP_FW_KEY_CERT,
	  .name = "scp-fw-key-cert" },
	{ .desc = "SoC Firmware key certificate",
	  .uuid = UUID_SOC_FW_KEY_CERT,
	  .name = "soc-fw-key-cert" },
	{ .desc = "Trusted OS Firmware key certificate",
	  .uuid = UUID_TRUSTED_OS_FW_KEY_CERT,
	  .name = "tos-fw-key-cert" },
	{ .desc = "Non-Trusted Firmware key certificate",
	  .uuid = UUID_NON_TRUSTED_FW_KEY_CERT,
	  .name = "nt-fw-key-cert" },

	/* Content certificates */
	{ .desc = "Trusted Boot Firmware BL2 certificate",
	  .uuid = UUID_TRUSTED_BOOT_FW_CERT,
	  .name = "tb-fw-cert" },
	{ .desc = "SCP Firmware content certificate",
	  .uuid = UUID_SCP_FW_CONTENT_CERT,
	  .name = "scp-fw-cert" },
	{ .desc = "SoC Firmware content certificate",
	  .uuid = UUID_SOC_FW_CONTENT_CERT,
	  .name = "soc-fw-cert" },
	{ .desc = "Trusted OS Firmware content certificate",
	  .uuid = UUID_TRUSTED_OS_FW_CONTENT_CERT,
	  .name = "tos-fw-cert" },
	{ .desc = "Non-Trusted Firmware content certificate",
	  .uuid = UUID_NON_TRUSTED_FW_CONTENT_CERT,
	  .name = "nt-fw-cert" },
	{ .desc = "SiP owned Secure Partition content certificate",
	  .uuid = UUID_SIP_SECURE_PARTITION_CONTENT_CERT,
	  .name = "sip-sp-cert" },
	{ .desc = "Platform owned Secure Partition content certificate",
	  .uuid = UUID_PLAT_SECURE_PARTITION_CONTENT_CERT,
	  .name = "plat-sp-cert" },
	{
		.desc = NULL,
		.uuid = UUID_NULL,
		.name = NULL,
	}
};

/*****************************************************************************
 * FIP PARSE
 *****************************************************************************/
static LIST_HEAD(common_image_desc_list);

static void free_image_descs(void)
{
	struct image_desc *desc;
	struct image_desc *tmp;

	list_for_each_entry_safe(desc, tmp, &common_image_desc_list, list) {
		list_del(&desc->list);
		free(desc);
	}
}

int fill_image_descs(void)
{
	const struct toc_entry *toc_entry;
	struct image_desc *desc;
	int ret;

	for (toc_entry = toc_entries; toc_entry->name; toc_entry++) {
		desc = calloc(1, sizeof(struct image_desc));
		if (!desc) {
			ret = -ENOMEM;
			goto err;
		}
		memcpy(&desc->uuid, &toc_entry->uuid, sizeof(union fip_uuid));
		desc->desc = toc_entry->desc;
		desc->name = toc_entry->name;
		list_add(&desc->list, &common_image_desc_list);
	}

	return 0;

err:
	free_image_descs();
	return ret;
}

static struct image_desc *lookup_image_desc(const union fip_uuid *uuid)
{
	struct image_desc *desc;

	list_for_each_entry(desc, &common_image_desc_list, list) {
		if (memcmp(&desc->uuid, uuid, sizeof(union fip_uuid)) == 0)
			return desc;
	}

	return NULL;
}

static const char *hexchars = "0123456789abcdef";

static void hash_sha256_sum(const void *data, size_t data_len, char *hash_str)
{
	u8 value[SHA256_SUM_LEN];
	int i;

	sha256_csum_wd(data, data_len, value, CHUNKSZ_SHA256);

	for (i = 0; i < SHA256_SUM_LEN ; i++) {
		hash_str[i * 2] = hexchars[(value[i] >> 4) & 0xf];
		hash_str[i * 2 + 1] = hexchars[value[i] & 0xf];
	}
}

int fip_find_image_desc(struct fip *fip, const char *name,
			struct image_desc **image_desc)
{
	struct image_desc *desc;
	char buffer[256];
	int found = 0;
	int i;

	if (strlen(name) > 255)
		return -EINVAL;

	memset(buffer, 0x0, 256);
	strcpy(buffer, name);

	for (i = 0 ; i < strlen(buffer) ; i++) {
		if (('A' <= buffer[i]) && (buffer[i] <= 'Z'))
			buffer[i] = buffer[i] + 0x20;
	}

	if (strcmp(buffer, "bl31") == 0)
		name = "soc-fw";
	else if (strcmp(buffer, "bl33") == 0)
		name = "nt-fw";
	else if (strcmp(buffer, "uboot") == 0)
		name = "nt-fw";
	else if (strcmp(buffer, "u-boot") == 0)
		name = "nt-fw";
	else if (strcmp(buffer, "bootloader") == 0)
		name = "nt-fw";

	list_for_each_entry(desc, &fip->images, list) {
		if (strcmp(desc->name, name) == 0) {
			*image_desc = desc;
			found = 1;
			break;
		}
	}

	if (!found) {
		*image_desc = NULL;
		debug("%s: image '%s' not found in fip\n", __func__, name);
		return -EINVAL;
	}

	return 0;
}

static void _show_image(struct image_desc *desc)
{
	char hash[SHA256_HASH_STRLEN];

	memset(hash, 0x0, SHA256_HASH_STRLEN);
	hash_sha256_sum(desc->data, desc->toc_e.size, hash);

	printf("%s: offset=0x%llX, size=0x%llX, name=\"%s\", sha256=%s\n",
	       desc->desc, desc->toc_e.offset_address, desc->toc_e.size,
	       desc->name, hash);
}

int fip_show_image(struct fip *fip, const char *name)
{
	struct image_desc *desc;
	int ret;

	ret = fip_find_image_desc(fip, name, &desc);
	if (ret)
		return ret;
	_show_image(desc);

	return 0;
}

int fip_get_image(struct fip *fip, const char *name, const void **data,
		  size_t *size)
{
	struct image_desc *desc;
	int ret;

	ret = fip_find_image_desc(fip, name, &desc);
	if (ret)
		return ret;

	*data = desc->data;
	*size = desc->toc_e.size;

	return 0;

}

int fip_show(struct fip *fip)
{
	struct image_desc *desc;
	char hash[SHA256_HASH_STRLEN];

	memset(hash, 0x0, SHA256_HASH_STRLEN);
	hash_sha256_sum(fip->data, fip->size, hash);

	printf("size=%ld (%lx hex), sha256=%s\n", fip->size, fip->size, hash);

	debug("toc_header[name]: 0x%x\n", fip->toc_header->name);
	debug("toc_header[serial_number]: 0x%x\n",
	      fip->toc_header->serial_number);
	debug("toc_header[flags]: 0x%llx\n", fip->toc_header->flags);

	list_for_each_entry(desc, &fip->images, list) {
		_show_image(desc);
	}

	return 0;
}

int fip_unpack(struct fip *fip, const char *name, void *data_out,
	       size_t max_len, size_t *data_len_out)
{
	struct image_desc *desc;
	int ret;

	ret = fip_find_image_desc(fip, name, &desc);
	if (ret)
		return ret;

	if (desc->toc_e.size > max_len) {
		debug("%s: image '%s' size is too large\n", __func__, name);
		return -ENOBUFS;
	}

	memcpy(data_out, desc->data, desc->toc_e.size);
	*data_len_out = desc->toc_e.size;

	return 0;
}

int fip_update(struct fip *fip, const char *name, const void *data_in,
	       size_t size)
{
	struct image_desc *desc, *next;
	size_t offset;
	int ret;

	ret = fip_find_image_desc(fip, name, &desc);
	if (ret)
		return ret;

	desc->data = data_in;
	desc->toc_e.size = size;

	/* We should update offset_address for all images in the fip */
	if (!list_is_last(&desc->list, &fip->images)) {
		next = list_entry((desc)->list.next, typeof(*(desc)), list);
		offset = desc->toc_e.offset_address + desc->toc_e.size;
		/* Check input values */
		if (offset < desc->toc_e.offset_address ||
		    offset < desc->toc_e.size)
			return -EOVERFLOW;
		list_for_each_entry_from(next, &fip->images, list) {
			next->toc_e.offset_address = offset;
			offset = next->toc_e.offset_address + next->toc_e.size;
			/* Check input values */
			if (offset < desc->toc_e.offset_address ||
			    offset < desc->toc_e.size)
				return -EOVERFLOW;
		}
		fip->size = offset;
	} else {
		fip->size = desc->toc_e.offset_address + desc->toc_e.size;
		/* Check input values */
		if (fip->size < desc->toc_e.offset_address ||
		    fip->size < desc->toc_e.size)
			return -EOVERFLOW;
	}

	return 0;
}

int fip_create(struct fip *fip, void *data_out, size_t max_len,
	       size_t *data_len_out)
{
	static const union fip_uuid uuid_null = UUID_NULL;
	struct fip_toc_entry last_toc_null_entry;
	struct image_desc *desc;
	size_t size = 0;

	if (fip->size > max_len) {
		debug("%s: fip image size is too large\n", __func__);
		return -ENOBUFS;
	}

	/* write fip header to data_out */
	memcpy(data_out, fip->toc_header, sizeof(struct fip_toc_header));
	size += sizeof(struct fip_toc_header);

	/* write toc_entry to data_out */
	list_for_each_entry(desc, &fip->images, list) {
		memcpy(data_out + size, &desc->toc_e,
		       sizeof(struct fip_toc_entry));
		size += sizeof(struct fip_toc_entry);
	}

	/* write last toc_entry to data_out (toc_null) */
	memset(&last_toc_null_entry, 0x0, sizeof(struct fip_toc_entry));
	memcpy(&last_toc_null_entry, &uuid_null, sizeof(union fip_uuid));
	last_toc_null_entry.offset_address = fip->size;
	memcpy(data_out + size, &last_toc_null_entry,
	       sizeof(struct fip_toc_entry));
	size += sizeof(struct fip_toc_entry);

	/* write every image to data_out */
	list_for_each_entry(desc, &fip->images, list) {
		memcpy(data_out + desc->toc_e.offset_address, desc->data,
		       desc->toc_e.size);
		size += desc->toc_e.size;
	}

	*data_len_out = size;

	return 0;
}

int fip_repack(struct fip *fip, void *data_swap, size_t max_len)
{
	size_t size;
	int ret;

	if (fip->size > max_len)
		return -ENOBUFS;

	if (!data_swap)
		return -EINVAL;

	ret = fip_create(fip, data_swap, fip->size, &size);
	if (ret)
		return ret;

	memcpy((u8 *)fip->data, data_swap, size);

	return 0;
}

void free_fip(struct fip *fip)
{
	struct image_desc *desc;
	struct image_desc *tmp;

	if (!fip)
		return;

	list_for_each_entry_safe(desc, tmp, &fip->images, list) {
		list_del(&desc->list);
		free(desc);
	}
}

int init_fip(const void *data, size_t size, struct fip *fip)
{
	static const union fip_uuid uuid_null = UUID_NULL;
	struct image_desc *desc, *common_desc;
	struct fip_toc_header *toc_header;
	struct fip_toc_entry *toc_entry;
	const u8 *end = data + size;
	size_t fip_size = 0;
	size_t value;
	int ret;

	ret = fill_image_descs();
	if (ret)
		return ret;

	if (!fip)
		return -EINVAL;

	/* initialize list head in the fip struct */
	INIT_LIST_HEAD(&fip->images);

	if (size < FIP_MIN_SIZE) {
		debug("%s: input data is not a FIP file\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	toc_header = (struct fip_toc_header *)data;

	if (toc_header->name != TOC_HEADER_NAME) {
		debug("%s: input data is not a FIP file\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	/* initialize every attribute in the fip struct */
	fip->toc_header = toc_header;

	/* Walk through each ToC entry in the file. */
	toc_entry = (struct fip_toc_entry *)(toc_header + 1);
	for (; (u8 *)(toc_entry + 1) <= end; toc_entry++) {
		/* Check if we has enough space for one ToC entry or not */
		if (end - (u8 *)toc_entry < sizeof(struct fip_toc_entry)) {
			debug("%s: FIP image was broken\n", __func__);
			ret = -EINVAL;
			goto err;
		}
		/* Found the ToC terminator, we are done. */
		if (!memcmp(&toc_entry->uuid, &uuid_null,
			    sizeof(union fip_uuid))) {
			break;
		}
		/* Compare image described by toc_entry with common fip image */
		common_desc = lookup_image_desc(&toc_entry->uuid);
		if (!common_desc) {
			debug("%s: unknown image in the FIP file\n", __func__);
			ret = -EINVAL;
			goto err;
		}
		/* Check input values */
		value = toc_entry->offset_address + toc_entry->size;
		if (value < toc_entry->offset_address ||
		    value < toc_entry->size) {
			ret = -EOVERFLOW;
			goto err;
		}
		/* Check image content described by fip_toc_entry */
		if (toc_entry->offset_address + toc_entry->size > size) {
			debug("%s: FIP image was broken\n", __func__);
			ret = -EINVAL;
			goto err;
		}
		if (toc_entry->offset_address + toc_entry->size > fip_size)
			fip_size = toc_entry->offset_address + toc_entry->size;

		desc = calloc(1, sizeof(struct image_desc));
		if (!desc) {
			ret = -ENOMEM;
			goto err;
		}
		memcpy(desc, common_desc, sizeof(struct image_desc));
		memcpy(&desc->toc_e, toc_entry, sizeof(struct fip_toc_entry));
		desc->data = data + toc_entry->offset_address;
		list_add_tail(&desc->list, &fip->images);
	}
	fip->data = data;
	fip->size = fip_size ? fip_size : size;

	return 0;

err:
	free_fip(fip);
	free_image_descs();

	return ret;
}
