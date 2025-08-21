// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <env.h>
#include <mtd_node.h>
#include <linux/mtd/mtd.h>
#include <linux/string.h>
#include <dm/ofnode.h>

static ofnode ofnode_get_mtd_layout(const char *layout_label)
{
	ofnode node, layout;
	const char *label;

	node = ofnode_path("/mtd-layout");
	if (!ofnode_valid(node)) {
		return ofnode_null();
	}

	if (!ofnode_get_child_count(node)) {
		return ofnode_null();
	}

	ofnode_for_each_subnode(layout, node) {
		label = ofnode_read_string(layout, "label");
		if (!strcmp(layout_label, label)) {
			return layout;
		}
	}

	return ofnode_null();
}

const char *get_mtd_layout_label(void)
{
	const char *layout_label = NULL;

	if (gd->flags & GD_FLG_ENV_READY)
		layout_label = env_get("mtd_layout_label");

	if (!layout_label)
		layout_label = "default";

	return layout_label;
}

void board_mtdparts_default(const char **mtdids, const char **mtdparts)
{
	const char *ids = NULL;
	const char *parts = NULL;
	const char *boot_part = NULL;
	const char *factory_part = NULL;
	const char *sysupgrade_kernel_ubipart = NULL;
	const char *sysupgrade_rootfs_ubipart = NULL;
	const char *cmdline = NULL;
	ofnode layout_node;

	layout_node = ofnode_get_mtd_layout(get_mtd_layout_label());

	if (ofnode_valid(layout_node)) {
		ids = ofnode_read_string(layout_node, "mtdids");
		parts = ofnode_read_string(layout_node, "mtdparts");
		boot_part = ofnode_read_string(layout_node, "boot_part");
		factory_part = ofnode_read_string(layout_node, "factory_part");
		sysupgrade_kernel_ubipart = ofnode_read_string(layout_node, "sysupgrade_kernel_ubipart");
		sysupgrade_rootfs_ubipart = ofnode_read_string(layout_node, "sysupgrade_rootfs_ubipart");
		cmdline = ofnode_read_string(layout_node, "cmdline");
	}

	if (ids && parts) {
		*mtdids = ids;
		*mtdparts = parts;
		//printf("%s: mtdids=%s & mtdparts=%s\n", __func__, ids, parts);
	}

	env_set("bootargs", cmdline);
	env_set("ubi_boot_part", boot_part);
	env_set("factory_part", factory_part);
	env_set("sysupgrade_kernel_ubipart", sysupgrade_kernel_ubipart);
	env_set("sysupgrade_rootfs_ubipart", sysupgrade_rootfs_ubipart);
}
