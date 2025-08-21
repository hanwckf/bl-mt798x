// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Simple HTTP server implementation
 */

#include <cyclic.h>
#include <errno.h>
#include <malloc.h>
#include <net.h>
#include <net/mtk_tcp.h>
#include <net/mtk_httpd.h>
#include <vsprintf.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

struct httpd_instance {
	struct list_head node;

	u16 port;
	struct list_head uri_handlers;
};

struct _httpd_uri_handler {
	struct list_head node;

	struct httpd_uri_handler urih;
};

enum httpd_session_status {
	HTTPD_S_NEW = 0,
	HTTPD_S_HEADER_RECVING,
	HTTPD_S_PAYLOAD_RECVING,
	HTTPD_S_FULL_RCVD,
	HTTPD_S_RESPONDING,
	HTTPD_S_CLOSING
};

struct httpd_mtk_tcp_pdata {
	enum httpd_session_status status;

	char buf[4096];
	u32 bufsize;

	char *uri;
	char *boundary;

	int is_uploading;
	char *upload_ptr;
	u32 payload_size;
	u32 upload_size;

	struct httpd_request request;
	struct httpd_response response;

	int resp_std_cnt;
};

struct http_response_code {
	u32 code;
	const char *text;
};

static struct http_response_code http_resp_codes[] = {
	{ 200, "OK" },
	{ 302, "Found" },
	{ 307, "Temporary Redirect" },
	{ 400, "Bad Request" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 413, "Request Entity Too Large" },
	{ 431, "Request Header Fields Too Large" },
	{ 500, "Internal Server Error" },
	{ 503, "Service Unavailable" }
};

u32 upload_id = (u32) -1;

static int is_uploading;
static LIST_HEAD(inst_head);

static void httpd_mtk_tcp_callback(struct mtk_tcp_cb_data *cbd);
static void httpd_std_err_response(struct mtk_tcp_cb_data *cbd, u32 code);

void __weak *httpd_get_upload_buffer_ptr(size_t size)
{
	return (void *)gd->ram_base;
}

static void dummy_urih_cb(enum httpd_uri_handler_status status,
			  struct httpd_request *request,
			  struct httpd_response *response)
{
}

static struct httpd_uri_handler dummy_urih = {
	.uri = "",
	.cb = dummy_urih_cb
};

struct httpd_instance *httpd_find_instance(u16 port)
{
	struct list_head *lh;
	struct httpd_instance *i;

	list_for_each(lh, &inst_head) {
		i = list_entry(lh, struct httpd_instance, node);
		if (i->port == port)
			return i;
	}

	return NULL;
}

struct httpd_instance *httpd_create_instance(u16 port)
{
	struct httpd_instance *inst;

	if (httpd_find_instance(port))
		return NULL;

	inst = malloc(sizeof(*inst));
	if (!inst)
		return NULL;

	inst->port = port;
	INIT_LIST_HEAD(&inst->uri_handlers);

	if (mtk_tcp_listen(htons(port), httpd_mtk_tcp_callback)) {
		free(inst);
		return NULL;
	}

	list_add_tail(&inst->node, &inst_head);

	return inst;
}

void httpd_free_instance(struct httpd_instance *httpd_inst)
{
	struct list_head *lh, *n;
	struct httpd_instance *inst;
	struct _httpd_uri_handler *u;

	mtk_tcp_listen_stop(htons(httpd_inst->port));

	inst = httpd_find_instance(httpd_inst->port);
	if (inst) {
		list_del(&inst->node);

		list_for_each_safe(lh, n, &inst->uri_handlers) {
			u = list_entry(lh, struct _httpd_uri_handler, node);
			list_del(&u->node);
			free(u);
		}

		free(inst);
	}
}

int httpd_free_instance_by_port(u16 port)
{
	struct httpd_instance *inst;

	inst = httpd_find_instance(port);
	if (inst) {
		httpd_free_instance(inst);
		return 0;
	}

	return -EINVAL;
}

