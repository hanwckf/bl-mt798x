// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic image verification helper
 */

#include <errno.h>
#include <image.h>
#include <vsprintf.h>
#include <linux/sizes.h>
#include <u-boot/crc.h>

#include "upgrade_helper.h"
#include "verify_helper.h"
#include "untar.h"

/* Only supports native byte-order for squashfs */
#define SQUASHFS_MAGIC		0x73717368

struct squashfs_super_block {
	u32 s_magic;
	u32 padding0[9];
	u64 bytes_used;
};

/**
 * fit_record_hashes() - Get all supported hashes from image node
 *
 * @description:
 * Get all supported hashes from image node
 *
 * @param fit: Pointer to FIT image data
 * @param image_noffset: image node offset in FIT
 * @param hashes: on return records all supported hash values
 * @return true if at least one supported hash found
 */
static bool fit_record_hashes(const void *fit, int image_noffset,
			      struct fit_hashes *hashes)
{
	int fit_value_len, noffset;
	uint8_t *fit_value;
	const char *algo;

	hashes->flags = 0;

	fdt_for_each_subnode(noffset, fit, image_noffset) {
		const char *name = fit_get_name(fit, noffset, NULL);

		if (strncmp(name, FIT_HASH_NODENAME,
			    strlen(FIT_HASH_NODENAME)))
			continue;

		debug("%s: found hash node '%s'\n", __func__, name);

		if (fit_image_hash_get_algo(fit, noffset, &algo)) {
			printf("Warning: algo property is missing in %s\n",
			       name);
			continue;
		}

		if (fit_image_hash_get_value(fit, noffset, &fit_value,
					     &fit_value_len)) {
			printf("Warning: hash property is missing in %s\n",
			       name);
			continue;
		}

		if (!strcmp(algo, "crc32")) {
			if (fit_value_len != 4) {
				printf("Warning: hash length mismatch in %s\n",
				       name);
				continue;
			}

			hashes->crc32 = fdt32_to_cpu(*(uint32_t *)fit_value);
			hashes->flags |= FIT_HASH_CRC32;
		}

		if (!strcmp(algo, "sha1")) {
			if (fit_value_len != SHA1_SUM_LEN) {
				printf("Warning: hash length mismatch in %s\n",
				       name);
				continue;
			}

			memcpy(hashes->sha1, fit_value, SHA1_SUM_LEN);
			hashes->flags |= FIT_HASH_SHA1;
		}
	}

	return hashes->flags != 0;
}

/**
 * verify_kernel_fit() - Verify kernel FIT image
 *
 * @description:
 * Verify kernel FIT image.
 *
 * @param fit: Pointer to FIT image data
 * @param size: Size of FIT image data
 * @param hashes: (optional) stores hashes of kernel
 * @return true if passed
 */
static bool verify_kernel_fit(const void *fit,  size_t size,
			      struct fit_hashes *hashes)
{
	int default_noffset, kernel_noffset;

	if (fit_check_format(fit, size)) {
		printf("Wrong FIT image format\n");
		return false;
	}

	if (!fit_all_image_verify(fit)) {
		printf("FIT image integrity checking failed\n");
		return false;
	}

	if (!hashes)
		return true;

	/* Find default configuration node */
	default_noffset = fit_conf_get_node(fit, NULL);

	debug("%s: default_noffset = 0x%x\n", __func__, default_noffset);

	if (default_noffset < 0) {
		printf("Default configuration node not found\n");
		return false;
	}

	/* Find kernel image node */
	kernel_noffset = fit_conf_get_prop_node(fit, default_noffset, "kernel",
						IH_PHASE_NONE);

	debug("%s: kernel_noffset = 0x%x\n", __func__, kernel_noffset);

	if (kernel_noffset < 0) {
		printf("Kernel image node not found\n");
		return false;
	}

	/* Record hashes */
	return fit_record_hashes(fit, kernel_noffset, hashes);
}

/**
 * verify_rootfs_hash_fit() - Verify rootfs with given hash from FIT image
 *
 * @description:
 * Verify the rootfs with given hash value and algo from FIT image.
 *
 * @param fit: Pointer to FIT image data
 * @param noffset: Node offset of rootfs in FIT image
 * @param rootfs: Pointer to rootfs data
 * @param size: Size of rootfs data
 * @param hashes: (optional) stores hashes of rootfs
 * @return zero if passed, negative if failed, positive if not supported
 */
static int verify_rootfs_hash_fit(const void *fit, int noffset,
				  const void *rootfs, size_t size,
				  struct fit_hashes *hashes)
{
	u8 value[FIT_MAX_HASH_LEN];
	int value_len, fit_value_len;
	u8 *fit_value;
	const char *algo;

