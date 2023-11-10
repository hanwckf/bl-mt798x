/* SPDX-License-Identifier: GPL 2.0+ OR BSD-3-Clause */
/*
 * Copyright (c) Thomas Gleixner <tglx@linutronix.de>
 */

#ifndef _UBOOT_MTD_UBISPL_H
#define _UBOOT_MTD_UBISPL_H

#include <platform_def.h>
#include <common/debug.h>
#include <drivers/io/io_ubi.h>
#include "ubi-wrapper.h"
#include "compat.h"

/*
 * Defines the maximum number of logical erase blocks per loadable
 * (static) volume to size the ubispl internal arrays.
 */
#ifndef CONFIG_SPL_UBI_MAX_VOL_LEBS
#define CONFIG_SPL_UBI_MAX_VOL_LEBS	256
#endif /* CONFIG_SPL_UBI_MAX_VOL_LEBS */

/*
 * Defines the maximum physical erase block size to size the fastmap
 * buffer for ubispl.
 */
#ifndef CONFIG_SPL_UBI_MAX_PEB_SIZE
#define CONFIG_SPL_UBI_MAX_PEB_SIZE	(256*1024)
#endif /* CONFIG_SPL_UBI_MAX_PEB_SIZE */

/*
 * Define the maximum number of physical erase blocks to size the
 * ubispl internal arrays.
 */
#ifndef CONFIG_SPL_UBI_MAX_PEBS
#define CONFIG_SPL_UBI_MAX_PEBS		4096
#endif /* CONFIG_SPL_UBI_MAX_PEBS */

/*
 * Defines the maximum number of volumes in which UBISPL is
 * interested. Limits the amount of memory for the scan data and
 * speeds up the scan process as we simply ignore stuff which we dont
 * want to load from the SPL anyway. So the volumes which can be
 * loaded in the above example are ids 0 - 7
 */
#ifndef CONFIG_SPL_UBI_VOL_IDS
#define CONFIG_SPL_UBI_VOL_IDS		16
#endif /* CONFIG_SPL_UBI_VOL_IDS */

/*
 * The maximum number of volume ids we scan. So you can load volume id
 * 0 to (CONFIG_SPL_UBI_VOL_ID_MAX - 1)
 */
#define UBI_SPL_VOL_IDS		CONFIG_SPL_UBI_VOL_IDS
/*
 * The size of the read buffer for the fastmap blocks. In theory up to
 * UBI_FM_MAX_BLOCKS * CONFIG_SPL_MAX_PEB_SIZE. In practice today
 * one or two blocks.
 */
#define UBI_FM_BUF_SIZE		(UBI_FM_MAX_BLOCKS*CONFIG_SPL_UBI_MAX_PEB_SIZE)
/*
 * The size of the bitmaps for the attach/ scan
 */
#define UBI_FM_BM_SIZE		((CONFIG_SPL_UBI_MAX_PEBS / BITS_PER_LONG) + 1)
/*
 * The maximum number of logical erase blocks per loadable volume
 */
#define UBI_MAX_VOL_LEBS	CONFIG_SPL_UBI_MAX_VOL_LEBS
/*
 * The bitmap size for the above to denote the found blocks inside the volume
 */
#define UBI_VOL_BM_SIZE		((UBI_MAX_VOL_LEBS / BITS_PER_LONG) + 1)

/**
 * struct ubi_vol_info - UBISPL internal volume represenation
 * @last_block:		The last block (highest LEB) found for this volume
 * @found:		Bitmap to mark found LEBS
 * @lebs_to_pebs:	LEB to PEB translation table
 */
struct ubi_vol_info {
	uint32_t			last_block;
	unsigned long			found[UBI_VOL_BM_SIZE];
	uint32_t			lebs_to_pebs[UBI_MAX_VOL_LEBS];
};

/**
 * struct ubi_vol_size - UBISPL internal volume size cache
 * @size:		The size of this volume
 * @valid:		Whether the cached size is valid or not
 */
struct ubi_vol_size {
	uint32_t size;
	int valid;
};

/**
 * struct ubi_leb_cache_info - UBISPL internal volume LEB cache
 * @vol_id:		The volume Id of the cache
 * @leb:		The LEB of the cached LEB
 * @len:		The valid data length of the cached LEB
 * @crc:		The data crc of the cached LEB
 * @valid:		Whether the cache is valid or not
 */
struct ubi_leb_cache_info {
	uint32_t vol_id;
	uint32_t leb;
	uint32_t len;
	uint32_t crc;
	int valid;
};

