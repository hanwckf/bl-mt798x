// SPDX-License-Identifier:	GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 */

#ifndef __NET_TCP_H__
#define __NET_TCP_H__

enum tcp_cb_status {
	TCP_CB_NONE,
	TCP_CB_NEW_CONN,
	TCP_CB_DATA_RCVD,
	TCP_CB_DATA_SENT,
	TCP_CB_REMOTE_CLOSING,
	TCP_CB_REMOTE_CLOSED,
	TCP_CB_CLOSING,
	TCP_CB_CLOSED
};

struct tcb_cb_data {
	const void *conn;
	enum tcp_cb_status status;
	__be32 sip;
	__be16 sp;
	__be16 dp;
	void *pdata;
	void *data;
	uint32_t datalen;
};

typedef void (*tcp_conn_cb)(struct tcb_cb_data *cbd);

/* Initialize TCP subsystem */
void tcp_start(void);

/* Listen on specific port */
int tcp_listen(__be16 port, tcp_conn_cb cb);

/* Stop listening one porot */
int tcp_listen_stop(__be16 port);

/* Set connection's private data. return previous pdata */
void *tcp_conn_set_pdata(const void *conn, void *pdata);

/*
 * Set data buffer to be sent. Fail if previous data hasn't been fully sent.
 * Contents pointed by data shouldn't be changed before they're fully acked
 */
int tcp_send_data(const void *conn, const void *data, uint32_t size);

/* Close a connection */
int tcp_close_conn(const void *conn, int rst);

/* Close all connections and then exit net loop */
void tcp_close_all_conn(void);

/* Reset all connections and then exit net loop */
void tcp_reset_all_conn(void);

/* Return 1 if connection is in ESTABLISHED state */
int tcp_conn_is_alive(const void *conn);

#endif /* __NET_TCP_H__ */