	if (fit_image_hash_get_algo(fit, noffset, &algo)) {
		printf("Warning: algo property is missing\n");
		return -EINVAL;
	}

	if (fit_image_hash_get_value(fit, noffset, &fit_value,
				     &fit_value_len)) {
		printf("Warning: hash property is missing\n");
		return -EINVAL;
	}

	printf("%s", algo);

	if (calculate_hash(rootfs, size, algo, value, &value_len)) {
		debug("Warning: Unsupported hash algorithm '%s'\n", algo);
		return 1;
	}

	if (value_len != fit_value_len) {
		printf("- ");
		debug("Error: Bad hash value length\n");
		return -EBADMSG;
	} else if (memcmp(value, fit_value, value_len) != 0) {
		printf("- ");
		debug("Error: Bad hash value\n");
		return -EBADMSG;
	}

	printf("+ ");

	if (!hashes)
		return 0;

	if (!strcmp(algo, "crc32")) {
		hashes->crc32 = fdt32_to_cpu(*(uint32_t *)fit_value);
		hashes->flags |= FIT_HASH_CRC32;
	}

	if (!strcmp(algo, "sha1")) {
		memcpy(hashes->sha1, fit_value, SHA1_SUM_LEN);
		hashes->flags |= FIT_HASH_SHA1;
	}

	return 0;
}

/**
 * verify_rootfs_fit() - Verify rootfs associated with the given FIT image
 *
 * @description:
 * Verify rootfs associated with the given FIT image. The FIT image must
 * contain rootfs node to make verification pass.
 *
 * @param fit: Pointer to FIT image data
 * @param rootfs: Pointer to rootfs data
 * @param rootfs_size: Size of rootfs data
 * @param hashes: (optional) stores hashes of rootfs
 * @return true if integrity verification passed
 */
static bool verify_rootfs_fit(const void *fit, const void *rootfs,
			      size_t rootfs_size, struct fit_hashes *hashes)
{
	int ret, len, rootfs_noffset, noffset, failed = 0, passed = 0;
	u32 fit_rootfs_size;
	const u32 *cell;

	/* Find rootfs node */
	rootfs_noffset = fdt_path_offset(fit, "/rootfs");

	debug("%s: rootfs_noffset = 0x%x\n", __func__, rootfs_noffset);

	if (rootfs_noffset < 0) {
		if (rootfs_size) {
			printf("No rootfs node found in FIT image!\n");
			return false;
		}

		return true;
	}

	if (!rootfs_size) {
		printf("No rootfs found after FIT image!\n");
		return false;
	}

	/* Read the actual rootfs size */
	cell = fdt_getprop(fit, rootfs_noffset, "size", &len);
	if (!cell || len != sizeof(*cell)) {
		printf("'size' property does not exist in FIT\n");
		return false;
	}

	fit_rootfs_size = fdt32_to_cpu(*cell);
	if (!fit_rootfs_size) {
		printf("Invalid rootfs size in FIT\n");
		return false;
	}

	/* Avoid interference from rootfs padding */
	if (fit_rootfs_size > rootfs_size) {
		printf("Incomplete rootfs\n");
		return false;
	}

	printf("   Hash(es) for rootfs: ");

	if (hashes)
		hashes->flags = 0;

	/*
	 * Process all hash subnodes, and succeed with at least one hash
	 * verification passed.
	 */
	fdt_for_each_subnode(noffset, fit, rootfs_noffset) {
		const char *name = fit_get_name(fit, noffset, NULL);

		if (strncmp(name, FIT_HASH_NODENAME,
			    strlen(FIT_HASH_NODENAME)))
			continue;

		debug("%s: verifying hash node '%s'\n", __func__, name);

		ret = verify_rootfs_hash_fit(fit, noffset, rootfs,
					     fit_rootfs_size, hashes);
		if (ret) {
			failed++;
		} else {
			passed++;

			debug("%s: hash node '%s' verification passed\n",
			      __func__, name);
		}
	}

	printf("\n");

	if (failed) {
		printf("Error: at lease one hash node failed verification\n");
		return false;
	}

	if (!passed) {
		printf("Error: no hash node verified\n");
		return false;
	}

	return true;
}

/**
 * verify_legacy_image() - Verify legacy image
 *
 * @description:
 * Verify legacy image. Typically only kernel image can be verified.
 *
 * @param data: image to be parsed
 * @param size: size of the image
 * @param hashes: (optional) stores hashes of the payload image
 * @return true if integrity verification passed
 */
static bool verify_legacy_image(const void *data, size_t size,
				struct fit_hashes *hashes)
{
	struct legacy_img_hdr hdr;
	uint32_t dcrc;

	memcpy(&hdr, data, image_get_header_size());

