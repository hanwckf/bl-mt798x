/* SPDX-License-Identifier:	GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 */

#include <net.h>
#include <rand.h>
#include <common.h>
#include <malloc.h>
#include <command.h>
#include <env.h>
#ifdef CONFIG_CMD_GL_BTN
#include <glbtn.h>
#endif
#include <net/tcp.h>
#include <net/httpd.h>
#include <u-boot/md5.h>
#include <linux/stringify.h>
#include <dm/ofnode.h>
#include <version_string.h>

#include "../board/mediatek/common/boot_helper.h"
#include "fs.h"

enum {
	FW_TYPE_GPT,
	FW_TYPE_BL2,
	FW_TYPE_FIP,
	FW_TYPE_FW,
	FW_TYPE_INITRD
};

typedef struct fip_toc_header {
	uint32_t	name;
	uint32_t	serial_number;
	uint64_t	flags;
} fip_toc_header_t;

/* This is used as a signature to validate the blob header */
#define TOC_HEADER_NAME	0xAA640001

static u32 upload_data_id;
static const void *upload_data;
static size_t upload_size;
static int upgrade_success;
static int fw_type;

#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
static const char *mtd_layout_label;
const char *get_mtd_layout_label(void);
#endif

struct data_part_entry;

#if defined(CONFIG_MT7981_BOOTMENU_EMMC) || defined(CONFIG_MT7986_BOOTMENU_EMMC)
int write_gpt(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size);
#endif

int write_bl2(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size);

int write_fip(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size);

int write_firmware(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);

static bool verify_fip(const void *data, size_t size)
{
	fip_toc_header_t *header = (fip_toc_header_t *)data;

	if (size < sizeof(fip_toc_header_t))
		return false;

	return header->name == TOC_HEADER_NAME;
}

static int write_firmware_failsafe(size_t data_addr, uint32_t data_size)
{
	int r;

	led_control("ledblink", "blink_led", "100");

	switch (fw_type) {
#if defined(CONFIG_MT7981_BOOTMENU_EMMC) || defined(CONFIG_MT7986_BOOTMENU_EMMC)
	case FW_TYPE_GPT:
		r = write_gpt(NULL, NULL, (const void *)data_addr, data_size);
		break;
#endif

	case FW_TYPE_BL2:
		r = write_bl2(NULL, NULL, (const void *)data_addr, data_size);
		break;

	case FW_TYPE_FIP:
		if (!verify_fip((const void *)data_addr, data_size))
			return -1;
		r = write_fip(NULL, NULL, (const void *)data_addr, data_size);
		break;

	default:
		r = write_firmware(NULL, NULL, (const void *)data_addr, data_size);
		break;
	}

	led_control("ledblink", "blink_led", "0");

	return r;
}

static int output_plain_file(struct httpd_response *response,
	const char *filename)
{
	const struct fs_desc *file;
	int ret = 0;

	file = fs_find_file(filename);

	response->status = HTTP_RESP_STD;

	if (file) {
		response->data = file->data;
		response->size = file->size;
	} else {
		response->data = "Error: file not found";
		response->size = strlen(response->data);
		ret = 1;
	}

	response->info.code = 200;
	response->info.connection_close = 1;
	response->info.content_type = "text/html";

	return ret;
}

static void not_found_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	if (status == HTTP_CB_NEW) {
		output_plain_file(response, "404.html");
		response->info.code = 404;
	}
}

static void index_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	if (status == HTTP_CB_NEW)
		output_plain_file(response, "index.html");
}

static void html_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	if (status != HTTP_CB_NEW)
		return;

	if (output_plain_file(response, request->urih->uri + 1))
		not_found_handler(status, request, response);
}

static void version_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	if (status != HTTP_CB_NEW)
		return;

	response->status = HTTP_RESP_STD;

	response->data = version_string;
	response->size = strlen(response->data);

	response->info.code = 200;
	response->info.connection_close = 1;
	response->info.content_type = "text/plain";
}

#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
#define MTD_LAYOUTS_MAXLEN	128

