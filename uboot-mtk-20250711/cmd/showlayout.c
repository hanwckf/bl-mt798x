// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <dm/ofnode.h>

static int do_showlayout(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    ofnode node, layout;

    node = ofnode_path("/mtd-layout");

    if (ofnode_valid(node) && ofnode_get_child_count(node)) {
        ofnode_for_each_subnode(layout, node) {
            printf("mtd label: %s, mtdids: %s, mtdparts: %s\n", 
                ofnode_read_string(layout, "label"), 
                ofnode_read_string(layout, "mtdids"), 
                ofnode_read_string(layout, "mtdparts"));
        }
    } else {
        printf("get mtd layout failed!\n");
    }

    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	showlayout, 2, 0, do_showlayout,
	"Show mtd layout",
	""
);