int httpd_register_uri_handler(struct httpd_instance *httpd_inst,
			       const char *uri,
			       httpd_uri_handler_cb cb,
			       struct httpd_uri_handler **returih)
{
	struct _httpd_uri_handler *u;

	if (!httpd_inst || !uri || !cb)
		return -EINVAL;

	u = calloc(1, sizeof(*u));
	if (!u)
		return -ENOMEM;

	u->urih.uri = uri;
	u->urih.cb = cb;

	list_add_tail(&u->node, &httpd_inst->uri_handlers);

	if (returih)
		*returih = &u->urih;

	return 0;
}

int httpd_unregister_uri_handler(struct httpd_instance *httpd_inst,
				 struct httpd_uri_handler *urih)
{
	struct list_head *lh, *n;
	struct _httpd_uri_handler *u;

	if (!httpd_inst || !urih)
		return -EINVAL;

	list_for_each_safe(lh, n, &httpd_inst->uri_handlers) {
		u = list_entry(lh, struct _httpd_uri_handler, node);
		if (&u->urih == urih) {
			list_del(&u->node);
			free(u);
			break;
		}
	}

	return 0;
}

struct httpd_uri_handler *httpd_find_uri_handler(
	struct httpd_instance *httpd_inst, const char *uri)
{
	struct list_head *lh;
	struct _httpd_uri_handler *u;

	if (!httpd_inst || !uri)
		return NULL;

	list_for_each(lh, &httpd_inst->uri_handlers) {
		u = list_entry(lh, struct _httpd_uri_handler, node);
		if (!strcmp(u->urih.uri, uri))
			return &u->urih;
	}

	list_for_each(lh, &httpd_inst->uri_handlers) {
		u = list_entry(lh, struct _httpd_uri_handler, node);
		if (!strcmp(u->urih.uri, ""))
			return &u->urih;
	}

	return NULL;
}

u32 http_make_response_header(struct http_response_info *info, char *buff,
			      u32 size)
{
	int i, http_ver = 1;
	char *p = buff;

	if (info->http_1_0)
		http_ver = 0;

	for (i = 0; i < ARRAY_SIZE(http_resp_codes); i++) {
		if (http_resp_codes[i].code == info->code) {
			p += snprintf(p, buff + size - p,
				      "HTTP/1.%d %u %s\r\n", http_ver,
				      info->code, http_resp_codes[i].text);
			break;
		}
	}

	if (p == buff)
		p += snprintf(p, buff + size - p, "HTTP/1.%d %u\r\n",
			      http_ver, info->code);

	if (p >= buff + size)
		return size;

	if (info->content_type)
		p += snprintf(p, buff + size - p, "Content-Type: %s\r\n",
			      info->content_type);

	if (p >= buff + size)
		return size;

	if (info->content_length >= 0)
		p += snprintf(p, buff + size - p, "Content-Length: %d\r\n",
			      info->content_length);

	if (p >= buff + size)
		return size;

	if (info->location)
		p += snprintf(p, buff + size - p, "Location: %s\r\n",
			      info->location);

	if (p >= buff + size)
		return size;

	if (info->chunked_encoding)
		p += snprintf(p, buff + size - p,
			      "Transfer-Encoding: chunked\r\n");

	if (p >= buff + size)
		return size;

	if (info->connection_close)
		p += snprintf(p, buff + size - p, "Connection: close\r\n");

	if (p >= buff + size)
		return size;

	p += snprintf(p, buff + size - p, "\r\n");

	return p - buff;
}

static int httpd_recv_hdr(struct httpd_instance *inst,
			  struct mtk_tcp_cb_data *cbd)
{
	struct httpd_mtk_tcp_pdata *pdata = cbd->pdata;
	char *p, *payload_ptr, *uri_ptr, *fields_ptr;
	char *cl_ptr, *ct_ptr, *b_ptr;
	enum httpd_request_method method;
	u32 size_rcvd, hdr_size, err_code = 400;
	int ret = 0;

