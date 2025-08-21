/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Wget implementation using mtk_tcp
 */

#include <asm/global_data.h>
#include <env.h>
#include <errno.h>
#include <malloc.h>
#include <vsprintf.h>
#include <image.h>
#include <net.h>
#include <net/mtk_tcp.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

enum mtk_wget_state {
	WGET_REQ_SENT,
	WGET_DATA_RECEIVING,
};

struct mtk_wget_pdata {
	struct in_addr ipaddr;
	__be16 port;
	char *host;
	char *url;
	int result;

	enum mtk_wget_state state;
	char *req_hdr;
	char *resp_hdr;
	size_t resp_hdr_len;

	void *data_ptr;
	size_t data_len;
	size_t data_rcvd_len;
	size_t last_percentage;
	bool data_len_unspec;
};

static const char http_end_of_header[] = "\r\n\r\n";
static const char http_end_of_line[] = "\r\n";
static const char content_length_str[] = "Content-Length:";

static int asprintf(char **strp, const char *fmt, ...)
{
	int n, size = 512;	/* start with 512 bytes */
	char *p, *np;
	va_list ap;

	/* initial pointer is NULL making the fist realloc to be malloc */
	p = NULL;
	while (1) {
		np = realloc(p, size);
		if (!np) {
			free(p);
			*strp = NULL;
			return -1;
		}

		p = np;

		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);

		/* If that worked, return the string. */
		if (n > -1 && n < size)
			break;
		/* Else try again with more space. */
		if (n > -1)	/* glibc 2.1 */
			size = n + 1; /* precisely what is needed */
		else		/* glibc 2.0 */
			size *= 2; /* twice the old size */
	}
	*strp = p;
	return strlen(p);
}

static void __wget_cleanup(struct mtk_wget_pdata *pdata)
{
	if (pdata->req_hdr)
		free(pdata->req_hdr);

	if (pdata->resp_hdr)
		free(pdata->resp_hdr);
}

static void wget_initiate(struct mtk_wget_pdata *pdata, struct mtk_tcp_cb_data *cbd)
{
	int ret;

	printf("GET %s HTTP/1.0\n", pdata->url);

	ret = asprintf(&pdata->req_hdr,
		"GET %s HTTP/1.0\r\n"
		"Host: %s\r\n"
		"User-Agent: U-Boot\r\n"
		"Accept: */*\r\n"
		"Connection: close\r\n"
		"\r\n",
		pdata->url, pdata->host);

	if (ret < 0) {
		mtk_tcp_close_conn(cbd->conn, 1);
		printf("No memory for HTTP request header\n");
		pdata->result = -ENOMEM;
		return;
	}

	/* send request header */
	mtk_tcp_send_data(cbd->conn, pdata->req_hdr, strlen(pdata->req_hdr));

	pdata->state = WGET_REQ_SENT;
}

static int wget_recv_data(struct mtk_wget_pdata *pdata, void *data, size_t len)
{
	bool initial_recv = false;
	uint32_t percentage;
	size_t chksz;

	if (!pdata->data_rcvd_len)
		initial_recv = true;

	chksz = pdata->data_len - pdata->data_rcvd_len;
	if (chksz > len)
		chksz = len;

	memcpy(pdata->data_ptr + pdata->data_rcvd_len, data, chksz);
	pdata->data_rcvd_len += chksz;

	if (pdata->data_len_unspec) {
		if (initial_recv ||
		    pdata->data_rcvd_len - pdata->last_percentage >= SZ_256K ||
		    pdata->data_rcvd_len == pdata->data_len) {
			printf("%zu bytes received.\r", pdata->data_rcvd_len);
			pdata->last_percentage = percentage;
		}
	} else {
		percentage = pdata->data_rcvd_len * 100 / pdata->data_len;

		if (initial_recv || percentage > pdata->last_percentage + 1 ||
		    pdata->data_rcvd_len == pdata->data_len) {
			printf("%zu/%zu (%u%%) received.\r",
			       pdata->data_rcvd_len, pdata->data_len,
			       percentage);
			pdata->last_percentage = percentage;
		}
	}

	if (pdata->data_rcvd_len == pdata->data_len) {
		net_boot_file_size = (u32)pdata->data_len;
		image_load_addr = (ulong)pdata->data_ptr;

		printf("\n");
		return 1;
	}

	return 0;
}

