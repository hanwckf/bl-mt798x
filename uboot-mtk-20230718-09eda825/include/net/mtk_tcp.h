// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Simple TCP stack implementation
 */

#ifndef __NET_MTK_MTK_TCP_H__
#define __NET_MTK_MTK_TCP_H__

enum mtk_tcp_cb_status {
	MTK_TCP_CB_NONE,
	MTK_TCP_CB_NEW_CONN,
	MTK_TCP_CB_DATA_RCVD,
	MTK_TCP_CB_DATA_SENT,
	MTK_TCP_CB_REMOTE_CLOSING,
	MTK_TCP_CB_REMOTE_CLOSED,
	MTK_TCP_CB_CLOSING,
	MTK_TCP_CB_CLOSED
};

struct mtk_tcp_cb_data {
	const void *conn;
	enum mtk_tcp_cb_status status;
	__be32 sip;
	__be16 sp;
	__be16 dp;
	void *pdata;
	void *data;
	uint32_t datalen;
};

typedef void (*mtk_tcp_conn_cb)(struct mtk_tcp_cb_data *cbd);

/* Initialize TCP subsystem */
void mtk_tcp_start(void);

/* Listen on specific port */
int mtk_tcp_listen(__be16 port, mtk_tcp_conn_cb cb);

/* Stop listening one porot */
int mtk_tcp_listen_stop(__be16 port);

/* Set connection's private data. return previous pdata */
void *mtk_tcp_conn_set_pdata(const void *conn, void *pdata);

/*
 * Set data buffer to be sent. Fail if previous data hasn't been fully sent.
 * Contents pointed by data shouldn't be changed before they're fully acked
 */
int mtk_tcp_send_data(const void *conn, const void *data, uint32_t size);

/* Close a connection */
int mtk_tcp_close_conn(const void *conn, int rst);

/* Close all connections and then exit net loop */
void mtk_tcp_close_all_conn(void);

/* Reset all connections and then exit net loop */
void mtk_tcp_reset_all_conn(void);

/* Return 1 if connection is in ESTABLISHED state */
int mtk_tcp_conn_is_alive(const void *conn);

#endif /* __NET_MTK_MTK_TCP_H__ */
