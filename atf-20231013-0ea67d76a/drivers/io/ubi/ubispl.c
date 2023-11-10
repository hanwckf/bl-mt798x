// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright (c) Thomas Gleixner <tglx@linutronix.de>
 *
 * The parts taken from the kernel implementation are:
 *
 * Copyright (c) International Business Machines Corp., 2006
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <endian.h>
#include <inttypes.h>

#include "ubispl.h"

/**
 * ubi_calc_fm_size - calculates the fastmap size in bytes for an UBI device.
 * @ubi: UBI device description object
 */
static size_t ubi_calc_fm_size(struct ubi_scan_info *ubi)
{
	size_t size;

	size = sizeof(struct ubi_fm_sb) +
		sizeof(struct ubi_fm_hdr) +
		sizeof(struct ubi_fm_scan_pool) +
		sizeof(struct ubi_fm_scan_pool) +
		(ubi->peb_count * sizeof(struct ubi_fm_ec)) +
		(sizeof(struct ubi_fm_eba) +
		(ubi->peb_count * sizeof(uint32_t))) +
		sizeof(struct ubi_fm_volhdr) * UBI_MAX_VOLUMES;
	return roundup(size, ubi->leb_size);
}

static int ubi_is_bad_peb(struct ubi_scan_info *ubi, uint32_t pnum)
{
	return ubi->is_bad_peb(pnum + ubi->peb_offset);
}

static int ubi_io_read(struct ubi_scan_info *ubi, void *buf, uint32_t pnum,
		       unsigned long from, unsigned long len)
{
	return ubi->read(pnum + ubi->peb_offset, from, len, buf);
}

static int ubi_io_is_bad(struct ubi_scan_info *ubi, int peb)
{
	return peb >= ubi->peb_count || peb < 0;
}

/**
 * ubi_dump_vtbl_record - dump a &struct ubi_vtbl_record object.
 * @r: the object to dump
 * @idx: volume table index
 */
void ubi_dump_vtbl_record(const struct ubi_vtbl_record *r, int idx)
{
	int name_len = be16toh(r->name_len);

	ubi_dbg("Volume table record %d dump: size: %" PRIuPTR,
		idx, sizeof(struct ubi_vtbl_record));
	ubi_dbg("\treserved_pebs   %d", be32toh(r->reserved_pebs));
	ubi_dbg("\talignment       %d", be32toh(r->alignment));
	ubi_dbg("\tdata_pad        %d", be32toh(r->data_pad));
	ubi_dbg("\tvol_type        %d", (int)r->vol_type);
	ubi_dbg("\tupd_marker      %d", (int)r->upd_marker);
	ubi_dbg("\tname_len        %d", name_len);

	if (r->name[0] == '\0') {
		ubi_dbg("\tname            NULL");
		return;
	}

	if (name_len <= UBI_VOL_NAME_MAX &&
	    strnlen(&r->name[0], name_len + 1) == name_len) {
		ubi_dbg("\tname            %s", &r->name[0]);
	} else {
		ubi_dbg("\t1st 5 characters of name: %c%c%c%c%c",
			r->name[0], r->name[1], r->name[2], r->name[3],
			r->name[4]);
	}
	ubi_dbg("\tcrc             %#08x", be32toh(r->crc));
}

/* Empty volume table record */
static struct ubi_vtbl_record empty_vtbl_record;

/**
 * vtbl_check - check if volume table is not corrupted and sensible.
 * @ubi: UBI device description object
 * @vtbl: volume table
 *
 * This function returns zero if @vtbl is all right, %1 if CRC is incorrect,
 * and %-EINVAL if it contains inconsistent data.
 */
static int vtbl_check(struct ubi_scan_info *ubi,
		      struct ubi_vtbl_record *vtbl)
{
	int i, n, reserved_pebs, alignment, data_pad, vol_type, name_len;
	int upd_marker, err;
	uint32_t crc;
	const char *name;

	for (i = 0; i < UBI_SPL_VOL_IDS; i++) {
		reserved_pebs = be32toh(vtbl[i].reserved_pebs);
		alignment = be32toh(vtbl[i].alignment);
		data_pad = be32toh(vtbl[i].data_pad);
		upd_marker = vtbl[i].upd_marker;
		vol_type = vtbl[i].vol_type;
		name_len = be16toh(vtbl[i].name_len);
		name = &vtbl[i].name[0];

		crc = ubi_crc32(UBI_CRC32_INIT, &vtbl[i], UBI_VTBL_RECORD_SIZE_CRC);
		if (be32toh(vtbl[i].crc) != crc) {
			ubi_err("bad CRC at record %u: %#08x, not %#08x",
				i, crc, be32toh(vtbl[i].crc));
			ubi_dump_vtbl_record(&vtbl[i], i);
			return 1;
		}

		if (reserved_pebs == 0) {
			if (memcmp(&vtbl[i], &empty_vtbl_record,
				   UBI_VTBL_RECORD_SIZE)) {
				err = 2;
				goto bad;
			}
			continue;
		}

		if (reserved_pebs < 0 || alignment < 0 || data_pad < 0 ||
		    name_len < 0) {
			err = 3;
			goto bad;
		}

		if (alignment > ubi->leb_size || alignment == 0) {
			err = 4;
			goto bad;
		}

		n = alignment & (ubi->vid_offset - 1);
		if (alignment != 1 && n) {
			err = 5;
			goto bad;
		}

		n = ubi->leb_size % alignment;
		if (data_pad != n) {
			ubi_err("bad data_pad, has to be %d", n);
			err = 6;
			goto bad;
		}

		if (vol_type != UBI_VID_DYNAMIC && vol_type != UBI_VID_STATIC) {
			err = 7;
			goto bad;
		}

		if (upd_marker != 0 && upd_marker != 1) {
			err = 8;
			goto bad;
		}

		if (name_len > UBI_VOL_NAME_MAX) {
			err = 10;
			goto bad;
		}

		if (name[0] == '\0') {
			err = 11;
			goto bad;
		}

		if (name_len != strnlen(name, name_len + 1)) {
			err = 12;
			goto bad;
		}

		ubi_dump_vtbl_record(&vtbl[i], i);
	}