	if (!image_check_hcrc(&hdr)) {
		printf("Error: Bad Header Checksum\n");
		return false;
	}

	dcrc = crc32_wd(0, data + image_get_header_size(),
			image_get_data_size(&hdr), CHUNKSZ_CRC32);

	if (hashes) {
		hashes->crc32 = dcrc;
		hashes->flags |= FIT_HASH_CRC32;
	}

	if (dcrc != image_get_dcrc(&hdr)) {
		printf("Error: Bad Data CRC\n");
		return false;
	}

	return true;
}

/**
 * verify_image_ram() - Parse and verify full image with rootfs
 *
 * @description:
 * Verify image with rootfs. Only FIT raw image (for SPI-NOR, kernel + rootfs)
 * and TAR archive (for UBI and SD/eMMC) are supported.
 *
 * @param data: image to be parsed
 * @param size: size of the image
 * @param block_size: block size for padding
 * @param verify_rootfs: whether to verify rootfs
 * @param ii: on success storing the image information
 * @param kernel_hashes: (optional) stores hashes of kernel
 * @param rootfs_hashes: (optional) stores hashes of rootfs
 * @return true if integrity verification passed
 */
bool verify_image_ram(const void *data, size_t size, u32 block_size,
		      bool verify_rootfs, struct owrt_image_info *ii,
		      struct fit_hashes *kernel_hashes,
		      struct fit_hashes *rootfs_hashes)
{
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	int ret;

	ret = parse_image_ram(data, size, block_size, ii);
	if (ret)
		return false;

	if (ii->type == IMAGE_TAR) {
		ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
				&rootfs_data, &rootfs_size);
		if (ret) {
			printf("Error: Invalid TAR archive\n");
			return false;
		}

		if (genimg_get_format(kernel_data) != IMAGE_FORMAT_FIT) {
			printf("Error: Not an FIT image\n");
			return false;
		}

		if (!verify_kernel_fit(kernel_data, kernel_size, kernel_hashes))
			return false;

		if (!verify_rootfs)
			return true;

		return verify_rootfs_fit(kernel_data, rootfs_data, rootfs_size,
					 rootfs_hashes);

	}

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_LEGACY_VERIFY) &&
	    ii->header_type == HEADER_LEGACY) {
		if (verify_rootfs && ii->rootfs_size) {
			printf("Error: Rootfs cannot be verified\n");
			return false;
		}

		return verify_legacy_image(data, size, kernel_hashes);
	}

	if (ii->header_type != HEADER_FIT) {
		printf("Error: Not an FIT image\n");
		return false;
	}

	if (ii->type != IMAGE_RAW) {
		printf("Error: UBI raw partition is not supported\n");
		return false;
	}

	if (!verify_kernel_fit(data, ii->kernel_size, kernel_hashes))
		return false;

	if (!verify_rootfs)
		return true;

	return verify_rootfs_fit(data,
				 data + ii->kernel_size + ii->padding_size,
				 ii->rootfs_size, rootfs_hashes);
}

/**
 * read_verify_kernel() - Read and verify kernel from flash
 *
 * @description:
 * Read kernel from flash and then verify. Only FIT raw image is supported.
 *
 * @param rpriv: priv structure for flash read operation
 * @param ptr: Pointer to memory to where kernel will be read
 * @param addr: Address in flash from where kernel will be read
 * @param max_size: Maximum size allowed for kernel
 * @param actual_size: (optional) on return stores the actual kernel size
 * @param hashes: (optional) stores hashes of kernel
 * @return true if integrity verification passed
 */
bool read_verify_kernel(struct image_read_priv *rpriv, void *ptr, u64 addr,
			size_t max_size, size_t *actual_size,
			struct fit_hashes *hashes)
{
	ulong probe_size, kernel_size;
	int ret;

	if (!rpriv || !ptr)
		return false;

	if (!rpriv->read)
		return false;

	probe_size = max_t(u32, sizeof(struct fdt_header),
			   sizeof(struct legacy_img_hdr));

	ret = rpriv->read(rpriv, ptr, addr, max_t(u32, rpriv->page_size,
						  probe_size));
	if (ret) {
		printf("Error: read failure at offset 0x%llx\n", addr);
		return false;
	}

	switch (genimg_get_format(ptr)) {
#if defined(CONFIG_LEGACY_IMAGE_FORMAT)
	case IMAGE_FORMAT_LEGACY:
		printf("Error: Verification on FIT image is not allowed\n");
		return false;
#endif
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		kernel_size = fit_get_size(ptr);
		if (max_size && kernel_size > max_size) {
			printf("Error: incomplete FIT image found\n");
			return false;
		}

		ret = rpriv->read(rpriv, ptr, addr, kernel_size);
		if (ret) {
			printf("Error: read failure at offset 0x%llx\n", addr);
			return false;
		}

		if (!verify_kernel_fit(ptr, kernel_size, hashes))
			return false;

		if (actual_size)
			*actual_size = kernel_size;

		return true;
#endif
	default:
		printf("Error: Invalid image format\n");
		return false;
	}
}