/**
 * struct ubi_scan_info - UBISPL internal data for FM attach and full scan
 *
 * @read:		Read function to access the flash provided by the caller
 * @peb_count:		Number of physical erase blocks in the UBI FLASH area
 *			aka MTD partition.
 * @peb_offset:		Offset of PEB0 in the UBI FLASH area (aka MTD partition)
 *			to the real start of the FLASH in erase blocks.
 * @fsize_mb:		Size of the scanned FLASH area in MB (stats only)
 * @vid_offset:		Offset from the start of a PEB to the VID header
 * @leb_start:		Offset from the start of a PEB to the data area
 * @leb_size:		Size of the data area
 *
 * @fastmap_pebs:	Counter of PEBs "attached" by fastmap
 * @fastmap_anchor:	The anchor PEB of the fastmap
 * @fm_sb:		The fastmap super block data
 * @fm_vh:		The fastmap VID header
 * @fm:			Pointer to the fastmap layout
 * @fm_layout:		The fastmap layout itself
 * @fm_pool:		The pool of PEBs to scan at fastmap attach time
 * @fm_wl_pool:		The pool of PEBs scheduled for wearleveling
 *
 * @fm_enabled:		Indicator whether fastmap attachment is enabled.
 * @fm_used:		Bitmap to indicate the PEBS covered by fastmap
 * @scanned:		Bitmap to indicate the PEBS of which the VID header
 *			hase been physically scanned.
 * @corrupt:		Bitmap to indicate corrupt blocks
 * @toload:		Bitmap to indicate the volumes which should be loaded
 *
 * @blockinfo:		The vid headers of the scanned blocks
 * @volinfo:		The volume information of the interesting (toload)
 *			volumes
 * @vtbl_corrupted:	Flag to indicate status of volume table
 * @vtbl:		Volume table
 *
 * @sizes:		Cached volume size
 * @leb_cache_info:	Volume LED cache information
 *
 * @fm_buf:		The large fastmap attach buffer
 *
 * @leb_cache:		Volume LEB cache data
 */
struct ubi_scan_info {
	ubi_is_bad_block		is_bad_peb;
	ubi_read_flash			read;
	unsigned int			fsize_mb;
	unsigned int			peb_count;
	unsigned int			peb_offset;

	unsigned long			vid_offset;
	unsigned long			leb_start;
	unsigned long			leb_size;

	/* Fastmap: The upstream required fields */
	int				fastmap_pebs;
	int				fastmap_anchor;
	size_t				fm_size;
	struct ubi_fm_sb		fm_sb;
	struct ubi_vid_hdr		fm_vh;
	struct ubi_fastmap_layout	*fm;
	struct ubi_fastmap_layout	fm_layout;
	struct ubi_fm_pool		fm_pool;
	struct ubi_fm_pool		fm_wl_pool;

	/* Fastmap: UBISPL specific data */
	int				fm_enabled;
	unsigned long			fm_used[UBI_FM_BM_SIZE];
	unsigned long			scanned[UBI_FM_BM_SIZE];
	unsigned long			corrupt[UBI_FM_BM_SIZE];

	/* Data for storing the VID and volume information */
	struct ubi_vol_info		volinfo[UBI_SPL_VOL_IDS];
	struct ubi_vid_hdr		blockinfo[CONFIG_SPL_UBI_MAX_PEBS];

	/* Volume table */
	int                             vtbl_valid;
	struct ubi_vtbl_record          vtbl[UBI_SPL_VOL_IDS];

	/* Volume size cache */
	struct ubi_vol_size		sizes[UBI_SPL_VOL_IDS];

	/* Volume LEB cache info */
	struct ubi_leb_cache_info	leb_cache_info;

	/* The large buffer for the fastmap */
	uint8_t				fm_buf[UBI_FM_BUF_SIZE];

	/* Volume LEB cache data */
	uint8_t				leb_cache[CONFIG_SPL_UBI_MAX_PEB_SIZE];
};

#define ubi_dbg(fmt, ...) VERBOSE("UBI: " fmt "\n", ##__VA_ARGS__)
#define ubi_msg(fmt, ...) NOTICE("UBI: " fmt "\n", ##__VA_ARGS__)
#define ubi_warn(fmt, ...) WARN("UBI warning: " fmt "\n", ##__VA_ARGS__)
#define ubi_err(fmt, ...) ERROR("UBI error: " fmt "\n", ##__VA_ARGS__)

void ubispl_init_scan(struct io_ubi_dev_spec *info, int fastmap);
int ubispl_get_volume_data_size(struct io_ubi_dev_spec *info, int vol_id,
		  		const char *vol_name);
int ubispl_load_volume(struct io_ubi_dev_spec *info, int vol_id,
		       const char *vol_name, void *buf, uint32_t offset,
		       uint32_t len, uint32_t *retlen);

#endif
