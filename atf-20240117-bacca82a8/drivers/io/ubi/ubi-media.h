/* SPDX-License-Identifier: GPL 2.0+ OR BSD-3-Clause */

#ifndef _UBI_MEDIA_H_
#define _UBI_MEDIA_H_

#include <stdint.h>
#include <cdefs.h>

#define UBI_CRC32_INIT			0xFFFFFFFFU

#define UBI_EC_HDR_MAGIC		0x55424923
#define UBI_VID_HDR_MAGIC		0x55424921

#define UBI_MAX_VOLUMES			128
#define UBI_VOL_NAME_MAX		127

#define UBI_EC_HDR_SIZE			sizeof(struct ubi_ec_hdr)
#define UBI_EC_HDR_SIZE_CRC		(UBI_EC_HDR_SIZE  - sizeof(uint32_t))

#define UBI_VID_HDR_SIZE		sizeof(struct ubi_vid_hdr)
#define UBI_VID_HDR_SIZE_CRC		(UBI_VID_HDR_SIZE - sizeof(uint32_t))

#define UBI_VTBL_RECORD_SIZE		sizeof(struct ubi_vtbl_record)
#define UBI_VTBL_RECORD_SIZE_CRC	(UBI_VTBL_RECORD_SIZE - sizeof(uint32_t))

#define UBI_INTERNAL_VOL_START		(0x7FFFFFFF - 4096)
#define UBI_LAYOUT_VOLUME_ID		UBI_INTERNAL_VOL_START

enum {
	UBI_VID_DYNAMIC = 1,
	UBI_VID_STATIC  = 2
};

struct ubi_ec_hdr {
	uint32_t magic;
	uint8_t  version;
	uint8_t  padding1[3];
	uint32_t padding2[13];
	uint32_t hdr_crc;
};

struct ubi_vid_hdr {
	uint32_t magic;
	uint8_t  version;
	uint8_t  vol_type;
	uint8_t  copy_flag;
	uint8_t  compat;
	uint32_t vol_id;
	uint32_t lnum;
	uint8_t  padding1[4];
	uint32_t data_size;
	uint32_t used_ebs;
	uint32_t data_pad;
	uint32_t data_crc;
	uint8_t  padding2[4];
	uint64_t sqnum;
	uint8_t  padding3[12];
	uint32_t hdr_crc;
} __packed;

struct ubi_vtbl_record {
	uint32_t reserved_pebs;
	uint32_t alignment;
	uint32_t data_pad;
	uint8_t  vol_type;
	uint8_t  upd_marker;
	uint16_t name_len;
	char     name[UBI_VOL_NAME_MAX+1];
	uint8_t  flags;
	uint8_t  padding[23];
	uint32_t crc;
} __packed;

#define UBI_FM_MAX_START		64
#define UBI_FM_MAX_BLOCKS		32
#define UBI_FM_MAX_POOL_SIZE		256

#define UBI_FM_FMT_VERSION		1

#define UBI_FM_SB_MAGIC			0x7B11D69F
#define UBI_FM_HDR_MAGIC		0xD4B82EF7
#define UBI_FM_VHDR_MAGIC		0xFA370ED1
#define UBI_FM_POOL_MAGIC		0x67AF4D08
#define UBI_FM_EBA_MAGIC		0xF0C040A8

#define UBI_FM_SB_VOLUME_ID		(UBI_LAYOUT_VOLUME_ID + 1)
#define UBI_FM_DATA_VOLUME_ID		(UBI_LAYOUT_VOLUME_ID + 2)

struct ubi_fm_sb {
	uint32_t magic;
	uint8_t  version;
	uint8_t  padding1[3];
	uint32_t data_crc;
	uint32_t used_blocks;
	uint32_t block_loc[UBI_FM_MAX_BLOCKS];
	uint32_t block_ec[UBI_FM_MAX_BLOCKS];
	uint64_t sqnum;
	uint8_t  padding2[32];
} __packed;

struct ubi_fm_hdr {
	uint32_t magic;
	uint32_t free_peb_count;
	uint32_t used_peb_count;
	uint32_t scrub_peb_count;
	uint32_t bad_peb_count;
	uint32_t erase_peb_count;
	uint32_t vol_count;
	uint8_t  padding[4];
} __packed;

struct ubi_fm_scan_pool {
	uint32_t magic;
	uint16_t size;
	uint16_t max_size;
	uint32_t pebs[UBI_FM_MAX_POOL_SIZE];
	uint32_t padding[4];
} __packed;

struct ubi_fm_ec {
	uint32_t pnum;
	uint32_t ec;
} __packed;

struct ubi_fm_volhdr {
	uint32_t magic;
	uint32_t vol_id;
	uint8_t  vol_type;
	uint8_t  padding1[3];
	uint32_t data_pad;
	uint32_t used_ebs;
	uint32_t last_eb_bytes;
	uint8_t  padding2[8];
} __packed;

struct ubi_fm_eba {
	uint32_t magic;
	uint32_t reserved_pebs;
	uint32_t pnum[0];
} __packed;

#endif /* _UBI_MEDIA_H_ */