static int wget_parse_response(struct mtk_wget_pdata *pdata)
{
	char *p, *payload_ptr, *fields_ptr;
	size_t hdr_size;

	payload_ptr = strstr(pdata->resp_hdr, http_end_of_header);
	*payload_ptr = 0;

	/* start of payload */
	payload_ptr += sizeof(http_end_of_header) - 1;

	/* accurate size of response header */
	hdr_size = payload_ptr - pdata->resp_hdr;

	/* parse HTTP response header */
	fields_ptr = strstr(pdata->resp_hdr, http_end_of_line);
	if (!fields_ptr)
		goto bad_response;

	/* start of header fields */
	*fields_ptr = 0;
	fields_ptr += sizeof(http_end_of_line) - 1;

	/* Print response status */
	printf("%s\n", pdata->resp_hdr);

	/* parse proto & status code */
	p = strchr(pdata->resp_hdr, ' ');
	if (!p)
		goto bad_response;

	if (dectoul(p + 1, NULL) != 200) {
		pdata->result = -1;
		return -1;
	}

	/* Content-Length */
	p = strstr(fields_ptr, content_length_str);
	if (p) {
		p += sizeof(content_length_str) - 1;
		while (*p == ' ')
			p++;

		pdata->data_len = dectoul(p, NULL);
		printf("Content-Length: %ld\n", pdata->data_len);
	} else {
		if ((ulong)pdata->data_ptr >= gd->ram_base &&
		    (ulong)pdata->data_ptr < gd->ram_top)
			pdata->data_len = gd->ram_top - (ulong)pdata->data_ptr;
		else
			pdata->data_len = SIZE_MAX;
		pdata->data_len_unspec = true;
	}

	printf("Saving to 0x%08lx\n", (ulong)pdata->data_ptr);

	if (hdr_size == pdata->resp_hdr_len)
		return 0;

	return wget_recv_data(pdata, pdata->resp_hdr + hdr_size,
			      pdata->resp_hdr_len - hdr_size);

bad_response:
	printf("Bad response\n");
	return -1;
}

static void wget_rx(struct mtk_wget_pdata *pdata, struct mtk_tcp_cb_data *cbd)
{
	char *new_ptr;
	int ret;

	switch (pdata->state) {
	case WGET_REQ_SENT:
		if (!pdata->resp_hdr)
			new_ptr = malloc(cbd->datalen + 1);
		else
			new_ptr = realloc(pdata->resp_hdr,
					  pdata->resp_hdr_len + cbd->datalen + 1);

		if (!new_ptr) {
			mtk_tcp_close_conn(cbd->conn, 1);
			printf("No memory for HTTP response header\n");
			__wget_cleanup(pdata);
			pdata->result = -ENOMEM;
			return;
		}

		pdata->resp_hdr = new_ptr;
		memcpy(pdata->resp_hdr + pdata->resp_hdr_len, cbd->data,
		       cbd->datalen);
		pdata->resp_hdr_len += cbd->datalen;
		pdata->resp_hdr[pdata->resp_hdr_len] = 0;

		if (strstr(pdata->resp_hdr, http_end_of_header)) {
			ret = wget_parse_response(pdata);
			if (ret < 0)
				return;

			if (ret > 0) {
				mtk_tcp_close_conn(cbd->conn, 0);
				return;
			}

			pdata->state = WGET_DATA_RECEIVING;
		}

		break;

	case WGET_DATA_RECEIVING:
		ret = wget_recv_data(pdata, cbd->data, cbd->datalen);
		if (ret < 0)
			return;

		if (ret > 0) {
			mtk_tcp_close_conn(cbd->conn, 0);
			return;
		}
	}
}

static void wget_tcp_callback(struct mtk_tcp_cb_data *cbd)
{
	struct mtk_wget_pdata *pdata = cbd->pdata;

	switch (cbd->status) {
	case MTK_TCP_CB_NEW_CONN:
		printf("Connected\n");
		wget_initiate(pdata, cbd);
		break;

	case MTK_TCP_CB_DATA_RCVD:
		if (cbd->datalen)
			wget_rx(pdata, cbd);
		break;

	case MTK_TCP_CB_DATA_SENT:
		/* Nothing to do */
		break;

	case MTK_TCP_CB_REMOTE_CLOSED:
		if (cbd->timedout) {
			printf("Error: Connection timed out\n");
			pdata->result = -ETIMEDOUT;
		} else {
			if (pdata->data_len != pdata->data_rcvd_len) {
				printf("\n");
				printf("Error: Connection closed by remote\n");
				pdata->result = -ECONNRESET;
			} else if (!pdata->data_len) {
				printf("\n");
				printf("Error: Connection failed\n");
				pdata->result = -ECONNRESET;
			}
		}

		/* fall through */
	case MTK_TCP_CB_CLOSED:
		__wget_cleanup(pdata);
		break;

	default:;
	}
}

static struct in_addr inet_aton_ptr(const char *s, size_t *retlen)
{
	struct in_addr addr;
	const char *start = s;
	char *e;
	int i;

