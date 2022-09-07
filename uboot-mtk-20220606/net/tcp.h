// SPDX-License-Identifier:	GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 */

#ifndef __TCP_H__
#define __TCP_H__

#include <common.h>
#include <net.h>

#include <net/tcp.h>

#define TCP_MSS			1440

struct tcp_hdr {
	__be16 src;
	__be16 dst;
	__be32 seq;
	__be32 ack;
	__be16 flags;
	__be16 wnd;
	__be16 chksum;
	__be16 urg;
};

#define TCP_HDR_SIZE		(sizeof(struct tcp_hdr))

/* TCP flag bit */
#define TCP_FIN			BIT(0)
#define TCP_SYN			BIT(1)
#define TCP_RST			BIT(2)
#define TCP_PSH			BIT(3)
#define TCP_ACK			BIT(4)
#define TCP_URG			BIT(5)
#define TCP_FLAG_MASK		GENMASK(5, 0)

#define TCP_HDR_LEN_MASK	0x0f
#define TCP_HDR_LEN_SHIFT	12

/* TCP option kind */
#define TCP_OPT_EOL		0
#define TCP_OPT_NOP		1
#define TCP_OPT_MSS		2
#define TCP_OPT_WS		3

/* TCP RTT/RTO options */
#define TCP_RTT_G		200
#define TCP_RTT_K		4
#define TCP_RTT_ALPHA		3
#define TCP_RTT_BETA		2
#define TCP_RTT_INTERVAL	3000

/* TCP retransmission options */
#define TCP_REXMIT_MAX_SEG_DELAY	60000
#define TCP_REXMIT_MAX_CONN_DELAY	300000

/* TCP state */
enum tcp_state {
	INVALID_TCP_STATE = 0,
	LISTEN,
	SYN_SENT,
	SYN_RCVD,
	ESTABLISHED,
	FIN_WAIT_1,
	FIN_WAIT_2,
	CLOSE_WAIT,
	CLOSING,
	LAST_ACK,
	TIME_WAIT,
	CLOSED
};

/* Receive TCP packet */
void receive_tcp(struct ip_hdr *ip, int len, struct ethernet_hdr *et);

/* Called periodically to check the TCP status & send packets */
void tcp_periodic_check(void);

#endif /* __TCP_H__ */