	static const char content_length_str[] = "Content-Length:";
	static const char content_type_str[] = "Content-Type:";
	static const char boundary_str[] = "boundary=";

	/* copy TCP data into cache */
	size_rcvd = min_t(size_t, cbd->datalen,
			  sizeof(pdata->buf) - pdata->bufsize - 1);

	memcpy(pdata->buf + pdata->bufsize, cbd->data, size_rcvd);
	pdata->bufsize += size_rcvd;
	pdata->buf[pdata->bufsize] = 0;

	/* check if request header has been fully received*/
	payload_ptr = strstr(pdata->buf, "\r\n\r\n");
	if (!payload_ptr) {
		/* not fully received, waiting for next rx */
		if (size_rcvd == cbd->datalen)
			return 1;

		/* request entity too large */
		err_code = 413;
		goto bad_request;
	}

	/* start of payload */
	*payload_ptr = 0;
	payload_ptr += 4;

	/* accurate size of request header */
	hdr_size = payload_ptr - pdata->buf;

	/* parse HTTP response header */
	fields_ptr = strstr(pdata->buf, "\r\n");
	if (!fields_ptr)
		goto bad_request;

	/* start of header fields */
	*fields_ptr = 0;
	fields_ptr += 2;

	/* parse method & uri */
	p = strchr(pdata->buf, ' ');
	if (!p)
		goto bad_request;

	*p = 0;
	uri_ptr = p + 1;

	/* check method */
	if (!strcmp(pdata->buf, "GET")) {
		method = HTTP_GET;
	} else if (!strcmp(pdata->buf, "POST")) {
		method = HTTP_POST;
	} else {
		err_code = 405;
		goto bad_request;
	}

	/* extract uri */
	p = strchr(uri_ptr, ' ');
	if (!p)
		goto bad_request;

	*p = 0;

	/* find ? and remove query string */
	p = strchr(uri_ptr, '?');
	if (p)
		*p = 0;

	printf("%s %s\n", pdata->buf, uri_ptr);

	/* record URI */
	pdata->uri = uri_ptr;

	/* find required fields if this is a POST request */
	if (method == HTTP_POST) {
		/* Content-Length */
		cl_ptr = strstr(fields_ptr, content_length_str);
		if (cl_ptr) {
			cl_ptr += sizeof(content_length_str) - 1;
			while (*cl_ptr == ' ')
				cl_ptr++;
			pdata->payload_size = simple_strtoul(cl_ptr, NULL, 10);
			printf("    Content-Length: %d\n", pdata->payload_size);
		}

		/* Content-Type */
		ct_ptr = strstr(fields_ptr, content_type_str);
		if (ct_ptr) {
			p = strstr(ct_ptr, "\r\n");
			if (p)
				*p = 0;

			b_ptr = strstr(ct_ptr, boundary_str);
			/* only accept multipart/form-data */
			if (!b_ptr)
				goto bad_request;

			b_ptr += sizeof(boundary_str) - 1;

			if (*b_ptr == '\"') {
				b_ptr++;
				p = strchr(b_ptr, '\"');
				if (p)
					*p = 0;
			} else {
				p = strchr(b_ptr, '\r');
				if (p)
					*p = 0;
			}

			pdata->boundary = b_ptr;
			debug("    Content-Type: boundary=\"%s\"\n", b_ptr);
		}

		if (hdr_size + pdata->payload_size < sizeof(pdata->buf)) {
			/* upload payload can be put into the cache */
			pdata->upload_ptr = pdata->buf + hdr_size;
			pdata->upload_size = pdata->bufsize - hdr_size;
		} else {
			/* upload payload must be put into unused ram region */
			if (is_uploading) {
				printf("Only one upload can be performed\n");
				mtk_tcp_close_conn(cbd->conn, 1);
				return 1;
			}

			/* generate new upload identifier */
			upload_id = rand();

			/* calculate new cache address */
			pdata->upload_ptr = httpd_get_upload_buffer_ptr(
				pdata->payload_size);
			pdata->upload_size = cbd->datalen - hdr_size;
			/* copy received parts to new cache */
			memcpy(pdata->upload_ptr, cbd->data + hdr_size,
			       pdata->upload_size);
		}

		if (pdata->upload_size == pdata->payload_size) {
			/* upload completed */
			pdata->upload_ptr[pdata->payload_size] = 0;
			pdata->status = HTTPD_S_FULL_RCVD;
		} else {
			/* switch status for further receving */
			pdata->status = HTTPD_S_PAYLOAD_RECVING;
			/* uploading mark */
			pdata->is_uploading = 1;
			is_uploading = 1;
			/*
			 * payload of current packet has been fully received,
			 * stop going to next status.
			 */
			ret = 1;
		}
	} else {
		pdata->status = HTTPD_S_FULL_RCVD;
	}