/**
 * read_verify_rootfs() - Read and verify rootfs from flash
 *
 * @description:
 * Read rootfs from flash and then verify. FIT image containing hashes of
 * rootfs should be provided.
 *
 * @param rpriv: priv structure for flash read operation
 * @param fit: Pointer to FIT image data
 * @param ptr: Pointer to memory to where rootfs will be read
 * @param addr: Address in flash from where rootfs will be read
 * @param max_size: Maximum size allowed for rootfs
 * @param actual_size: (optional) on return stores the actual rootfs size
 * @param keep_padding: whether to keep padding from `addr' to start of rootfs
 * @param padding_size: stores padding size from `addr' to start of rootfs
 * @param hashes: (optional) stores hashes of rootfs
 * @return true if integrity verification passed
 */
bool read_verify_rootfs(struct image_read_priv *rpriv, const void *fit,
			void *ptr, u64 addr, u64 max_size, size_t *actual_size,
			bool keep_padding, size_t *padding_size,
			struct fit_hashes *hashes)
{
	struct squashfs_super_block sb;
	u32 trailing = 0;
	u64 offset = 0;
	void *rptr;
	int ret;

	if (!rpriv || !fit || !ptr)
		return false;

	if (!rpriv->read)
		return false;

	/*
	 * For the first round, the search offset will not be aligned to
	 * the block boundary (i.e. no padding)
	 */
	if (rpriv->block_size)
		trailing = addr % rpriv->block_size;

	while (offset < max_size - sizeof(sb)) {
		rptr = ptr;
		if (keep_padding)
			rptr += offset;

		ret = rpriv->read(rpriv, rptr, addr + offset,
				  max_t(u32, rpriv->page_size, sizeof(sb)));
		if (ret) {
			printf("Error: read failure at offset 0x%llx\n",
			       addr + offset);
			return false;
		}

		debug("%s: checking at 0x%llx, magic = 0x%08x, size = 0x%llx\n",
		      __func__, addr + offset, sb.s_magic, sb.bytes_used);

		/* Check native-endian magic only */
		memcpy(&sb, rptr, sizeof(sb));
		if (sb.s_magic != SQUASHFS_MAGIC)
			goto next_offset;

		/* Whether the size field in the superblock is valid */
		if (max_size && offset + sb.bytes_used > max_size) {
			/*
			 * This may be a fake superblock, so we continue to
			 * check the next block. Mark this so we can return
			 * precise error code if no rootfs found at last.
			 */
			goto next_offset;
		}

		/* Read rootfs */
		ret = rpriv->read(rpriv, rptr, addr + offset, sb.bytes_used);
		if (ret) {
			printf("Error: read failure at offset 0x%llx\n",
			       addr + offset);
			return false;
		}

		/* Verify rootfs */
		if (verify_rootfs_fit(fit, rptr, sb.bytes_used, hashes)) {
			if (actual_size)
				*actual_size = sb.bytes_used;

			if (keep_padding && padding_size)
				*padding_size = offset;

			debug("%s: found at 0x%llx, size = 0x%llx\n", __func__,
			       addr + offset, sb.bytes_used);

			return true;
		}

	next_offset:
		/* block_size == 0 means no padding */
		if (!rpriv->block_size)
			break;

		/* Align the search offset to next block */
		offset += rpriv->block_size - trailing;
		trailing = 0;
	}

	return false;
}

/**
 * compare_hashes() - Compare hashes of two images
 *
 * @description:
 * Compare commonly shared hash(es) of two images.
 *
 * @param hash1: hash(es) from golden image as reference
 * @param hash2: hash(es) from target image to be compared
 * @return true if all commonly shared hash properties are the same
 */
bool compare_hashes(const struct fit_hashes *hash1,
		    const struct fit_hashes *hash2)
{
	int common_flags = hash1->flags & hash2->flags;

	if (!common_flags) {
		printf("Error: no common hash property shared by two images\n");
		return false;
	}

	if (common_flags & FIT_HASH_CRC32) {
		if (hash1->crc32 != hash2->crc32) {
			printf("CRC32 checksum mismatch\n");
			return false;
		}
	}

	if (common_flags & FIT_HASH_SHA1) {
		if (memcmp(hash1->sha1, hash2->sha1, SHA1_SUM_LEN)) {
			printf("SHA1 hash mismatch\n");
			return false;
		}
	}

	return true;
}
