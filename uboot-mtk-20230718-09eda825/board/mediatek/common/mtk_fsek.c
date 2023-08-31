// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Tim-cy Yang <Tim-cy.Yang@mediatek.com>
 */

#include <common.h>
#include <image.h>
#include <linux/arm-smccc.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/libfdt.h>
#include <hexdump.h>
#include <malloc.h>
#include "boot_helper.h"
#include "dm_parser.h"
#include "mtk_fsek.h"
#include "mtk_sec_env.h"

static const char *fsek_node_names[] = {
	MTK_FSEK_ROE_KEY,
	MTK_FSEK_FIT_SECRET,
	MTK_FSEK_RFS_KEY,
	MTK_FSEK_SIG_HASH
};

static inline void memzero_explicit(void *s, size_t count)
{
	memset(s, 0, count);
	barrier_data(s);
}

static int get_fit_secret_prop(const void *fit, const char *conf_name,
			       const char *prop_name, const void **prop,
			       int *size)
{
	int conf_noffset;
	int sec_noffset;

	if (!fit || !conf_name || !prop_name || !prop || !size)
		return -EINVAL;

	sec_noffset = fdt_path_offset(fit, FIT_SECRETS_PATH);
	if (sec_noffset < 0) {
		printf("Can't find fit-secrets parent node %s\n",
			FIT_SECRETS_PATH);
		return sec_noffset;
	}

	conf_noffset = fdt_subnode_offset(fit, sec_noffset, conf_name);
	if (conf_noffset < 0) {
		printf("Can't find %s node in %s node\n", conf_name,
			FIT_SECRETS_PATH);
		return conf_noffset;
	}

	*prop = fdt_getprop(fit, conf_noffset, prop_name, size);
	if (!(*prop)) {
		printf("Can't get property %s in %s/%s node\n", prop_name,
			FIT_SECRETS_PATH, conf_name);
		return -ENOENT;
	}

	return 0;
}

/**
 * get_sig_hash_data_and_size
 * @fit: pointer to the FIT format image header
 * @conf_noffset: config node's offset used to boot in FIT
 * @conf_name: config name used for booting
 * @hash: signature's hash buffer
 * @size: size of @hash in bytes
 *
 * get_sig_hash_data_and_size extracts signature according to
 * current using config, algorithm, and calculates hash of signature, stores
 * hash value and size in @hash, @size.
 *
 * returns:
 *	0, on success
 *	<0, on failure
 */
static int get_sig_hash_data_and_size(const void *fit, int conf_noffset,
				      const char *conf_name, uint8_t *hash,
				      int *size)
{
	int len;
	int ret;
	int sig_noffset;
	const void *val;
	char *algo_name;

	if (!fit || conf_noffset < 0 || !conf_name || !hash || !size)
		return -EINVAL;

	sig_noffset = fdt_subnode_offset(fit, conf_noffset, FIT_SIG_NODENAME);
	if (sig_noffset < 0) {
		printf("Can't find %s node in config\n", FIT_SIG_NODENAME);
		return sig_noffset;
	}

	ret = get_fit_secret_prop(fit, conf_name, FIT_ALGO_PROP,
				  (const void **)&algo_name, &len);
	if (ret) {
		printf("Can't get FIT-secret hash algorithm: %d\n", ret);
		return ret;
	}

	val = fdt_getprop(fit, sig_noffset, FIT_VALUE_PROP, &len);
	if (!val) {
		printf("Can't find property %s in %s\n", FIT_VALUE_PROP,
							 FIT_SIG_NODENAME);
		return -ENOENT;
	}

	ret = calculate_hash(val, len, algo_name, hash, size);
	if (ret)
		printf("Can't calculate hash of signature\n");

	return ret;
}