	/* Checks that all names are unique */
	for (i = 0; i < UBI_SPL_VOL_IDS - 1; i++) {
		for (n = i + 1; n < UBI_SPL_VOL_IDS; n++) {
			int len1 = be16toh(vtbl[i].name_len);
			int len2 = be16toh(vtbl[n].name_len);

			if (len1 > 0 && len1 == len2 &&
			    !strncmp(vtbl[i].name, vtbl[n].name, len1)) {
				ubi_err("volumes %d and %d have the same name \"%s\"",
					i, n, vtbl[i].name);
				ubi_dump_vtbl_record(&vtbl[i], i);
				ubi_dump_vtbl_record(&vtbl[n], n);
				return -EINVAL;
			}
		}
	}

	return 0;

bad:
	ubi_err("volume table check failed: record %d, error %d", i, err);
	ubi_dump_vtbl_record(&vtbl[i], i);
	return -EINVAL;
}

static int ubi_read_volume_table(struct ubi_scan_info *ubi, uint32_t pnum)
{
	int err = -EINVAL;

	empty_vtbl_record.crc = htobe32(0xf116c36b);

	err = ubi_io_read(ubi, &ubi->vtbl, pnum, ubi->leb_start,
			  sizeof(struct ubi_vtbl_record) * UBI_SPL_VOL_IDS);
	if (err && err != UBI_IO_BITFLIPS) {
		ubi_err("unable to read volume table");
		goto out;
	}

	if (!vtbl_check(ubi, ubi->vtbl)) {
		ubi->vtbl_valid = 1;
		err = 0;
	}
out:
	return err;
}

static int ubi_check_ec_hdr(struct ubi_scan_info *ubi, int pnum)
{
	struct ubi_ec_hdr eh;
	uint32_t magic;
	int res;

	/* Check EC header */
	res = ubi_io_read(ubi, &eh, pnum, 0, sizeof(eh));
	if (res) {
		ubi_dbg("Unable to read block %d", pnum);
		goto out;
	}

	/* Magic number available ? */
	magic = be32toh(eh.magic);
	if (magic != UBI_EC_HDR_MAGIC) {
		ubi_msg("Bad EC magic in block %d %08x", pnum, magic);
		goto out;
	}

	/* Header CRC correct ? */
	if (ubi_crc32(UBI_CRC32_INIT, &eh, UBI_EC_HDR_SIZE_CRC) !=
	    be32toh(eh.hdr_crc)) {
		ubi_msg("Bad EC CRC in block %d", pnum);
		goto out;
	}

	if (eh.padding1[0] == 'E' && eh.padding1[1] == 'O' &&
	    eh.padding1[2] == 'F') {
		ubi_msg("EOF marker found in block %d", pnum);
		return UBI_PEB_EOF;
	}

out:
	return UBI_IO_FF;
}

static int ubi_io_read_vid_hdr(struct ubi_scan_info *ubi, int pnum,
			       struct ubi_vid_hdr *vh, int unused)
{
	uint32_t magic;
	int res;

	/* No point in rescanning a corrupt block */
	if (test_bit(pnum, ubi->corrupt))
		return UBI_IO_BAD_HDR;
	/*
	 * If the block has been scanned already, no need to rescan
	 */
	if (test_and_set_bit(pnum, ubi->scanned))
		return 0;

	/* Check bad PEB first */
	res = ubi_is_bad_peb(ubi, pnum);
	if (res)
		goto mark_bad_peb;

	res = ubi_io_read(ubi, vh, pnum, ubi->vid_offset, sizeof(*vh));

	/*
	 * Bad block, unrecoverable ECC error, skip the block
	 */
	if (res) {
	mark_bad_peb:
		ubi_dbg("Skipping bad or unreadable block %d", pnum);
		vh->magic = 0;
		generic_set_bit(pnum, ubi->corrupt);
		return res;
	}

	/* Magic number available ? */
	magic = be32toh(vh->magic);
	if (magic != UBI_VID_HDR_MAGIC) {
		generic_set_bit(pnum, ubi->corrupt);
		if (magic == 0xffffffff)
			return ubi_check_ec_hdr(ubi, pnum);
		ubi_msg("Bad VID magic in block %d %08x", pnum, magic);
		return UBI_IO_BAD_HDR;
	}

	/* Header CRC correct ? */
	if (ubi_crc32(UBI_CRC32_INIT, vh, UBI_VID_HDR_SIZE_CRC) !=
	    be32toh(vh->hdr_crc)) {
		ubi_msg("Bad VID CRC in block %d", pnum);
		generic_set_bit(pnum, ubi->corrupt);
		return UBI_IO_BAD_HDR;
	}

	ubi_dbg("RV: pnum: %i sqnum %" PRId64 "u", pnum, be64toh(vh->sqnum));

	return 0;
}

