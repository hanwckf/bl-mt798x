// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (C) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <common/debug.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_ubi.h>
#include <drivers/io/io_storage.h>
#include <lib/utils.h>

#include "ubispl.h"

#ifndef MAX_UBI_DEVICES
#define MAX_UBI_DEVICES		1
#endif

typedef struct {
	io_ubi_dev_spec_t *dev_spec;

	uint32_t read_pos;
	uint32_t vol_size;
	bool vol_size_init;

	const io_ubi_spec_t *vol;
} ubi_dev_state_t;

static ubi_dev_state_t state_pool[MAX_UBI_DEVICES];
static io_dev_info_t dev_info_pool[MAX_UBI_DEVICES];

/* Track number of allocated UBI state */
static unsigned int ubi_dev_count;

static int ubi_dev_open(const uintptr_t dev_spec, io_dev_info_t **dev_info);

static const io_dev_connector_t ubi_dev_connector = {
	.dev_open = ubi_dev_open
};

static io_type_t device_type_ubi(void);
static int ubi_volume_open(io_dev_info_t *dev_info, const uintptr_t spec,
			   io_entity_t *entity);
static int ubi_volume_seek(io_entity_t *entity, int mode,
			   signed long long offset);
static int ubi_volume_size(io_entity_t *entity, size_t *length);
static int ubi_volume_read(io_entity_t *entity, uintptr_t buffer, size_t length,
			   size_t *length_read);
static int ubi_volume_close(io_entity_t *entity);
static int ubi_dev_close(io_dev_info_t *dev_info);

static const io_dev_funcs_t ubi_dev_funcs = {
	.type = device_type_ubi,
	.open = ubi_volume_open,
	.seek = ubi_volume_seek,
	.size = ubi_volume_size,
	.read = ubi_volume_read,
	.write = NULL,
	.close = ubi_volume_close,
	.dev_close = ubi_dev_close,
};

/* Locate a UBI state in the pool, specified by address */
static int find_first_ubi_state(const io_ubi_dev_spec_t *dev_spec,
				unsigned int *index_out)
{
	int result = -ENOENT;
	unsigned int index;

	for (index = 0; index < (unsigned int)MAX_UBI_DEVICES; ++index) {
		/* dev_spec is used as identifier since it's unique */
		if (state_pool[index].dev_spec == dev_spec) {
			result = 0;
			*index_out = index;
			break;
		}
	}
	return result;
}


/* Allocate a device info from the pool and return a pointer to it */
static int allocate_dev_info(io_dev_info_t **dev_info)
{
	int result = -ENOMEM;

	assert(dev_info != NULL);

	if (ubi_dev_count < (unsigned int)MAX_UBI_DEVICES) {
		unsigned int index = 0;

		result = find_first_ubi_state(NULL, &index);
		assert(result == 0);
		/* initialize dev_info */
		dev_info_pool[index].funcs = &ubi_dev_funcs;
		dev_info_pool[index].info =
				(uintptr_t)&state_pool[index];
		*dev_info = &dev_info_pool[index];
		++ubi_dev_count;
	}

	return result;
}

/* Release a device info to the pool */
static int free_dev_info(io_dev_info_t *dev_info)
{
	int result;
	unsigned int index = 0;
	ubi_dev_state_t *state;

	assert(dev_info != NULL);

	state = (ubi_dev_state_t *)dev_info->info;
	result = find_first_ubi_state(state->dev_spec, &index);
	if (result ==  0) {
		/* free if device info is valid */
		zeromem(state, sizeof(ubi_dev_state_t));
		--ubi_dev_count;
	}

	return result;
}

/* Simply identify the device type as a MTD driver */
static io_type_t device_type_ubi(void)
{
	return IO_TYPE_MTD;
}

static int ubi_volume_close(io_entity_t *entity)
{
	/* Clear the Entity info. */
	entity->info = 0;

	return 0;
}

static int ubi_volume_open(io_dev_info_t *dev_info, const uintptr_t spec,
			   io_entity_t *entity)
{
	ubi_dev_state_t *ud = (ubi_dev_state_t *)dev_info->info;
	const io_ubi_spec_t *ubi_spec = (const io_ubi_spec_t *)spec;

	assert((dev_info->info != (uintptr_t)NULL) &&
	       (spec != (uintptr_t)NULL) &&
	       (entity->info == (uintptr_t)NULL));

	assert((ubi_spec->vol_id >= 0 && ubi_spec->vol_id < UBI_SPL_VOL_IDS) ||
	       (ubi_spec->vol_id < 0 && ubi_spec->vol_name));

	ud->vol = ubi_spec;
	ud->read_pos = 0;
	ud->vol_size_init = false;
	ud->vol_size = 0;

	entity->info = (uintptr_t)ud;

	return 0;
}