static void mtd_layout_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	static char mtd_layout_str[MTD_LAYOUTS_MAXLEN];
	ofnode node, layout;

	if (status != HTTP_CB_NEW)
		return;

	sprintf(mtd_layout_str, "%s;", get_mtd_layout_label());

	node = ofnode_path("/mtd-layout");
	if (ofnode_valid(node) && ofnode_get_child_count(node)) {
		ofnode_for_each_subnode(layout, node) {
			strcat(mtd_layout_str, ofnode_read_string(layout, "label"));
			strcat(mtd_layout_str, ";");
		}
	}

	response->status = HTTP_RESP_STD;

	response->data = mtd_layout_str;
	response->size = strlen(response->data);

	response->info.code = 200;
	response->info.connection_close = 1;
	response->info.content_type = "text/plain";
}
#else
static void mtd_layout_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	if (status != HTTP_CB_NEW)
		return;

	response->status = HTTP_RESP_STD;

	response->data = "error";
	response->size = strlen(response->data);

	response->info.code = 200;
	response->info.connection_close = 1;
	response->info.content_type = "text/plain";
}
#endif

static void upload_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	static char hexchars[] = "0123456789abcdef";
	struct httpd_form_value *fw;
#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
	struct httpd_form_value *mtd = NULL;
#endif
	static char md5_str[33] = "";
	static char resp[128];
	u8 md5_sum[16];
	int i;

	if (status != HTTP_CB_NEW)
		return;

	response->status = HTTP_RESP_STD;
	response->info.code = 200;
	response->info.connection_close = 1;
	response->info.content_type = "text/plain";

	fw = httpd_request_find_value(request, "gpt");
	if (fw) {
		fw_type = FW_TYPE_GPT;
		goto done;
	}

	fw = httpd_request_find_value(request, "bl2");
	if (fw) {
		fw_type = FW_TYPE_BL2;
		goto done;
	}

	fw = httpd_request_find_value(request, "fip");
	if (fw) {
		if (!verify_fip(fw->data, fw->size)) {
			response->data = "fail";
			goto fail;
		}
		fw_type = FW_TYPE_FIP;
		goto done;
	}

	fw = httpd_request_find_value(request, "initramfs");
	if (fw) {
		fw_type = FW_TYPE_INITRD;
		if (fdt_check_header(fw->data))
			goto fail;
		goto done;
	}

	fw = httpd_request_find_value(request, "firmware");
	if (fw) {
		fw_type = FW_TYPE_FW;
#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
		mtd = httpd_request_find_value(request, "mtd_layout");
#endif
		goto done;
	}

fail:
	response->data = "fail";
	response->size = strlen(response->data);
	return;

done:
	upload_data_id = upload_id;
	upload_data = fw->data;
	upload_size = fw->size;

	md5((u8 *)fw->data, fw->size, md5_sum);

	for (i = 0; i < 16; i++) {
		u8 hex = (md5_sum[i] >> 4) & 0xf;
		md5_str[i * 2] = hexchars[hex];
		hex = md5_sum[i] & 0xf;
		md5_str[i * 2 + 1] = hexchars[hex];
	}

#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
	if (mtd) {
		mtd_layout_label = mtd->data;
		sprintf(resp, "%ld %s %s", fw->size, md5_str, mtd->data);
	} else {
		sprintf(resp, "%ld %s", fw->size, md5_str);
	}
#else
	sprintf(resp, "%ld %s", fw->size, md5_str);
#endif

	response->data = resp;
	response->size = strlen(response->data);
}

struct flashing_status {
	char buf[4096];
	int ret;
	int body_sent;
};

