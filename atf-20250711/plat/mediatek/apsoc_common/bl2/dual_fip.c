// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <stddef.h>
#include <string.h>
#include <common/debug.h>
#include <common/tf_crc32.h>
#include <drivers/io/io_driver.h>
#include <tools_share/firmware_image_package.h>
#include <plat_def_fip_uuid.h>
#include "bl2_plat_setup.h"
#include "bsp_conf.h"
#include "dual_fip.h"

#if TRUSTED_BOARD_BOOT
#include <mbedtls/sha256.h>
#else
#include "sha256/sha256.h"
#endif

#define FIP_TOC_ENTRY_READ_NUM			16
#define FIP_TOC_ENTRY_MAX_NUM			(FIP_TOC_ENTRY_READ_NUM * 2)

static const uuid_t fip_uuids[] = {
	UUID_EL3_RUNTIME_FIRMWARE_BL31,
	UUID_NON_TRUSTED_FIRMWARE_BL33,
#ifdef NEED_BL32
	UUID_SECURE_PAYLOAD_BL32,
#endif
#if TRUSTED_BOARD_BOOT
	UUID_TRUSTED_KEY_CERT,
	UUID_SOC_FW_KEY_CERT,
	UUID_SOC_FW_CONTENT_CERT,
	UUID_NON_TRUSTED_FW_KEY_CERT,
	UUID_NON_TRUSTED_FW_CONTENT_CERT,
#ifdef NEED_BL32
	UUID_TRUSTED_OS_FW_KEY_CERT,
	UUID_TRUSTED_OS_FW_CONTENT_CERT,
#endif
#endif
};

static const uuid_t uuid_null;
static const uuid_t uuid_fip_chksum = UUID_MTK_FIP_CHECKSUM;

static inline int compare_uuid(const uuid_t *uuid1, const uuid_t *uuid2)
{
	return memcmp(uuid1, uuid2, sizeof(uuid_t));
}

static void uuid_to_str(char *s, size_t len, const uuid_t *u)
{
	snprintf(s, len,
	    "%02X%02X%02X%02X-%02X%02X-%02X%02X-%04X-%04X%04X%04X",
	    u->time_low[0], u->time_low[1], u->time_low[2], u->time_low[3],
	    u->time_mid[0], u->time_mid[1],
	    u->time_hi_and_version[0], u->time_hi_and_version[1],
	    (u->clock_seq_hi_and_reserved << 8) | u->clock_seq_low,
	    (u->node[0] << 8) | u->node[1],
	    (u->node[2] << 8) | u->node[3],
	    (u->node[4] << 8) | u->node[5]);
}

