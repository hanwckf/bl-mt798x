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
#include <net/tcp.h>
#include <net/httpd.h>
#include <u-boot/md5.h>
#include <version_string.h>

#include "fs.h"

enum {
	FW_TYPE_GPT,
	FW_TYPE_BL2,
	FW_TYPE_FIP,
	FW_TYPE_FW
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

struct data_part_entry;

#ifdef CONFIG_MT7981_BOOTMENU_EMMC
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

	run_command("ledblink blue:run 100", 0);

	switch (fw_type) {
#ifdef CONFIG_MT7981_BOOTMENU_EMMC
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

	run_command("ledblink blue:run 0", 0);

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

static void html_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	if (status != HTTP_CB_NEW)
		return;

	if (!strcmp(request->urih->uri, "/"))
		output_plain_file(response, "index.html");
	else if (output_plain_file(response, request->urih->uri + 1))
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

static void upload_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response)
{
	static char hexchars[] = "0123456789abcdef";
	struct httpd_form_value *fw;
	static char md5_str[33] = "";
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

	fw = httpd_request_find_value(request, "firmware");
	if (fw) {
		fw_type = FW_TYPE_FW;
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

	response->data = md5_str;
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

		if (upload_data_id == upload_id)
			st->ret = write_firmware_failsafe((size_t) upload_data,
				upload_size);

		/* invalidate upload identifier */
		upload_data_id = rand();

		run_command("led blue:run on", 0);
		run_command("led white:system off", 0);

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

	httpd_register_uri_handler(inst, "/", &html_handler, NULL);
#ifdef CONFIG_MT7981_BOOTMENU_EMMC
        httpd_register_uri_handler(inst, "/gpt.html", &html_handler, NULL);
#endif
	httpd_register_uri_handler(inst, "/bl2.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/uboot.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/fail.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/flashing.html", &html_handler, NULL);
	httpd_register_uri_handler(inst, "/version", &version_handler, NULL);
	httpd_register_uri_handler(inst, "/upload", &upload_handler, NULL);
	httpd_register_uri_handler(inst, "/result", &result_handler, NULL);
	httpd_register_uri_handler(inst, "/style.css", &style_handler, NULL);
	httpd_register_uri_handler(inst, "/index.js", &js_handler, NULL);
	httpd_register_uri_handler(inst, "", &not_found_handler, NULL);

	net_loop(TCP);

	return 0;
}

static int do_httpd(struct cmd_tbl *cmdtp, int flag, int argc,
	char *const argv[])
{
	int ret;

	printf("\nWeb failsafe UI started\n");

	ret = start_web_failsafe();

	if (upgrade_success)
		do_reset(NULL, 0, 0, NULL);

	return ret;
}

U_BOOT_CMD(httpd, 1, 0, do_httpd,
	"Start failsafe HTTP server", ""
);