	pdata->request.method = method;

	return ret;

bad_request:
	httpd_std_err_response(cbd, err_code);
	return 1;
}

static int httpd_recv_payload(struct httpd_instance *inst,
			      struct mtk_tcp_cb_data *cbd)
{
	struct httpd_mtk_tcp_pdata *pdata = cbd->pdata;
	u32 size_recv;

	size_recv = min(pdata->payload_size - pdata->upload_size, cbd->datalen);
	memcpy(pdata->upload_ptr + pdata->upload_size, cbd->data, size_recv);
	pdata->upload_size += size_recv;

	if (pdata->upload_size == pdata->payload_size) {
		pdata->upload_ptr[pdata->payload_size] = 0;
		pdata->status = HTTPD_S_FULL_RCVD;
		/* remove uploading mark */
		pdata->is_uploading = 0;
		is_uploading = 0;
		return 0;
	}

	return 1;
}

static void *memstr(void *src, size_t limit, const char *str)
{
	int l2;
	char *s1 = src;

	if (!str)
		return NULL;

	l2 = strlen(str);

	while (limit >= l2) {
		limit--;
		if (!memcmp(s1, str, l2))
			return s1;
		s1++;
	}
	return NULL;
}

static char *name_extract(char *s)
{
	char *name, *p;

	if (*s == '\"') {
		s++;
		name = s;
		p = strchr(s, '\"');
		if (p)
			*p = 0;
	} else {
		name = s;
		p = strchr(s, '\r');
		if (p)
			*p = 0;
	}

	return name;
}

static int httpd_handle_request(struct httpd_instance *inst,
				 struct mtk_tcp_cb_data *cbd)
{
	struct httpd_mtk_tcp_pdata *pdata = cbd->pdata;
	struct httpd_request *req = &pdata->request;
	char *formdata[MAX_HTTP_FORM_VALUE_ITEMS];
	char *formdata_end[MAX_HTTP_FORM_VALUE_ITEMS], *p, *payload_end;
	char *boundary, *name_ptr, *filename_ptr;
	u32 i, numformdata = 0, boundarylen;
	struct httpd_form_value *val;

	static const char name_str[] = "name=";
	static const char filename_str[] = "filename=";

	schedule();

	req->urih = httpd_find_uri_handler(inst, pdata->uri);
	if (!req->urih) {
		/* TODO: no handler / response 404 */
		mtk_tcp_close_conn(cbd->conn, 1);
		return 1;
	}