static int ubi_rescan_fm_vid_hdr(struct ubi_scan_info *ubi,
				 struct ubi_vid_hdr *vh,
				 uint32_t fm_pnum, uint32_t fm_vol_id, uint32_t fm_lnum)
{
	int res;

	if (ubi_io_is_bad(ubi, fm_pnum))
		return -EINVAL;

	res = ubi_io_read_vid_hdr(ubi, fm_pnum, vh, 0);
	if (!res) {
		/* Check volume id, volume type and lnum */
		if (be32toh(vh->vol_id) == fm_vol_id &&
		    vh->vol_type == UBI_VID_STATIC &&
		    be32toh(vh->lnum) == fm_lnum)
			return 0;
		ubi_dbg("RS: PEB %u vol: %u : %u typ %u lnum %u %u",
			fm_pnum, fm_vol_id, vh->vol_type,
			be32toh(vh->vol_id),
			fm_lnum, be32toh(vh->lnum));
	}
	return res;
}

/* Insert the logic block into the volume info */
static int ubi_add_peb_to_vol(struct ubi_scan_info *ubi,
			      struct ubi_vid_hdr *vh, uint32_t vol_id,
			      uint32_t pnum, uint32_t lnum)
{
	struct ubi_vol_info *vi = ubi->volinfo + vol_id;
	uint32_t *ltp;

	/*
	 * If the volume is larger than expected, yell and give up :(
	 */
	if (lnum >= UBI_MAX_VOL_LEBS) {
		ubi_warn("Vol: %u LEB %d > %d", vol_id, lnum, UBI_MAX_VOL_LEBS);
		return -EINVAL;
	}

	ubi_dbg("SC: Add PEB %u to Vol %u as LEB %u fnd %d sc %d",
		pnum, vol_id, lnum, !!test_bit(lnum, vi->found),
		!!test_bit(pnum, ubi->scanned));

	/* Points to the translation entry */
	ltp = vi->lebs_to_pebs + lnum;

	/* If the block is already assigned, check sqnum */
	if (test_and_set_bit(lnum, vi->found)) {
		uint32_t cur_pnum = *ltp;
		struct ubi_vid_hdr *cur = ubi->blockinfo + cur_pnum;

		/*
		 * If the current block hase not yet been scanned, we
		 * need to do that. The other block might be stale or
		 * the current block corrupted and the FM not yet
		 * updated.
		 */
		if (!test_bit(cur_pnum, ubi->scanned)) {
			/*
			 * If the scan fails, we use the valid block
			 */
			if (ubi_rescan_fm_vid_hdr(ubi, cur, cur_pnum, vol_id,
						  lnum)) {
				*ltp = pnum;
				return 0;
			}
		}

		/*
		 * Should not happen ....
		 */
		if (test_bit(cur_pnum, ubi->corrupt)) {
			*ltp = pnum;
			return 0;
		}

		ubi_dbg("Vol %u LEB %u PEB %u->sqnum %" PRIu64 " NPEB %u->sqnum %" PRIu64,
			vol_id, lnum, cur_pnum, be64toh(cur->sqnum), pnum,
			be64toh(vh->sqnum));

		/*
		 * Compare sqnum and take the newer one
		 */
		if (be64toh(cur->sqnum) < be64toh(vh->sqnum))
			*ltp = pnum;
	} else {
		*ltp = pnum;
		if (lnum > vi->last_block)
			vi->last_block = lnum;
	}

	return 0;
}

static int ubi_scan_vid_hdr(struct ubi_scan_info *ubi, struct ubi_vid_hdr *vh,
			    uint32_t pnum)
{
	uint32_t vol_id, lnum;
	int res;

	if (ubi_io_is_bad(ubi, pnum))
		return -EINVAL;

	res = ubi_io_read_vid_hdr(ubi, pnum, vh, 0);
	if (res)
		return res;

	/* Get volume id */
	vol_id = be32toh(vh->vol_id);

	/* If this is the fastmap anchor, return right away */
	if (vol_id == UBI_FM_SB_VOLUME_ID)
		return ubi->fm_enabled ? UBI_FASTMAP_ANCHOR : 0;

	/* If this is a UBI volume table, read it and return */
	if (vol_id == UBI_LAYOUT_VOLUME_ID && !ubi->vtbl_valid) {
		res = ubi_read_volume_table(ubi, pnum);
		return res;
	}

	/* We only care about static volumes with an id < UBI_SPL_VOL_IDS */
	if (vol_id >= UBI_SPL_VOL_IDS || vh->vol_type != UBI_VID_STATIC)
		return 0;

	lnum = be32toh(vh->lnum);
	return ubi_add_peb_to_vol(ubi, vh, vol_id, pnum, lnum);
}

static int assign_aeb_to_av(struct ubi_scan_info *ubi, uint32_t pnum, uint32_t lnum,
			     uint32_t vol_id, uint32_t vol_type, uint32_t used)
{
	struct ubi_vid_hdr *vh;

	if (ubi_io_is_bad(ubi, pnum))
		return -EINVAL;

	ubi->fastmap_pebs++;

	vh = ubi->blockinfo + pnum;

	return ubi_scan_vid_hdr(ubi, vh, pnum);
}

static int scan_pool(struct ubi_scan_info *ubi, /* __be32 */ uint32_t *pebs, int pool_size)
{
	struct ubi_vid_hdr *vh;
	uint32_t pnum;
	int i;

	ubi_dbg("Scanning pool size: %d", pool_size);

	for (i = 0; i < pool_size; i++) {
		pnum = be32toh(pebs[i]);

		if (ubi_io_is_bad(ubi, pnum)) {
			ubi_err("FM: Bad PEB in fastmap pool! %u", pnum);
			return UBI_BAD_FASTMAP;
		}

		vh = ubi->blockinfo + pnum;
		/*
		 * We allow the scan to fail here. The loader will notice
		 * and look for a replacement.
		 */
		ubi_scan_vid_hdr(ubi, vh, pnum);
	}
	return 0;
}