static int ubi_volume_size(io_entity_t *entity, size_t *length)
{
	ubi_dev_state_t *ud = (ubi_dev_state_t *)entity->info;
	int ret;

	assert(entity != NULL);
	assert(length != NULL);

	if (!ud->dev_spec->init_done) {
	retry:
		ubispl_init_scan(ud->dev_spec, ud->dev_spec->fastmap);
	}

	ret = ubispl_get_volume_data_size(ud->dev_spec, ud->vol->vol_id,
					  ud->vol->vol_name);
	if (ret < 0) {
		if (ud->dev_spec->fastmap) {
			ud->dev_spec->fastmap = 0;
			goto retry;
		}

		if (ret == -ENOENT)
			ud->dev_spec->init_done = 1;

		return ret;
	}

	ud->dev_spec->init_done = 1;

	*length = ret;

	return 0;
}

static int ubi_volume_seek(io_entity_t *entity, int mode,
			   signed long long offset)
{
	ubi_dev_state_t *ud = (ubi_dev_state_t *)entity->info;
	size_t vol_size;
	int ret;

	assert(entity->info != (uintptr_t)NULL);

	if (!ud->vol_size_init) {
		ret = ubi_volume_size(entity, &vol_size);
		if (ret)
			return ret;

		ud->vol_size_init = true;
		ud->vol_size = vol_size;
	}

	assert((offset >= 0) && ((unsigned long long)offset < ud->vol_size));

	switch (mode) {
	case IO_SEEK_SET:
		ud->read_pos = (uint32_t)offset;
		break;
	case IO_SEEK_CUR:
		ud->read_pos += (int32_t)offset;
		break;
	default:
		return -EINVAL;
	}

	assert(ud->read_pos < ud->vol_size);

	return 0;
}

static int ubi_volume_read(io_entity_t *entity, uintptr_t buffer, size_t length,
			   size_t *length_read)
{
	ubi_dev_state_t *ud = (ubi_dev_state_t *)entity->info;
	uint32_t retlen;
	int ret;

	assert(entity != NULL);
	assert(length_read != NULL);
	assert(entity->info != (uintptr_t)NULL);

	if (!ud->dev_spec->init_done) {
	retry:
		ubispl_init_scan(ud->dev_spec, ud->dev_spec->fastmap);
	}

	ret = ubispl_load_volume(ud->dev_spec, ud->vol->vol_id,
				 ud->vol->vol_name, (void *)buffer,
				 ud->read_pos, length, &retlen);
	if (ret < 0) {
		if (ud->dev_spec->fastmap) {
			ud->dev_spec->fastmap = 0;
			goto retry;
		}

		if (ret == -ENOENT)
			ud->dev_spec->init_done = 1;

		return ret;
	}

	ud->dev_spec->init_done = 1;

	*length_read = retlen;
	ud->read_pos += retlen;

	return 0;
}

/* Close a connection to the UBI device */
static int ubi_dev_close(io_dev_info_t *dev_info)
{
	return free_dev_info(dev_info);
}

static int ubi_dev_open(const uintptr_t dev_spec, io_dev_info_t **dev_info)
{
	int result;
	io_dev_info_t *info;
	ubi_dev_state_t *state;

	assert(dev_info != NULL);
#if MAX_UBI_DEVICES > 1
	assert(dev_spec != (uintptr_t)NULL);
#endif

	result = allocate_dev_info(&info);
	if (result != 0)
		return -ENOMEM;

	state = (ubi_dev_state_t *)info->info;
	state->dev_spec = (io_ubi_dev_spec_t *)dev_spec;

	assert((state->dev_spec->ubi != NULL) &&
	       (state->dev_spec->peb_size > 0) &&
	       (state->dev_spec->peb_count > 0) &&
	       (state->dev_spec->read != NULL));

	*dev_info = info;

	return 0;
}

int register_io_dev_ubi(const io_dev_connector_t **dev_con)
{
	int result;
	assert(dev_con != NULL);

	/*
	 * Since dev_info isn't really used in io_register_device, always
	 * use the same device info at here instead.
	 */
	result = io_register_device(&dev_info_pool[0]);
	if (result == 0)
		*dev_con = &ubi_dev_connector;

	return result;
}