static void result_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	struct flashing_status *st;
	u32 size;

	if (status == HTTP_CB_NEW) {
		st = calloc(1, sizeof(*st));
		if (!st) {
			response->info.code = 500;
			return;
		}

		st->ret = -1;

		response->session_data = st;

		response->status = HTTP_RESP_CUSTOM;

		response->info.http_1_0 = 1;
		response->info.content_length = -1;
		response->info.connection_close = 1;
		response->info.content_type = "text/html";
		response->info.code = 200;

		size = http_make_response_header(&response->info,
			st->buf, sizeof(st->buf));

		response->data = st->buf;
		response->size = size;

		return;
	}

	if (status == HTTP_CB_RESPONDING) {
		st = response->session_data;

		if (st->body_sent) {
			response->status = HTTP_RESP_NONE;
			return;
		}

		if (upload_data_id == upload_id) {
#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
			if (mtd_layout_label &&
					strcmp(get_mtd_layout_label(), mtd_layout_label) != 0) {
				printf("httpd: saving mtd_layout_label: %s\n", mtd_layout_label);
				env_set("mtd_layout_label", mtd_layout_label);
				env_save();
			}
#endif
			if (fw_type == FW_TYPE_INITRD)
				st->ret = 0;
			else
				st->ret = write_firmware_failsafe((size_t) upload_data,
					upload_size);
		}

		/* invalidate upload identifier */
		upload_data_id = rand();

		led_control("led", "blink_led", "on");
		led_control("led", "system_led", "off");

		if (!st->ret)
			response->data = "success";
		else
			response->data = "failed";

		response->size = strlen(response->data);
		st->body_sent = 1;
		return;
	}

	if (status == HTTP_CB_CLOSED) {
		st = response->session_data;

		upgrade_success = !st->ret;

		free(response->session_data);

		if (upgrade_success)
			tcp_close_all_conn();
	}
}

static void style_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	if (status == HTTP_CB_NEW) {
		output_plain_file(response, "style.css");
		response->info.content_type = "text/css";
	}
}

static void js_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	if (status == HTTP_CB_NEW) {
		output_plain_file(response, "index.js");
		response->info.content_type = "text/javascript";
	}
}

int start_web_failsafe(void)
{
	struct httpd_instance *inst;

	inst = httpd_find_instance(80);
	if (inst)
		httpd_free_instance(inst);

	inst = httpd_create_instance(80);
	if (!inst) {
		printf("Error: failed to create HTTP instance on port 80\n");
		return -1;
	}

	httpd_register_uri_handler(inst, "/", &index_handler, NULL);
	httpd_register_uri_handler(inst, "/bl2.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/booting.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/cgi-bin/luci", &index_handler, NULL);
	httpd_register_uri_handler(inst, "/cgi-bin/luci/", &index_handler, NULL);
	httpd_register_uri_handler(inst, "/fail.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/flashing.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/getmtdlayout", &mtd_layout_handler, NULL);
#if defined(CONFIG_MT7981_BOOTMENU_EMMC) || defined(CONFIG_MT7986_BOOTMENU_EMMC)
        httpd_register_uri_handler(inst, "/gpt.html", &html_handler, NULL);
#endif
	httpd_register_uri_handler(inst, "/index.js", &js_handler, NULL);
	httpd_register_uri_handler(inst, "/initramfs.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/result", &result_handler, NULL);
	httpd_register_uri_handler(inst, "/style.css", &style_handler, NULL);
	httpd_register_uri_handler(inst, "/uboot.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/upload", &upload_handler, NULL);
	httpd_register_uri_handler(inst, "/version", &version_handler, NULL);
	httpd_register_uri_handler(inst, "", &not_found_handler, NULL);

	net_loop(TCP);

	return 0;
}

static int do_httpd(struct cmd_tbl *cmdtp, int flag, int argc,
	char *const argv[])
{
	u32 local_ip;
	int ret;

#ifdef CONFIG_NET_FORCE_IPADDR
	net_ip = string_to_ip(__stringify(CONFIG_IPADDR));
	net_netmask = string_to_ip(__stringify(CONFIG_NETMASK));
#endif
	local_ip = ntohl(net_ip.s_addr);

	printf("\nWeb failsafe UI started\n");
	printf("URL: http://%u.%u.%u.%u/\n",
	       (local_ip >> 24) & 0xff, (local_ip >> 16) & 0xff,
	       (local_ip >> 8) & 0xff, local_ip & 0xff);
	printf("\nPress Ctrl+C to exit\n");

	ret = start_web_failsafe();

	if (upgrade_success) {
		if (fw_type == FW_TYPE_INITRD)
			boot_from_mem((ulong)upload_data);
		else
			do_reset(NULL, 0, 0, NULL);
	}

	return ret;
}

U_BOOT_CMD(httpd, 1, 0, do_httpd,
	"Start failsafe HTTP server", ""
);