/*
 * Fastmap code is stolen from Linux kernel and this stub structure is used
 * to make it happy.
 */
struct ubi_attach_info {
	int i;
};

static int ubi_attach_fastmap(struct ubi_scan_info *ubi,
			      struct ubi_attach_info *ai,
			      struct ubi_fastmap_layout *fm)
{
	struct ubi_fm_hdr *fmhdr;
	struct ubi_fm_scan_pool *fmpl1, *fmpl2;
	struct ubi_fm_ec *fmec;
	struct ubi_fm_volhdr *fmvhdr;
	struct ubi_fm_eba *fm_eba;
	int ret, i, j, pool_size, wl_pool_size;
	size_t fm_pos = 0, fm_size = ubi->fm_size;
	void *fm_raw = ubi->fm_buf;

	memset(ubi->fm_used, 0, sizeof(ubi->fm_used));

	fm_pos += sizeof(struct ubi_fm_sb);
	if (fm_pos >= fm_size)
		goto fail_bad;

	fmhdr = (struct ubi_fm_hdr *)(fm_raw + fm_pos);
	fm_pos += sizeof(*fmhdr);
	if (fm_pos >= fm_size)
		goto fail_bad;

	if (be32toh(fmhdr->magic) != UBI_FM_HDR_MAGIC) {
		ubi_err("bad fastmap header magic: 0x%x, expected: 0x%x",
			be32toh(fmhdr->magic), UBI_FM_HDR_MAGIC);
		goto fail_bad;
	}

	fmpl1 = (struct ubi_fm_scan_pool *)(fm_raw + fm_pos);
	fm_pos += sizeof(*fmpl1);
	if (fm_pos >= fm_size)
		goto fail_bad;
	if (be32toh(fmpl1->magic) != UBI_FM_POOL_MAGIC) {
		ubi_err("bad fastmap pool magic: 0x%x, expected: 0x%x",
			be32toh(fmpl1->magic), UBI_FM_POOL_MAGIC);
		goto fail_bad;
	}

	fmpl2 = (struct ubi_fm_scan_pool *)(fm_raw + fm_pos);
	fm_pos += sizeof(*fmpl2);
	if (fm_pos >= fm_size)
		goto fail_bad;
	if (be32toh(fmpl2->magic) != UBI_FM_POOL_MAGIC) {
		ubi_err("bad fastmap pool magic: 0x%x, expected: 0x%x",
			be32toh(fmpl2->magic), UBI_FM_POOL_MAGIC);
		goto fail_bad;
	}

	pool_size = be16toh(fmpl1->size);
	wl_pool_size = be16toh(fmpl2->size);
	fm->max_pool_size = be16toh(fmpl1->max_size);
	fm->max_wl_pool_size = be16toh(fmpl2->max_size);

	if (pool_size > UBI_FM_MAX_POOL_SIZE || pool_size < 0) {
		ubi_err("bad pool size: %i", pool_size);
		goto fail_bad;
	}

	if (wl_pool_size > UBI_FM_MAX_POOL_SIZE || wl_pool_size < 0) {
		ubi_err("bad WL pool size: %i", wl_pool_size);
		goto fail_bad;
	}

	if (fm->max_pool_size > UBI_FM_MAX_POOL_SIZE ||
	    fm->max_pool_size < 0) {
		ubi_err("bad maximal pool size: %i", fm->max_pool_size);
		goto fail_bad;
	}

	if (fm->max_wl_pool_size > UBI_FM_MAX_POOL_SIZE ||
	    fm->max_wl_pool_size < 0) {
		ubi_err("bad maximal WL pool size: %i", fm->max_wl_pool_size);
		goto fail_bad;
	}

	/* read EC values from free list */
	for (i = 0; i < be32toh(fmhdr->free_peb_count); i++) {
		fmec = (struct ubi_fm_ec *)(fm_raw + fm_pos);
		fm_pos += sizeof(*fmec);
		if (fm_pos >= fm_size)
			goto fail_bad;
	}

	/* read EC values from used list */
	for (i = 0; i < be32toh(fmhdr->used_peb_count); i++) {
		fmec = (struct ubi_fm_ec *)(fm_raw + fm_pos);
		fm_pos += sizeof(*fmec);
		if (fm_pos >= fm_size)
			goto fail_bad;

		generic_set_bit(be32toh(fmec->pnum), ubi->fm_used);
	}

	/* read EC values from scrub list */
	for (i = 0; i < be32toh(fmhdr->scrub_peb_count); i++) {
		fmec = (struct ubi_fm_ec *)(fm_raw + fm_pos);
		fm_pos += sizeof(*fmec);
		if (fm_pos >= fm_size)
			goto fail_bad;
	}

	/* read EC values from erase list */
	for (i = 0; i < be32toh(fmhdr->erase_peb_count); i++) {
		fmec = (struct ubi_fm_ec *)(fm_raw + fm_pos);
		fm_pos += sizeof(*fmec);
		if (fm_pos >= fm_size)
			goto fail_bad;
	}