static int get_roek_data_and_size(uint8_t *data, int *len)
{
	int ret;
	void *sec_env;

	if (!data || !len)
		return -EINVAL;

	ret = board_sec_env_load();
	if (ret) {
		puts("Can't load secure env\n");
		return ret;
	}

	ret = get_sec_env(MTK_FSEK_ROE_KEY, &sec_env);
	if (ret) {
		printf("Failed to get secure env: %s\n", MTK_FSEK_ROE_KEY);
		return ret;
	}

	if (!sec_env || strlen(sec_env) != MTK_FSEK_KEY_LEN * 2)
		return -EINVAL;

	ret = hex2bin(data, sec_env, MTK_FSEK_KEY_LEN);
	if (ret) {
		printf("Failed to convert hex to bin\n");
		return ret;
	}

	*len = MTK_FSEK_KEY_LEN;

	return 0;
}

/**
 * fsek_setup_shm
 * @fit: pointer to the FIT format image header
 * @conf_noffset: config node's offset used to boot in FIT
 * @conf_name: config node's name used to boot in FIT
 * @shm_paddr: shared memory physical address to store data
 *
 * fesk_setup_shm extracts encrypted data from FIT, and
 * copys to shared memory region within ATF
 *
 * Buffer is following format:
 * Low address	--------------------------------
 * 		| k-roe			       |
 * 		--------------------------------
 * 		| fit-secret		       |
 * 		--------------------------------
 * 		| k-rootfs		       |
 * 		--------------------------------
 * 		| fit-signature's hash	       |
 * High address	--------------------------------
 * returns:
 *	0, on success
 *	<0, on failure
 */
static int fsek_setup_shm(const void *fit, int conf_noffset,
			  const char *conf_name, uint8_t *shm_paddr)
{
	int ret;
	int i = 0;
	int len;
	uint8_t data[MTK_FSEK_MAX_DATA_LEN];

	if (!fit || conf_noffset < 0 || !conf_name || !shm_paddr)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(fsek_node_names); i++) {
		const void *prop;

		if (!strncmp(fsek_node_names[i], MTK_FSEK_FIT_SECRET,
			     strlen(MTK_FSEK_FIT_SECRET))) {
			ret = get_fit_secret_prop(fit, conf_name, FIT_DATA_PROP,
						  &prop, &len);
			if (ret)
				return ret;
		} else if (!strncmp(fsek_node_names[i], MTK_FSEK_SIG_HASH,
				  strlen(MTK_FSEK_SIG_HASH))) {
			ret = get_sig_hash_data_and_size(fit, conf_noffset,
							 conf_name, data, &len);
			if (ret)
				return ret;

			prop = data;
		}  else if (!strncmp(fsek_node_names[i], MTK_FSEK_ROE_KEY,
				     strlen(MTK_FSEK_ROE_KEY))) {
			ret = get_roek_data_and_size(data, &len);
			if (ret)
				return ret;

			prop = data;
		} else {
			prop = fdt_getprop(fit, conf_noffset,
					   fsek_node_names[i], &len);
			if (!prop) {
				printf("Can't find property %s in %s\n",
				       fsek_node_names[i], conf_name);
				return -ENOENT;
			}
		}

		if (len <= 0 || len > MTK_FSEK_MAX_DATA_LEN)
			return -EINVAL;

		memcpy(shm_paddr + i * MTK_FSEK_MAX_DATA_LEN, prop, len);
	}

	return 0;
}

static int get_shm_config(uintptr_t *shm_paddr, size_t *size)
{
	struct arm_smccc_res res;

	if (!shm_paddr || !size)
		return -EINVAL;

	*shm_paddr = 0;
	*size = 0;

	arm_smccc_smc(MTK_FSEK_GET_SHM_CONFIG, 0, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0 || !res.a1 || !res.a2)
		return -ENOMEM;

	*shm_paddr = res.a1;
	*size = res.a2;

	return 0;
}

/**
 * mtk_fsek_decrypt_rfsk
 * @fit: pointer to the FIT format image header
 * @conf_name: config node's name used to boot in FIT
 * @conf_noffset: config node's offset used to boot in FIT
 *
 * mtk_fsek_decrypt_rfsk extracts encrypted data from FIT,
 * setups data in shared buffer, and invokes SMC call to ask
 * ATF to decrypt data.
 *
 * returns:
 *	0, on success
 *	<0, on failure
 */