static int load_validate_fip(uintptr_t dev_handle, uintptr_t spec,
			     uint32_t slot)
{
	uintptr_t image_handle, pos, fip_read_len, chklen, chksum_offs = 0;
	bool uuid_matches[ARRAY_SIZE(fip_uuids)] = { false };
	void *fip_buf = (void *)DUAL_FIP_BUF_OFFSET;
	char uuid_str[_UUID_STR_LEN + 1];
	struct mtk_fip_checksum chksum;
	uint64_t entry_end, max_end;
	uint32_t i, crc, ntoc = 0;
	uint8_t sha256sum[0x20];
	fip_toc_entry_t entry;
	fip_toc_header_t hdr;
	int ret;

	if (!dev_handle || !spec) {
		ERROR("FIP slot %u is invalid\n", slot);
		return -EINVAL;
	}

	chklen = FIP_TOC_ENTRY_READ_NUM * sizeof(entry);
	fip_read_len = sizeof(hdr) + chklen;
	entry_end = max_end = sizeof(hdr) + sizeof(entry);

	ret = io_open(dev_handle, spec, &image_handle);
	if (ret) {
		ERROR("Failed to open FIP slot %u for read (%d)\n", slot, ret);
		return ret;
	}

	ret = mtk_io_read_incr(image_handle, fip_buf, fip_read_len);
	if (ret) {
		ERROR("Failed to read FIP slot %u with length %zu (%d)\n", slot,
		      fip_read_len, ret);
		goto cleanup;
	}

	memcpy(&hdr, fip_buf, sizeof(hdr));

	if (hdr.name != TOC_HEADER_NAME || !hdr.serial_number) {
		ERROR("FIP header is invalid\n");
		ret = -EBADMSG;
		goto cleanup;
	}

	pos = sizeof(hdr);

	while (pos < fip_read_len) {
		memcpy(&entry, fip_buf + pos, sizeof(entry));

		if (!compare_uuid(&entry.uuid, &uuid_null)) {
			if (entry.offset_address < max_end) {
				ERROR("FIP null entry has invalid offset\n");
				ret = -EBADMSG;
				goto cleanup;
			}

			if (entry.size) {
				ERROR("FIP null entry has non-zero size\n");
				ret = -EBADMSG;
				goto cleanup;
			}

			max_end = entry.offset_address;
			break;
		}

		for (i = 0; i < ARRAY_SIZE(fip_uuids); i++) {
			if (!compare_uuid(&entry.uuid, &fip_uuids[i]))
				uuid_matches[i] = true;
		}

		if (entry.offset_address < pos + sizeof(entry) * 2) {
			uuid_to_str(uuid_str, sizeof(uuid_str), &entry.uuid);
			ERROR("FIP entry %s has invalid offset\n", uuid_str);
			ret = -EBADMSG;
			goto cleanup;
		}

		entry_end = entry.offset_address + entry.size;
		if (entry_end > max_end)
			max_end = entry_end;

		if (!compare_uuid(&entry.uuid, &uuid_fip_chksum)) {
			if (entry.size < sizeof(struct mtk_fip_checksum)) {
				ERROR("Invalid FIP chekcsum data\n");
				ret = -EBADMSG;
				goto cleanup;
			}

			chksum_offs = entry.offset_address;
		}

		ntoc++;
		pos += sizeof(entry);

		if (fip_read_len - pos < sizeof(entry)) {
			if (fip_read_len + chklen >= DUAL_FIP_BUF_SIZE ||
			    ntoc >= FIP_TOC_ENTRY_MAX_NUM) {
				ERROR("Too many FIP toc entries\n");
				ret = -EBADMSG;
				goto cleanup;
			}

			ret = mtk_io_read_incr(image_handle,
					       fip_buf + fip_read_len, chklen);
			if (ret) {
				ERROR("Failed to read FIP slot %u with length %zu (%d)\n",
				      slot, chklen, ret);
				goto cleanup;
			}

			fip_read_len += chklen;
		}
	}

	if (max_end > DUAL_FIP_BUF_SIZE) {
		ERROR("FIP is too big\n");
		ret = -EBADMSG;
		goto cleanup;
	}

	for (i = 0; i < ARRAY_SIZE(fip_uuids); i++) {
		if (!uuid_matches[i]) {
			uuid_to_str(uuid_str, sizeof(uuid_str), &fip_uuids[i]);
			ERROR("FIP is missing toc entry %s\n", uuid_str);
			ret = -EBADMSG;
			goto cleanup;
		}
	}

	if (!chksum_offs) {
		ERROR("FIP checksum data not exist\n");
		ret = -EBADMSG;
		goto cleanup;
	}

	if (entry_end > chksum_offs + sizeof(chksum))
		WARN("Extra data after FIP checksum data\n");

	if (chksum_offs + sizeof(chksum) > fip_read_len) {
		chklen = chksum_offs + sizeof(chksum) - fip_read_len;

		ret = mtk_io_read_incr(image_handle, fip_buf + fip_read_len,
				       chklen);
		if (ret) {
			ERROR("Failed to read FIP slot %u with length %zu (%d)\n",
			      slot, chklen, ret);
			goto cleanup;
		}
	}

	io_close(image_handle);

	memcpy(&chksum, fip_buf + chksum_offs, sizeof(chksum));

	if (chksum.len != chksum_offs) {
		ERROR("FIP checksum data mismatch\n");
		return -EBADMSG;
	}

	crc = tf_crc32(0, fip_buf, chksum.len);
	if (crc != chksum.crc) {
		ERROR("FIP checksum CRC32 mismatch (calculated %08x, expect %08x)\n",
		      crc, chksum.crc);
		return -EBADMSG;
	}

	mbedtls_sha256(fip_buf, chksum.len, sha256sum, 0);

	if (memcmp(sha256sum, chksum.sha256sum, sizeof(sha256sum))) {
		ERROR("FIP checksum SHA256 hash mismatch\n");
		return -EBADMSG;
	}

	return 0;

cleanup:
	io_close(image_handle);

	return ret;
}

static int get_fip_slot(const uintptr_t dev_handles[], const uintptr_t specs[],
			bool curr_invalid, uint32_t *retslot)
{
	uint32_t curr_slot, n = FIP_NUM;
	int ret;

	curr_slot = curr_bspconf.current_fip_slot;
	if (curr_slot >= FIP_NUM) {
		curr_slot %= FIP_NUM;
		WARN("Current FIP slot is invalid. Assuming %u\n", curr_slot);
	}

	while (n) {
		if (curr_bspconf.fip[curr_slot].invalid || curr_invalid) {
			if (curr_invalid) {
				curr_bspconf.fip[curr_slot].invalid = 1;

				WARN("Failed to load FIP slot %u\n", curr_slot);
			} else {
				WARN("FIP slot %u was marked invalid\n",
				     curr_slot);
			}
		} else {
			NOTICE("Verifying FIP slot %u ...\n", curr_slot);

			ret = load_validate_fip(dev_handles[curr_slot],
						specs[curr_slot], curr_slot);
			if (!ret) {
				NOTICE("FIP slot %u is usable\n", curr_slot);
				curr_bspconf.current_fip_slot = curr_slot;
				*retslot = curr_slot;
				return 0;
			}

			WARN("FIP slot %u is unusable\n", curr_slot);

			curr_bspconf.fip[curr_slot].invalid = 1;
		}

		curr_slot = (curr_slot - 1) % FIP_NUM;
		curr_invalid = false;
		n--;
	}

	ERROR("No valid FIP slot\n");

	return -EOPNOTSUPP;
}

int dual_fip_check(const uintptr_t dev_handles[], const uintptr_t specs[],
		   uint32_t *retslot)
{
	return get_fip_slot(dev_handles, specs, false, retslot);
}

int dual_fip_next_slot(const uintptr_t dev_handles[], const uintptr_t specs[],
		       uint32_t *retslot)
{
	return get_fip_slot(dev_handles, specs, true, retslot);
}