	/* Iterate over all volumes and read their EBA table */
	for (i = 0; i < be32toh(fmhdr->vol_count); i++) {
		uint32_t vol_id, vol_type, used, reserved;

		fmvhdr = (struct ubi_fm_volhdr *)(fm_raw + fm_pos);
		fm_pos += sizeof(*fmvhdr);
		if (fm_pos >= fm_size)
			goto fail_bad;

		if (be32toh(fmvhdr->magic) != UBI_FM_VHDR_MAGIC) {
			ubi_err("bad fastmap vol header magic: 0x%x, " \
				"expected: 0x%x",
				be32toh(fmvhdr->magic), UBI_FM_VHDR_MAGIC);
			goto fail_bad;
		}

		vol_id = be32toh(fmvhdr->vol_id);
		vol_type = fmvhdr->vol_type;
		used = be32toh(fmvhdr->used_ebs);

		fm_eba = (struct ubi_fm_eba *)(fm_raw + fm_pos);
		fm_pos += sizeof(*fm_eba);
		fm_pos += (sizeof(uint32_t) * be32toh(fm_eba->reserved_pebs));
		if (fm_pos >= fm_size)
			goto fail_bad;

		if (be32toh(fm_eba->magic) != UBI_FM_EBA_MAGIC) {
			ubi_err("bad fastmap EBA header magic: 0x%x, " \
				"expected: 0x%x",
				be32toh(fm_eba->magic), UBI_FM_EBA_MAGIC);
			goto fail_bad;
		}

		reserved = be32toh(fm_eba->reserved_pebs);
		ubi_dbg("FA: vol %u used %u res: %u", vol_id, used, reserved);
		for (j = 0; j < reserved; j++) {
			int pnum = be32toh(fm_eba->pnum[j]);

			if ((int)be32toh(fm_eba->pnum[j]) < 0)
				continue;

			if (!test_and_clear_bit(pnum, ubi->fm_used))
				continue;

			/*
			 * We only handle static volumes so used_ebs
			 * needs to be handed in. And we do not assign
			 * the reserved blocks
			 */
			if (j >= used)
				continue;

			ret = assign_aeb_to_av(ubi, pnum, j, vol_id,
					       vol_type, used);
			if (!ret)
				continue;

			/*
			 * Nasty: The fastmap claims that the volume
			 * has one block more than it, but that block
			 * is always empty and the other blocks have
			 * the correct number of total LEBs in the
			 * headers. Deal with it.
			 */
			if (ret != UBI_IO_FF && j != used - 1)
				goto fail_bad;
			ubi_dbg("FA: Vol: %u Ignoring empty LEB %d of %d",
				vol_id, j, used);
		}
	}

#if (ARM_ARCH_MAJOR > 7)
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#endif

	ret = scan_pool(ubi, (void *)fmpl1->pebs, pool_size);
	if (ret)
		goto fail;

	ret = scan_pool(ubi, (void *)fmpl2->pebs, wl_pool_size);
	if (ret)
		goto fail;

#ifdef CHECKME
	/*
	 * If fastmap is leaking PEBs (must not happen), raise a
	 * fat warning and fall back to scanning mode.
	 * We do this here because in ubi_wl_init() it's too late
	 * and we cannot fall back to scanning.
	 */
	if (WARN_ON(count_fastmap_pebs(ai) != ubi->peb_count -
		    ai->bad_peb_count - fm->used_blocks))
		goto fail_bad;
#endif

	return 0;

fail_bad:
	ret = UBI_BAD_FASTMAP;
fail:
	return ret;
}