int mtk_fsek_decrypt_rfsk(const void *fit, const char *conf_name,
			  int conf_noffset)
{
	int ret;
	struct arm_smccc_res res;
	uintptr_t shm_paddr;
	size_t shm_size;

	if (!fit || !conf_name || conf_noffset < 0)
		return -EINVAL;

	ret = get_shm_config(&shm_paddr, &shm_size);
	if (ret) {
		puts("Can't get shm config\n");
		return -ENOMEM;
	}
	if (ARRAY_SIZE(fsek_node_names) * MTK_FSEK_MAX_DATA_LEN > shm_size)
		return -ENOMEM;

	ret = fsek_setup_shm(fit, conf_noffset,
			     conf_name, (uint8_t *)shm_paddr);
	if (ret) {
		puts("Can't setup encrypted data to shm\n");
		goto decrypt_rfsk_err;
	}

	puts("mtk-fsek: Decrypting RFSK... ");
	arm_smccc_smc(MTK_FSEK_DECRYPT_RFSK, 0, 0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;

decrypt_rfsk_err:
	memzero_explicit((void *)shm_paddr, ARRAY_SIZE(fsek_node_names) *
	       MTK_FSEK_MAX_DATA_LEN);

	if (ret)
		printf("Failed: %d\n", ret);
	else
		puts("OK\n");

	return ret;
}

static int mtk_fsek_get_key(uint32_t key_id, struct arm_smccc_res *res)
{
	if (!res)
		return -EINVAL;

	arm_smccc_smc(MTK_FSEK_GET_KEY, key_id, 0, 0, 0, 0, 0, 0, res);
	if (res->a0 && !res->a1 && !res->a2 && !res->a3)
		return res->a0;

	return 0;
}

static bool compare_key(const char *key, size_t keylen, const char *str)
{
	size_t ckeylen;

	ckeylen = strlen(str);
	if (ckeylen == keylen) {
		if (!strncmp(str, key, keylen))
			return true;
	}

	return false;
}

/**
 * construct_dm_mod_create_args
 * @dm_mod: string of dm-mod.create arguments
 * @b: current pointer in new bootargs buffer
 *
 * construct_dm_mod_create_args parses dm-mod.create, finds
 * dm-crypt key argument position, and calls SMC to get key
 * from ATF, inserts key value into new bootargs.
 *
 * returns:
 *	0, on success
 * 	<0, on failure
 */
static int construct_dm_mod_create_args(char *dm_mod, char *b)
{
	int ret;
	struct dm_crypt_info dci = {0};
	struct arm_smccc_res res;

	if (!dm_mod || !b)
		return -EINVAL;

	ret = dm_mod_create_arg_parse(dm_mod, "crypt", (struct dm_info *)&dci);
	if (ret) {
		puts("Can't parse dm-crypt args\n");
		goto construct_dm_mod_create_err;
	}

	if (!dci.key) {
		ret = -EINVAL;
		goto construct_dm_mod_create_err;
	}

	memcpy(b, dm_mod, dci.key_pos);
	b += dci.key_pos;

	ret = mtk_fsek_get_key(MTK_FSEK_RFS_KEY_ID, &res);
	if (ret) {
		printf("ATF SMC call failed: %d\n", ret);
		goto construct_dm_mod_create_err;
	}

	b = bin2hex(b, (uint8_t *)&res, sizeof(struct arm_smccc_res));

	memcpy(b, dm_mod + dci.key_pos + dci.key_len,
	       strlen(dm_mod) - dci.key_pos - dci.key_len);
	b += strlen(dm_mod) - dci.key_pos - dci.key_len;

	memcpy(b, "\" ", 2);
	b += 2;
	*b = '\0';

construct_dm_mod_create_err:
	memzero_explicit(&res, sizeof(struct arm_smccc_res));
	if (dci.tmpbuf)
		free(dci.tmpbuf);

	return ret;
}

static int create_new_bootargs(const char *orig_bootargs, char *new_bootargs)
{
	int ret;
	size_t keylen;
	size_t plen;
	char *dm_mod = NULL;
	const char *n;
	const char *p;
	char *b;

	if (!orig_bootargs || !strlen(orig_bootargs) || !new_bootargs)
		return -EINVAL;

	n = orig_bootargs;
	b = new_bootargs;

	/* find dm-mod.create entry in original bootargs */
	while (1) {
		n = get_arg_next(n, &p, &keylen);
		if (!n)
			break;

		plen = n - p;

		if (compare_key(p, keylen, "dm-mod.create")) {
			if (dm_mod) {
				free(dm_mod);
				return -EINVAL;
			}
			/* assume "" always exists */
			if (keylen + 3 >= plen)
				return -ENOMEM;

			dm_mod = malloc(plen - keylen - 2);
			if (!dm_mod)
				return -ENOMEM;

			memcpy(dm_mod, p + keylen + 2, plen - keylen - 3);
			dm_mod[plen - keylen - 3] = '\0';
		} else {
			memcpy(b, p, plen);
			b += plen;
			*b++ = ' ';
		}
	}

	if (!dm_mod)
		return -EINVAL;

	p = "dm-mod.create=\"";
	plen = strlen(p);
	memcpy(b, p, plen);
	b += plen;

	ret = construct_dm_mod_create_args(dm_mod, b);
	if (ret)
		puts("Can't parse dm-mod args\n");

	free(dm_mod);

	return ret;
}

static int set_bootargs(void *fdt)
{
	int ret;
	int orig_len;
	int len;
	int chosen_noffset;
	const char *orig_bootargs;
	char *bootargs;

	if (!fdt)
		return -EINVAL;

	chosen_noffset = fdt_find_or_add_subnode(fdt, 0, "chosen");
	if (chosen_noffset < 0) {
		puts("Can't find chosen node in FDT\n");
		return -ENOENT;
	}

	orig_bootargs = fdt_getprop(fdt, chosen_noffset, "bootargs", &orig_len);
	if (!orig_bootargs) {
		puts("Can't find property bootargs in chosen node\n");
		return -ENOENT;
	}

	if (orig_len <= 0 ||
	    INT_MAX - orig_len < MTK_FSEK_KEY_LEN * 2 + 1)
		return -EINVAL;

	len = orig_len + MTK_FSEK_KEY_LEN * 2 + 1;
	if (len > MAX_BOOTARGS_LEN)
		return -EINVAL;

	bootargs = malloc(len);
	if (!bootargs)
		return -ENOMEM;

	ret = create_new_bootargs(orig_bootargs, bootargs);
	if (ret) {
		puts("Can't create new bootargs\n");
		goto set_bootargs_err;
	}

	ret = fdt_setprop(fdt, chosen_noffset,
			  "bootargs", bootargs, strlen(bootargs));
	if (ret)
		printf("Can't set property bootargs in chosen node: %d\n", ret);

set_bootargs_err:
	memzero_explicit(bootargs, len);
	free(bootargs);

	return ret;
}

static int set_dev_specific_key(void *fdt)
{
	int ret;
	int dev_noffset;
	struct arm_smccc_res res;

	if (!fdt)
		return -EINVAL;

	dev_noffset = fdt_find_or_add_subnode(fdt, 0, FDT_DEV_NODE);
	if (dev_noffset < 0) {
		printf("Can't find or add %s node\n", FDT_DEV_NODE);
		return dev_noffset;
	}

	ret = mtk_fsek_get_key(MTK_FSEK_DER_KEY_ID, &res);
	if (ret) {
		printf("ATF SMC call failed: %d\n", ret);
		goto set_dev_specific_key_err;
	}

	ret = fdt_setprop(fdt, dev_noffset, FDT_DEV_NODE, (uint8_t *)&res,
			  sizeof(struct arm_smccc_res));
	if (ret)
		printf("Can't set property in %s node\n", FDT_DEV_NODE);

set_dev_specific_key_err:
	memzero_explicit(&res, sizeof(struct arm_smccc_res));

	return ret;
}

int mtk_fsek_set_fdt(void *fdt)
{
	int ret;

	if (!fdt)
		return -EINVAL;

	ret = set_bootargs(fdt);
	if (ret) {
		puts("Can't set bootargs in FDT\n");
		return ret;
	}

	ret = set_dev_specific_key(fdt);
	if (ret)
		puts("Can't set dev-specific key\n");

	return ret;
}