	addr.s_addr = 0;
	if (s == NULL)
		return addr;

	for (addr.s_addr = 0, i = 0; i < 4; ++i) {
		ulong val = s ? dectoul(s, &e) : 0;
		if (val > 255) {
			addr.s_addr = 0;
			*retlen = 0;
			return addr;
		}
		if (i != 3 && *e != '.') {
			addr.s_addr = 0;
			*retlen = 0;
			return addr;
		}
		addr.s_addr <<= 8;
		addr.s_addr |= (val & 0xFF);
		if (s) {
			if (i < 3)
				s = (*e) ? e+1 : e;
			else
				s = e;
		}
	}

	addr.s_addr = htonl(addr.s_addr);
	*retlen = s - start;
	return addr;
}

static char *encode_url(const char *path)
{
	size_t i, len, n = 0;
	char *str, *p;

	len = strlen(path);

	for (i = 0; i < len; i++) {
		if (path[i] == ' ')
			n++;
	}

	str = calloc(1, len + n * 2 + 1);
	if (!str)
		return NULL;

	p = str;

	for (i = 0; i < len; i++) {
		if (path[i] == ' ') {
			*p++ = '%';
			*p++ = '2';
			*p++ = '0';
		} else {
			*p++ = path[i];
		}
	}

	return str;
}

int start_wget(ulong load_addr, const char *url, size_t *retlen)
{
	struct mtk_wget_pdata pdata = { 0 };
	const char *host_str, *path;
	char tmp[46], *p;
	size_t len;
	ulong val;

	p = strstr(url, "://");
	if (p) {
		host_str = p + 3;
		if ((p - url != 4) || strncmp(url, "http", 4)) {
			printf("Error: only HTTP is supported\n");
			return -ENOTSUPP;
		}
	} else {
		host_str = url;
	}

	if (*host_str == '[') {
		printf("Error: IPv6 address is not supported\n");
		return -ENOTSUPP;
	}

	pdata.ipaddr = inet_aton_ptr(host_str, &len);
	path = host_str + len;

	if (!len || (*path != ':' && *path != '/')) {
#if defined(CONFIG_CMD_DNS)
		const char *s;
		char *p2;

		p = strchr(host_str, '/');
		p2 = strrchr(host_str, ':');

		if (p2 > host_str && p2 < p)
			path = p2;
		else
			path = p;

		len = path - host_str;

		pdata.host = calloc(1, len + 1);
		if (!pdata.host) {
			printf("Error: No memory\n");
			return -ENOMEM;
		}

		memcpy(pdata.host, host_str, len);

		net_dns_resolve = pdata.host;
		net_dns_env_var = "httpserverip";
		if (net_loop(DNS) < 0) {
			log_err("Error: dns lookup of %s failed, check setup\n", net_dns_resolve);
			free(pdata.host);
			return -EADDRNOTAVAIL;
		}

		s = env_get("httpserverip");
		if (!s) {
			log_err("Error: dns lookup of %s failed, check setup\n", net_dns_resolve);
			free(pdata.host);
			return -EADDRNOTAVAIL;
		}

		pdata.ipaddr = inet_aton_ptr(s, &len);
#else
		printf("Error: domain is not supported\n");
		return -ENOTSUPP;
#endif
	} else {
		len = path - host_str;

		pdata.host = calloc(1, len + 1);
		if (!pdata.host) {
			printf("Error: No memory\n");
			return -ENOMEM;
		}

		memcpy(pdata.host, host_str, len);
	}

	if (*path == ':') {
		if (path[1] == '/') {
			/* be compatible with native wget */
			pdata.port = htons(80);
			path++;
		} else {
			val = dectoul(path + 1, &p);
			if (p == path + 1 || *p != '/' || val > 65535) {
				printf("Error: invalid port number\n");
				return -EINVAL;
			}

			pdata.port = htons(val);
			path = p;
		}
	} else {
		pdata.port = htons(80);
	}

	pdata.url = encode_url(path);
	if (!pdata.url) {
		free(pdata.host);
		printf("Error: No memory\n");
		return -ENOMEM;
	}

	pdata.data_ptr = (void *)load_addr;

	net_server_ip = pdata.ipaddr;

	ip_to_string(net_server_ip, tmp);
	env_set("serverip", tmp);

	printf("Connecting to %s:%u... ", tmp, ntohs(pdata.port));

	mtk_tcp_connect(pdata.ipaddr.s_addr, pdata.port, wget_tcp_callback,
			&pdata);

	net_loop(MTK_TCP);

	free(pdata.host);
	free(pdata.url);

	if (pdata.result)
		return pdata.result;

	if (retlen)
		*retlen = pdata.data_len;

	return 0;
}