static int ubi_scan_fastmap(struct ubi_scan_info *ubi,
			    struct ubi_attach_info *ai,
			    int fm_anchor)
{
	struct ubi_fm_sb *fmsb, *fmsb2;
	struct ubi_vid_hdr *vh;
	struct ubi_fastmap_layout *fm;
	int i, used_blocks, pnum, ret = 0;
	size_t fm_size;
	uint32_t crc, tmp_crc;
	unsigned long long sqnum = 0;

	fmsb = &ubi->fm_sb;
	fm = &ubi->fm_layout;

	ret = ubi_io_read(ubi, fmsb, fm_anchor, ubi->leb_start, sizeof(*fmsb));
	if (ret && ret != UBI_IO_BITFLIPS)
		goto free_fm_sb;
	else if (ret == UBI_IO_BITFLIPS)
		fm->to_be_tortured[0] = 1;

	if (be32toh(fmsb->magic) != UBI_FM_SB_MAGIC) {
		ubi_err("bad super block magic: 0x%x, expected: 0x%x",
			be32toh(fmsb->magic), UBI_FM_SB_MAGIC);
		ret = UBI_BAD_FASTMAP;
		goto free_fm_sb;
	}

	if (fmsb->version != UBI_FM_FMT_VERSION) {
		ubi_err("bad fastmap version: %i, expected: %i",
			fmsb->version, UBI_FM_FMT_VERSION);
		ret = UBI_BAD_FASTMAP;
		goto free_fm_sb;
	}

	used_blocks = be32toh(fmsb->used_blocks);
	if (used_blocks > UBI_FM_MAX_BLOCKS || used_blocks < 1) {
		ubi_err("number of fastmap blocks is invalid: %i", used_blocks);
		ret = UBI_BAD_FASTMAP;
		goto free_fm_sb;
	}

	fm_size = ubi->leb_size * used_blocks;
	if (fm_size != ubi->fm_size) {
		ubi_err("bad fastmap size: %zi, expected: %zi", fm_size,
			ubi->fm_size);
		ret = UBI_BAD_FASTMAP;
		goto free_fm_sb;
	}

	vh = &ubi->fm_vh;

	for (i = 0; i < used_blocks; i++) {
		pnum = be32toh(fmsb->block_loc[i]);

		if (ubi_io_is_bad(ubi, pnum)) {
			ret = UBI_BAD_FASTMAP;
			goto free_hdr;
		}

#ifdef LATER
		int image_seq;
		ret = ubi_io_read_ec_hdr(ubi, pnum, ech, 0);
		if (ret && ret != UBI_IO_BITFLIPS) {
			ubi_err("unable to read fastmap block# %i EC (PEB: %i)",
				i, pnum);
			if (ret > 0)
				ret = UBI_BAD_FASTMAP;
			goto free_hdr;
		} else if (ret == UBI_IO_BITFLIPS)
			fm->to_be_tortured[i] = 1;

		image_seq = be32toh(ech->image_seq);
		if (!ubi->image_seq)
			ubi->image_seq = image_seq;
		/*
		 * Older UBI implementations have image_seq set to zero, so
		 * we shouldn't fail if image_seq == 0.
		 */
		if (image_seq && (image_seq != ubi->image_seq)) {
			ubi_err("wrong image seq:%d instead of %d",
				be32toh(ech->image_seq), ubi->image_seq);
			ret = UBI_BAD_FASTMAP;
			goto free_hdr;
		}
#endif
		ret = ubi_io_read_vid_hdr(ubi, pnum, vh, 0);
		if (ret && ret != UBI_IO_BITFLIPS) {
			ubi_err("unable to read fastmap block# %i (PEB: %i)",
				i, pnum);
			goto free_hdr;
		}

		/*
		 * Mainline code rescans the anchor header. We've done
		 * that already so we merily copy it over.
		 */
		if (pnum == fm_anchor)
			memcpy(vh, ubi->blockinfo + pnum, sizeof(*fm));

		if (i == 0) {
			if (be32toh(vh->vol_id) != UBI_FM_SB_VOLUME_ID) {
				ubi_err("bad fastmap anchor vol_id: 0x%x," \
					" expected: 0x%x",
					be32toh(vh->vol_id),
					UBI_FM_SB_VOLUME_ID);
				ret = UBI_BAD_FASTMAP;
				goto free_hdr;
			}
		} else {
			if (be32toh(vh->vol_id) != UBI_FM_DATA_VOLUME_ID) {
				ubi_err("bad fastmap data vol_id: 0x%x," \
					" expected: 0x%x",
					be32toh(vh->vol_id),
					UBI_FM_DATA_VOLUME_ID);
				ret = UBI_BAD_FASTMAP;
				goto free_hdr;
			}
		}

		if (sqnum < be64toh(vh->sqnum))
			sqnum = be64toh(vh->sqnum);

		ret = ubi_io_read(ubi, ubi->fm_buf + (ubi->leb_size * i), pnum,
				  ubi->leb_start, ubi->leb_size);
		if (ret && ret != UBI_IO_BITFLIPS) {
			ubi_err("unable to read fastmap block# %i (PEB: %i, " \
				"err: %i)", i, pnum, ret);
			goto free_hdr;
		}
	}

	fmsb2 = (struct ubi_fm_sb *)(ubi->fm_buf);
	tmp_crc = be32toh(fmsb2->data_crc);
	fmsb2->data_crc = 0;
	crc = ubi_crc32(UBI_CRC32_INIT, ubi->fm_buf, fm_size);
	if (crc != tmp_crc) {
		ubi_err("fastmap data CRC is invalid");
		ubi_err("CRC should be: 0x%x, calc: 0x%x", tmp_crc, crc);
		ret = UBI_BAD_FASTMAP;
		goto free_hdr;
	}

	fmsb2->sqnum = sqnum;

	fm->used_blocks = used_blocks;

	ret = ubi_attach_fastmap(ubi, ai, fm);
	if (ret) {
		if (ret > 0)
			ret = UBI_BAD_FASTMAP;
		goto free_hdr;
	}

	ubi->fm = fm;
	ubi->fm_pool.max_size = ubi->fm->max_pool_size;
	ubi->fm_wl_pool.max_size = ubi->fm->max_wl_pool_size;
	ubi_msg("attached by fastmap %uMB %u blocks",
		ubi->fsize_mb, ubi->peb_count);
	ubi_dbg("fastmap pool size: %d", ubi->fm_pool.max_size);
	ubi_dbg("fastmap WL pool size: %d", ubi->fm_wl_pool.max_size);

out:
	if (ret)
		ubi_err("Attach by fastmap failed, doing a full scan!");
	return ret;

free_hdr:
free_fm_sb:
	goto out;
}

/*
 * Scan the flash and attempt to attach via fastmap
 */
static void ipl_scan(struct ubi_scan_info *ubi)
{
	unsigned int pnum;
	int res;

	/*
	 * Scan first for the fastmap super block
	 */
	for (pnum = 0; pnum < UBI_FM_MAX_START; pnum++) {
		res = ubi_scan_vid_hdr(ubi, ubi->blockinfo + pnum, pnum);
		if (res == UBI_PEB_EOF) {
			ubi->peb_count = pnum;
			break;
		}

		/*
		 * We ignore errors here as we are meriliy scanning
		 * the headers.
		 */
		if (res != UBI_FASTMAP_ANCHOR)
			continue;

		/*
		 * If fastmap is disabled, continue scanning. This
		 * might happen because the previous attempt failed or
		 * the caller disabled it right away.
		 */
		if (!ubi->fm_enabled)
			continue;

		/*
		 * Try to attach the fastmap, if that fails continue
		 * scanning.
		 */
		if (!ubi_scan_fastmap(ubi, NULL, pnum))
			return;
		/*
		 * Fastmap failed. Clear everything we have and start
		 * over. We are paranoid and do not trust anything.
		 */
		memset(ubi->volinfo, 0, sizeof(ubi->volinfo));
		pnum = 0;
		break;
	}

	/*
	 * Continue scanning, ignore errors, we might find what we are
	 * looking for,
	 */
	for (; pnum < ubi->peb_count; pnum++) {
		res = ubi_scan_vid_hdr(ubi, ubi->blockinfo + pnum, pnum);
		if (res == UBI_PEB_EOF) {
			ubi->peb_count = pnum;
			break;
		}
	}
}