	if (req->method == HTTP_POST) {
		boundarylen = strlen(pdata->boundary);
		boundary = malloc(boundarylen + 3);
		if (!boundary) {
			/* out of memory */
			mtk_tcp_close_conn(cbd->conn, 1);
			return 1;
		}

		/* add prefix -- */
		boundary[0] = boundary[1] = '-';
		memcpy(boundary + 2, pdata->boundary, boundarylen + 1);
		boundarylen += 2;

		/* record and split each formdata */
		p = pdata->upload_ptr;
		payload_end = pdata->upload_ptr + pdata->payload_size;
		do {
			p += boundarylen;
			if (strncmp(p, "\r\n", 2))
				break;

			p += 2;

			formdata[numformdata] = p;

			p = memstr(p, payload_end - p, boundary);
			if (!p)
				break;

			if (!strncmp(p - 2, "\r\n", 2))
				p[-2] = 0;
			else
				*p = 0;

			formdata_end[numformdata] = p - 2;
			numformdata++;
		} while (numformdata < MAX_HTTP_FORM_VALUE_ITEMS);

		/* process each formdata */
		for (i = 0; i < numformdata; i++) {
			val = &req->form.values[req->form.count];
			p = strstr(formdata[i], "\r\n\r\n");
			if (!p)
				continue;

			*p = 0;
			p += 4;

			/*
			 * make the data aligned by 4.
			 * the previous 3 CR/LFs can be used as buffer.
			 */
			val->data = (char *)(((uintptr_t)p) & (~(4 - 1)));

			name_ptr = strstr(formdata[i], name_str);
			filename_ptr = strstr(formdata[i], filename_str);

			if (name_ptr) {
				name_ptr += sizeof(name_str) - 1;
				val->name = name_extract(name_ptr);
			}

			if (filename_ptr) {
				filename_ptr += sizeof(filename_str) - 1;
				val->filename = name_extract(filename_ptr);
			}

			val->size = formdata_end[i] - p;
			req->form.count++;

			/* move data if not aligned */
			if (p != val->data) {
				memmove((char *)val->data, p, val->size);
				((char *)val->data)[val->size] = 0;
			}
		}

		free(boundary);
	}

	/* call uri handler */
	assert((size_t)req->urih->cb > gd->ram_base);
	req->urih->cb(HTTP_CB_NEW, req, &pdata->response);

	if (pdata->response.status == HTTP_RESP_NONE) {
		mtk_tcp_close_conn(cbd->conn, 0);
		return 1;
	}

	if (pdata->response.status == HTTP_RESP_STD) {
		/* generate HTTP response header */
		u32 size;

		pdata->response.info.content_length = pdata->response.size;

		size = http_make_response_header(&pdata->response.info,
						 pdata->buf,
						 sizeof(pdata->buf));

		/* send response header */
		mtk_tcp_send_data(cbd->conn, pdata->buf, size);

		pdata->resp_std_cnt = 0;
	} else {
		/* send first response data */
		mtk_tcp_send_data(cbd->conn, pdata->response.data,
			      pdata->response.size);
	}

	pdata->status = HTTPD_S_RESPONDING;

	return 0;
}

static void httpd_rx(struct httpd_instance *inst, struct mtk_tcp_cb_data *cbd)
{
	struct httpd_mtk_tcp_pdata *pdata = cbd->pdata;
	u8 sip[4];

	if (pdata->status == HTTPD_S_NEW) {
		memcpy(sip, &cbd->sip, 4);
		debug("New connection from %d.%d.%d.%d:%d\n",
		      sip[0], sip[1], sip[2], sip[3], ntohs(cbd->sp));
		pdata->status = HTTPD_S_HEADER_RECVING;
	}

	if (pdata->status == HTTPD_S_HEADER_RECVING)
		if (httpd_recv_hdr(inst, cbd))
			return;

	if (pdata->status == HTTPD_S_PAYLOAD_RECVING)
		if (httpd_recv_payload(inst, cbd))
			return;

	if (pdata->status == HTTPD_S_FULL_RCVD)
		httpd_handle_request(inst, cbd);
}

