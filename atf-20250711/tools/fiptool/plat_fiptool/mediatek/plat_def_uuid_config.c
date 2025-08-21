// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 */

#include <stddef.h>
#include <string.h>
#include <endian.h>
#include <firmware_image_package.h>
#include "tbbr_config.h"
#include "crc32.h"
#include "sha256.h"

toc_entry_t plat_def_toc_entries[] = {
	{
		/* Must be the last one */
		.name = "FIP checksum data",
		.uuid = UUID_MTK_FIP_CHECKSUM,
		.cmdline_name = "fip-chksum"
	},

	{
		.name = NULL,
		.uuid = { {0} },
		.cmdline_name = NULL,
	}
};

static struct mtk_fip_checksum chksum;
static mbedtls_sha256_context sha256ctx;

void mtk_fip_chksum_init(void)
{
	memset(&chksum, 0, sizeof(chksum));
	mbedtls_sha256_init(&sha256ctx);
	mbedtls_sha256_starts(&sha256ctx, 0); /* SHA-256, not 224 */
}

void mtk_fip_chksum_update(const void *buf, size_t len)
{
	mbedtls_sha256_update(&sha256ctx, buf, len);
	chksum.crc = crc32(chksum.crc, buf, len);
	chksum.len += len;
}

void mtk_fip_chksum_finish(void *buf)
{
	mbedtls_sha256_finish(&sha256ctx, chksum.sha256sum);
	chksum.len = htole32(chksum.len);
	chksum.crc = htole32(chksum.crc);

	memcpy(buf, &chksum, sizeof(chksum));

	mbedtls_sha256_free(&sha256ctx);
}