/*
 * Load a logical block of a volume into memory
 */
static int ubi_load_block(struct ubi_scan_info *ubi, uint8_t *laddr,
			  struct ubi_vol_info *vi, uint32_t vol_id,
			  uint32_t lnum, uint32_t last, uint32_t skiplen,
			  uint32_t readlen, uint32_t *retlen)
{
	struct ubi_vid_hdr *vh, *vrepl;
	uint32_t pnum, crc, dlen;
	int ret;

retry:
	/*
	 * If this is a fastmap run, we try to rescan full, otherwise
	 * we simply give up.
	 */
	if (!test_bit(lnum, vi->found)) {
		ubi_warn("LEB %d of %d is missing", lnum, last);
		return -EINVAL;
	}

	pnum = vi->lebs_to_pebs[lnum];

	ubi_dbg("Load vol %u LEB %u PEB %u", vol_id, lnum, pnum);

	if (ubi_io_is_bad(ubi, pnum)) {
		ubi_warn("Corrupted mapping block %d PB %d\n", lnum, pnum);
		return -EINVAL;
	}

	if (test_bit(pnum, ubi->corrupt))
		goto find_other;

	/*
	 * Lets try to read that block
	 */
	vh = ubi->blockinfo + pnum;

	if (!test_bit(pnum, ubi->scanned)) {
		ubi_warn("Vol: %u LEB %u PEB %u not yet scanned", vol_id,
			 lnum, pnum);
		if (ubi_rescan_fm_vid_hdr(ubi, vh, pnum, vol_id, lnum))
			goto find_other;
	}

	/*
	 * Check, if the total number of blocks is correct
	 */
	if (be32toh(vh->used_ebs) != last) {
		ubi_dbg("Block count mismatch.");
		ubi_dbg("vh->used_ebs: %d nrblocks: %d",
			be32toh(vh->used_ebs), last);
		generic_set_bit(pnum, ubi->corrupt);
		goto find_other;
	}

	/*
	 * Get the data length of this block.
	 */
	dlen = be32toh(vh->data_size);

	/* Return if we only want to get the data length,
	 * or all data will be skipped
	 */
	if (!laddr || skiplen >= dlen) {
		*retlen = 0;
		return dlen;
	}

	if (dlen - skiplen < readlen)
		readlen = dlen - skiplen;

	/* Use cache to speed up FIP parsing */
	if (!ubi->leb_cache_info.valid ||
	    ubi->leb_cache_info.vol_id != vol_id ||
	    ubi->leb_cache_info.leb != lnum ||
	    ubi->leb_cache_info.len != dlen ||
	    ubi->leb_cache_info.crc != be32toh(vh->data_crc)) {
		/*
		 * Read the data into RAM.
		 */
		ret = ubi_io_read(ubi, ubi->leb_cache, pnum, ubi->leb_start, dlen);
		if (ret && ret != UBI_IO_BITFLIPS) {
			ubi_warn("Vol: %u LEB %u PEB %u read failure", vol_id,
				lnum, pnum);
			generic_set_bit(pnum, ubi->corrupt);
			goto find_other;
		}

		/* Calculate CRC over the data */
		crc = ubi_crc32(UBI_CRC32_INIT, ubi->leb_cache, dlen);

		if (crc != be32toh(vh->data_crc)) {
			ubi_warn("Vol: %u LEB %u PEB %u data CRC failure", vol_id,
				lnum, pnum);
			generic_set_bit(pnum, ubi->corrupt);
			goto find_other;
		}

		ubi->leb_cache_info.vol_id = vol_id;
		ubi->leb_cache_info.leb = lnum;
		ubi->leb_cache_info.len = dlen;
		ubi->leb_cache_info.crc = crc;
		ubi->leb_cache_info.valid = 1;
	}

	/* We are good. Return the data length we read */
	memcpy(laddr, ubi->leb_cache + skiplen, readlen);

	*retlen = readlen;
	return dlen;

find_other:
	ubi_dbg("Find replacement for LEB %u PEB %u", lnum, pnum);
	generic_clear_bit(lnum, vi->found);
	vrepl = NULL;

	for (pnum = 0; pnum < ubi->peb_count; pnum++) {
		struct ubi_vid_hdr *tmp = ubi->blockinfo + pnum;
		uint32_t t_vol_id = be32toh(tmp->vol_id);
		uint32_t t_lnum = be32toh(tmp->lnum);

		if (test_bit(pnum, ubi->corrupt))
			continue;

		if (t_vol_id != vol_id || t_lnum != lnum)
			continue;

		if (!test_bit(pnum, ubi->scanned)) {
			ubi_warn("Vol: %u LEB %u PEB %u not yet scanned",
				 vol_id, lnum, pnum);
			if (ubi_rescan_fm_vid_hdr(ubi, tmp, pnum, vol_id, lnum))
				continue;
		}

		/*
		 * We found one. If its the first, assign it otherwise
		 * compare the sqnum
		 */
		generic_set_bit(lnum, vi->found);

		if (!vrepl) {
			vrepl = tmp;
			continue;
		}

		if (be64toh(vrepl->sqnum) < be64toh(tmp->sqnum))
			vrepl = tmp;
	}

	if (vrepl) {
		/* Update the vi table */
		pnum = vrepl - ubi->blockinfo;
		vi->lebs_to_pebs[lnum] = pnum;
		ubi_dbg("Trying PEB %u for LEB %u", pnum, lnum);
		vh = vrepl;
	}
	goto retry;
}