static void httpd_tx(struct httpd_instance *inst, struct mtk_tcp_cb_data *cbd)
{
	struct httpd_mtk_tcp_pdata *pdata = cbd->pdata;
	struct httpd_request *req = &pdata->request;
	struct httpd_response *resp = &pdata->response;

	if (resp->status == HTTP_RESP_STD) {
		if (pdata->resp_std_cnt == 0) {
			/* send response payload */
			mtk_tcp_send_data(cbd->conn, pdata->response.data,
				      pdata->response.size);

			pdata->resp_std_cnt = 1;
		} else {
			mtk_tcp_close_conn(cbd->conn, 0);
			pdata->status = HTTPD_S_CLOSING;
		}

		return;
	}

	/* call uri handler */
	assert((size_t)req->urih->cb > gd->ram_base);
	req->urih->cb(HTTP_CB_RESPONDING, req, &pdata->response);

	if (pdata->response.status == HTTP_RESP_NONE) {
		mtk_tcp_close_conn(cbd->conn, 0);
		pdata->status = HTTPD_S_CLOSING;
	} else {
		/* send next response data */
		mtk_tcp_send_data(cbd->conn, pdata->response.data,
			      pdata->response.size);
	}
}

static void httpd_cleanup(struct httpd_instance *inst, struct mtk_tcp_cb_data *cbd)
{
	struct httpd_mtk_tcp_pdata *pdata = cbd->pdata;
	struct httpd_request *req = &pdata->request;
	struct httpd_response *resp = &pdata->response;

	if (pdata->is_uploading)
		is_uploading = 0;

	/* call uri handler */
	if (req->urih) {
		assert((size_t)req->urih->cb > gd->ram_base);
		req->urih->cb(HTTP_CB_CLOSED, req, resp);
	}

	free(pdata);
}

static void httpd_mtk_tcp_callback(struct mtk_tcp_cb_data *cbd)
{
	struct httpd_instance *inst;

	inst = httpd_find_instance(ntohs(cbd->dp));
	if (!inst) {
		mtk_tcp_close_conn(cbd->conn, 1);
		return;
	}

	if (cbd->status == MTK_TCP_CB_NEW_CONN) {
		cbd->pdata = calloc(1, sizeof(struct httpd_mtk_tcp_pdata));
		if (!cbd->pdata) {
			mtk_tcp_close_conn(cbd->conn, 1);
			return;
		}

		mtk_tcp_conn_set_pdata(cbd->conn, cbd->pdata);
	}

	switch (cbd->status) {
	case MTK_TCP_CB_NEW_CONN:
	case MTK_TCP_CB_DATA_RCVD:
		if (cbd->datalen)
			httpd_rx(inst, cbd);
		break;
	case MTK_TCP_CB_DATA_SENT:
		httpd_tx(inst, cbd);
		break;
	case MTK_TCP_CB_REMOTE_CLOSED:
	case MTK_TCP_CB_CLOSED:
		httpd_cleanup(inst, cbd);
		break;
	default:
		;
	}
}

static void httpd_std_err_response(struct mtk_tcp_cb_data *cbd, u32 code)
{
	struct httpd_mtk_tcp_pdata *pdata = cbd->pdata;
	const char *body = NULL;
	u32 size, i;

	for (i = 0; i < ARRAY_SIZE(http_resp_codes); i++) {
		if (http_resp_codes[i].code == code) {
			body = http_resp_codes[i].text;
			break;
		}
	}

	pdata->status = HTTPD_S_RESPONDING;

	pdata->request.urih = &dummy_urih;
	pdata->response.status = HTTP_RESP_STD;
	pdata->response.data = body;
	if (body)
		pdata->response.size = strlen(body);

	pdata->response.info.code = code;
	pdata->response.info.connection_close = 1;
	pdata->response.info.content_length = pdata->response.size;
	pdata->response.info.content_type = "text/html";

	size = http_make_response_header(&pdata->response.info,
	       pdata->buf,
	       sizeof(pdata->buf));

	/* send response header */
	mtk_tcp_send_data(cbd->conn, pdata->buf, size);

	pdata->resp_std_cnt = body ? 0 : 1;
}

struct httpd_form_value *httpd_request_find_value(
	struct httpd_request *request, const char *name)
{
	u32 i;

	if (!request || !name)
		return NULL;

	for (i = 0; i < request->form.count; i++) {
		if (!strcmp(request->form.values[i].name, name))
			return &request->form.values[i];
	}

	return NULL;
}
