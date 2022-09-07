// SPDX-License-Identifier:	GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 */

#ifndef __NET_HTTPD_H__
#define __NET_HTTPD_H__

#include <linux/list.h>

#define MAX_HTTP_FORM_VALUE_ITEMS	5

struct httpd_form_value {
	const char *name;
	const char *data;
	const char *filename;
	size_t size;
};

struct httpd_form_values {
	u32 count;
	struct httpd_form_value values[MAX_HTTP_FORM_VALUE_ITEMS];
};

struct httpd_instance;
struct httpd_uri_handler;

enum httpd_request_method {
	HTTP_GET,
	HTTP_POST
};

enum httpd_uri_handler_status {
	HTTP_CB_NEW,
	HTTP_CB_RESPONDING,
	HTTP_CB_CLOSED
};

struct httpd_request {
	enum httpd_request_method method;
	const struct httpd_uri_handler *urih;
	struct httpd_form_values form;
};

enum httpd_response_status {
	HTTP_RESP_NONE,
	HTTP_RESP_STD,
	HTTP_RESP_CUSTOM
};

struct http_response_info {
	u32 code;
	const char *content_type;
	int content_length;
	const char *location;
	int connection_close;
	int chunked_encoding;
	int http_1_0;
};

struct httpd_response {
	enum httpd_response_status status;
	struct http_response_info info;
	const char *data;
	u32 size;

	void *session_data;
};

typedef void(*httpd_uri_handler_cb)(enum httpd_uri_handler_status status,
				    struct httpd_request *request,
				    struct httpd_response *response);

struct httpd_uri_handler {
	const char *uri;
	httpd_uri_handler_cb cb;
};

/* Last valid upload identifier */
extern u32 upload_id;

/* Find existing http server instance */
struct httpd_instance *httpd_find_instance(u16 port);

/* Create new http server instance */
struct httpd_instance *httpd_create_instance(u16 port);

/* Free http server instance */
void httpd_free_instance(struct httpd_instance *httpd_inst);

/* Free http server instance by port */
int httpd_free_instance_by_port(u16 port);

/* Register URI handler to a http server instance */
int httpd_register_uri_handler(struct httpd_instance *httpd_inst,
			       const char *uri,
			       httpd_uri_handler_cb cb,
			       struct httpd_uri_handler **returih);

/* Unregister URI handler from a http server instance */
int httpd_unregister_uri_handler(struct httpd_instance *httpd_inst,
				 struct httpd_uri_handler *urih);

/* Find URI handler from a http server instance */
struct httpd_uri_handler *httpd_find_uri_handler(
	struct httpd_instance *httpd_inst, const char *uri);

/* Generate HTTP response header */
u32 http_make_response_header(struct http_response_info *info, char *buff,
			      u32 size);

/* Find request value */
struct httpd_form_value *httpd_request_find_value(
	struct httpd_request *request, const char *name);

#endif /* __NET_HTTPD_H__ */