/*
 * Load a volume into RAM
 */
static int ipl_load(struct ubi_scan_info *ubi, const uint32_t vol_id,
		    uint8_t *laddr, uint32_t offset, uint32_t size)
{
	uint32_t lnum, last, retlen, skiplen, doff = 0, lenread = 0;
	struct ubi_vol_info *vi;
	int ret;

	if (vol_id >= UBI_SPL_VOL_IDS)
		return -EINVAL;

	vi = ubi->volinfo + vol_id;
	last = vi->last_block + 1;

	/* Read the blocks to RAM, check CRC */
	for (lnum = 0 ; lnum < last; lnum++) {
		if (laddr && doff < offset)
			skiplen = offset - doff;
		else
			skiplen = 0;

		ret = ubi_load_block(ubi, laddr, vi, vol_id, lnum, last,
				     skiplen, size - lenread, &retlen);
		if (ret < 0) {
			if (laddr)
				ubi_warn("Failed to load volume %u", vol_id);
			else
				ubi_warn("Failed to travers LEBs of volume %u",
					 vol_id);
			return ret;
		}

		if (laddr) {
			lenread += retlen;
			laddr += retlen;
			doff += ret;

			if (size == lenread)
				break;
		} else {
			lenread += ret;
		}
	}

	return lenread;
}

void ubispl_init_scan(struct io_ubi_dev_spec *info, int fastmap)
{
	struct ubi_scan_info *ubi = info->ubi;
	uint32_t fsize;

	/*
	 * We do a partial initializiation of @ubi. Cleaning fm_buf is
	 * not necessary.
	 */
	memset(ubi, 0, offsetof(struct ubi_scan_info, fm_buf));

	ubi->is_bad_peb = info->is_bad_peb;
	ubi->read = info->read;

	/* Precalculate the offsets */
	ubi->vid_offset = info->vid_offset;
	ubi->leb_start = info->leb_start;
	ubi->leb_size = info->peb_size - ubi->leb_start;
	ubi->peb_count = info->peb_count;
	ubi->peb_offset = info->peb_offset;
	ubi->vtbl_valid = 0;

	fsize = info->peb_size * info->peb_count;
	ubi->fsize_mb = fsize >> 20;

	/* Fastmap init */
	ubi->fm_size = ubi_calc_fm_size(ubi);
	ubi->fm_enabled = fastmap;

	ubi_msg("scanning [0x%" PRIx64 " - 0x%" PRIx64 "] ...",
		(uint64_t)info->peb_offset * info->peb_size,
		(uint64_t)(info->peb_offset + info->peb_count) * info->peb_size);

	ipl_scan(ubi);

	ubi_msg("scanning is finished");
	ubi_msg("PEB size: %u bytes (%u KiB), LEB size: %lu bytes",
		info->peb_size, info->peb_size >> 10, ubi->leb_size);
	ubi_msg("VID header offset: %u (aligned %u), data offset: %u",
		info->vid_offset, info->vid_offset, info->leb_start);
}

static int ubispl_vol_id_from_name(struct ubi_scan_info *ubi,
				   const char *name)
{
	uint16_t len;
	uint32_t i;

	for (i = 0; i < UBI_SPL_VOL_IDS; i++) {
		len = be16toh(ubi->vtbl[i].name_len);
		if (!len)
			continue;

		if (strncmp(name, ubi->vtbl[i].name, len) == 0)
			return i;
	}

	ubi_err("No volume named %s could be found", name);
	return -ENOENT;
}

int ubispl_get_volume_data_size(struct io_ubi_dev_spec *info, int vol_id,
		  		const char *vol_name)
{
	struct ubi_scan_info *ubi = info->ubi;
	int ret;

	if (vol_id < 0) {
		if (!vol_name)
			return -EINVAL;

		vol_id = ubispl_vol_id_from_name(ubi, vol_name);
		if (vol_id < 0)
			return vol_id;
	}

	if (ubi->sizes[vol_id].valid)
		return ubi->sizes[vol_id].size;

	ret = ipl_load(ubi, vol_id, NULL, 0, UINT32_MAX);
	if (ret < 0)
		return ret;

	ubi->sizes[vol_id].size = ret;
	ubi->sizes[vol_id].valid = 1;

	if (vol_id < 0) {
		ubi_msg("Volume #%d size is %u bytes", vol_id, ret);
	} else {
		ubi_msg("Volume %s (Id #%d) size is %u bytes", vol_name, vol_id,
			ret);
	}

	return ret;
}

int ubispl_load_volume(struct io_ubi_dev_spec *info, int vol_id,
		       const char *vol_name, void *buf, uint32_t offset,
		       uint32_t len, uint32_t *retlen)
{
	struct ubi_scan_info *ubi = info->ubi;
	int res;

	if (vol_id < 0) {
		if (!vol_name)
			return -EINVAL;

		vol_id = ubispl_vol_id_from_name(ubi, vol_name);
		if (vol_id < 0)
			return vol_id;

		ubi_dbg("Loading volume with name %s (Id #%d)", vol_name,
			vol_id);
	} else {
		ubi_dbg("Loading volume with Id #%d", vol_id);
	}

	res = ipl_load(ubi, vol_id, buf, offset, len);
	if (res < 0) {
		ubi_warn("Failed");
		return res;
	}

	*retlen = res;

	return 0;
}
