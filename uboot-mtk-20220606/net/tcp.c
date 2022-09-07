// SPDX-License-Identifier:	GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc. All Rights Reserved.
 *
 * Simple TCP stack implementation
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 */

#include <common.h>
#include <errno.h>
#include <watchdog.h>
#include <malloc.h>
#include <net.h>
#include <command.h>
#include <div64.h>
#include <asm/gpio.h>

#include <linux/list.h>

#include "arp.h"
#include "tcp.h"

struct tcp_conn {
	struct list_head node;

	enum tcp_state status;

	struct in_addr ip_remote;
	__be16 port_remote;
	__be16 port_local;

	u32 peer_seq;
	u32 local_seq;
	u32 local_seq_last;
	u32 local_seq_acked;

	u32 ts;
	u32 ts_rexmit;
	u32 num_rexmit;

	u8 ethaddr[6];
	u16 mss;

	const u8 *tx;
	u32 txlen;

	tcp_conn_cb cb;
	int close_flag;
	int ack_flag;

	int rexmit_mode;
	int rexmit_wait_ack;

	int zw_mode;
	u32 peer_wnd;
	u32 peer_ws;

	u32 srtt;
	u32 rttvar;
	u32 rto;
	u32 ts_rtt;
	u32 ack_calc_rtt;

	void *pdata;
};

struct tcp_listen {
	struct list_head node;

	__be16 port;
	tcp_conn_cb cb;
};

static int tcp_send_packet_opt(struct tcp_conn *c, u16 flags, u32 seq, u32 ack,
	void *opt, int opt_size, const void *payload, int payload_len);
static int tcp_send_packet(struct tcp_conn *c, u16 flags, u32 seq, u32 ack,
	const void *payload, int payload_len);
static int tcp_send_packet_ctrl(struct tcp_conn *c, u16 flags);

static LIST_HEAD(listen_head);
static LIST_HEAD(conn_head);

static int tcp_stop;

void tcp_start(void)
{
	tcp_stop = 0;
}

static struct tcp_listen *tcp_listen_find(__be16 port)
{
	struct list_head *lh;
	struct tcp_listen *l;

	list_for_each(lh, &listen_head) {
		l = list_entry(lh, struct tcp_listen, node);
		if (l->port == port)
			return l;
	}

	return NULL;
}

int tcp_listen(__be16 port, tcp_conn_cb cb)
{
	struct tcp_listen *l;

	if (!port || !cb)
		return -EINVAL;

	if (tcp_listen_find(port))
		return -EADDRINUSE;

	l = malloc(sizeof(struct tcp_listen));
	if (!l)
		return -ENOMEM;

	l->port = port;
	l->cb = cb;

	list_add_tail(&l->node, &listen_head);

	return 0;
}

int tcp_listen_stop(__be16 port)
{
	struct tcp_listen *l;

	l = tcp_listen_find(port);
	if (l) {
		list_del(&l->node);
		return 0;
	}

	return -1;
}

static struct tcp_conn *tcp_conn_find(__be32 remoteip, __be16 remoteport,
	__be16 localport)
{
	struct list_head *lh;
	struct tcp_conn *c;

	list_for_each(lh, &conn_head) {
		c = list_entry(lh, struct tcp_conn, node);

		if (c->ip_remote.s_addr == remoteip &&
			c->port_remote == remoteport &&
			c->port_local == localport)
			return c;
	}

	return NULL;
}

static void tcp_conn_del(struct tcp_conn *c)
{
	list_del(&c->node);
	free(c);
}

static u16 tcp_checksum_compute(void *data, int len, __be32 sip,
	__be32 dip)
{
	int sum, oddbyte, phlen;
	u16 *ptr;

	struct {
		__be32 sip;
		__be32 dip;
		u8 zero;
		u8 prot;
		__be16 len;
	} tcp_pseudo_hdr = { sip, dip, 0, IPPROTO_TCP , htons(len & 0xffff) };

	sum = 0;

	phlen = sizeof(tcp_pseudo_hdr);
	ptr = (u16 *) &tcp_pseudo_hdr;

	while (phlen > 1) {
		sum += *ptr++;
		phlen -= 2;
	}

	ptr = (u16 *) data;

	while (len > 1) {
		sum += *ptr++;
		len -= 2;
	}

	if (len == 1) {
		oddbyte = 0;
		((u8 *) &oddbyte)[0] = *(u8 *) ptr;
		((u8 *) &oddbyte)[1] = 0;
		sum += oddbyte;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	sum = ~sum & 0xffff;

	return sum;
}

static int tcp_seq_sub(ulong a, ulong b)
{
	if (time_after(a, b)) {
		if (a >= b)
			return a - b;
		else
			return a + (~b) + 1;
	} else {
		if (a <= b)
			return - (int) (b - a);
		else
			return - (int) (b + (~a) + 1);
	}
}

static void rtt_calc(struct tcp_conn *c)
{
	u32 rtt = get_timer(c->ts_rtt);
	u32 sub;

	if (rtt > c->srtt)
		sub = rtt - c->srtt;
	else
		sub = c->srtt - rtt;

	c->rttvar = c->rttvar - (c->rttvar >> TCP_RTT_ALPHA) +
		(sub >> TCP_RTT_ALPHA);
	c->srtt = c->srtt - (c->srtt >> TCP_RTT_BETA) + (rtt >> TCP_RTT_BETA);
	c->rto = c->srtt + max((u32) TCP_RTT_G, TCP_RTT_K * c->rttvar);
}

static void tcp_set_mss_opt(u8 *opt, u16 mss)
{
	opt[0] = TCP_OPT_MSS;
	opt[1] = 4;
	opt[2] = (mss >> 8) & 0xff;
	opt[3] = mss & 0xff;
	opt[4] = TCP_OPT_WS;
	opt[5] = 3;
	opt[6] = 0;
	opt[7] = TCP_OPT_EOL;
}

static struct tcp_conn *tcp_conn_create(__be32 remoteip, struct tcp_hdr *tcp,
	u32 tcphdr_len, u8 *ethaddr, tcp_conn_cb cb)
{
	struct tcp_conn *c;
	u8 *o;
	u16 mss;
	u8 *optend;
	u8 opt[8];

	c = malloc(sizeof(struct tcp_conn));
	if (!c) {
		tcp_send_packet(c, TCP_RST | TCP_ACK, 1, c->peer_seq, NULL, 0);
		return NULL;
	}

	memset(c, 0, sizeof(struct tcp_conn));

	list_add_tail(&c->node, &conn_head);

	c->status = SYN_RCVD;
	memcpy(c->ethaddr, ethaddr, 6);
	c->ip_remote.s_addr = remoteip;
	c->port_remote = tcp->src;
	c->port_local = tcp->dst;
	c->peer_seq = ntohl(net_read_u32(&tcp->seq));
	c->local_seq = (u32) lldiv(64000ULL * get_timer(0), 500);
	c->peer_wnd = ntohs(tcp->wnd);
	c->peer_ws = 0;
	c->mss = 1460;
	c->cb = cb;

	/* parse tcp options */
	if (tcphdr_len > sizeof(struct tcp_hdr)) {
		o = (u8 *) tcp + sizeof(struct tcp_hdr);
		optend = o + tcphdr_len - sizeof(struct tcp_hdr);

		while (o < optend) {
			if (*o == TCP_OPT_EOL)
				break;

			if (*o == TCP_OPT_NOP) {
				o++;
				continue;
			}

			switch (*o) {
			case TCP_OPT_MSS:
				mss = ((u16) o[2] << 8) | o[3];
				if (mss < c->mss)
					c->mss = mss;
				break;
			case TCP_OPT_WS:
				c->peer_ws = min((u32) o[2], 14U);
				break;
			}

			o += o[1];
		}
	}

	/* send first SYN ACK packet */
	tcp_set_mss_opt(opt, c->mss);

	tcp_send_packet_opt(c, TCP_SYN | TCP_ACK,
		c->local_seq, c->peer_seq + 1, opt, sizeof(opt), NULL, 0);
	c->ts_rtt = get_timer(0);
	c->ts = get_timer(0);
	c->ts_rexmit = get_timer(0);
	c->num_rexmit = 0;

	return c;
}

void receive_tcp(struct ip_hdr *ip, int len, struct ethernet_hdr *et)
{
	struct tcp_hdr *tcp;
	struct tcp_listen *l;
	struct tcp_conn *c;
	u32 iphdr_len, tcphdr_len, data_size;
	u8 *data;
	u32 seq, ack, tmp;
	u16 flags, chksum;
	struct tcb_cb_data cbd = {};

	iphdr_len = (ip->ip_hl_v & 0x0f) * 4;

	tcp = (struct tcp_hdr *)((uintptr_t) ip + iphdr_len);

	chksum = tcp_checksum_compute(tcp, len - iphdr_len,
		net_read_ip(&ip->ip_src).s_addr,
		net_read_ip(&ip->ip_dst).s_addr);
	if (chksum) {
		debug("bad tcp checksum\n");
		return;
	}

	flags = ntohs(tcp->flags);
	tcphdr_len = ((flags >> TCP_HDR_LEN_SHIFT) & TCP_HDR_LEN_MASK) * 4;
	if (tcphdr_len < sizeof(struct tcp_hdr)) {
		debug("bad tcp header size\n");
		return;
	}

	data = (u8 *) ((uintptr_t) tcp + tcphdr_len);
	if (len < iphdr_len + tcphdr_len) {
		debug("incomplete tcp data\n");
		return;
	}

	data_size = len - iphdr_len - tcphdr_len;

	/* discard urgent data */
	if (flags & TCP_URG) {
		data += ntohs(tcp->urg);
		data_size -= ntohs(tcp->urg);
	}

	seq = ntohl(net_read_u32(&tcp->seq));
	ack = ntohl(net_read_u32(&tcp->ack));

	/* find existing connection */
	c = tcp_conn_find(net_read_ip(&ip->ip_src).s_addr, tcp->src, tcp->dst);

	if (!c) {
		/* whether to create new connection */
		if ((flags & TCP_FLAG_MASK) == TCP_SYN) {
			if ((l = tcp_listen_find(tcp->dst))) {
				/* create new connection */
				tcp_conn_create(
					net_read_ip(&ip->ip_src).s_addr,
					tcp, tcphdr_len, et->et_src, l->cb);
				return;
			}
		}

		return;
	}

	/* Prepare for callback data */
	cbd.conn = c;
	cbd.sip = c->ip_remote.s_addr;
	cbd.sp = tcp->src;
	cbd.dp = tcp->dst;
	cbd.pdata = c->pdata;

	if (flags & TCP_RST) {
		if (c->status != SYN_RCVD) {
			/* The peer has reset the connection */
			cbd.status = TCP_CB_REMOTE_CLOSED;
			assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
			c->cb(&cbd);
		}

		tcp_conn_del(c);
		return;
	}

	if (!(flags & TCP_ACK)) {
		switch (c->status) {
		case FIN_WAIT_1:
		case FIN_WAIT_2:
			break;
		default:
			/* Needs ACK bit */
			return;
		}
	}

	switch (c->status) {
	case SYN_RCVD:
		/* Calculate first RTO */
		c->srtt = get_timer(c->ts_rtt);
		c->rttvar = c->srtt / 2;
		c->rto = c->srtt + max((u32) TCP_RTT_G,
			TCP_RTT_K * c->rttvar);

		c->status = ESTABLISHED;
		c->peer_seq++;
		c->local_seq++;
		c->local_seq_last = c->local_seq;
		c->local_seq_acked = c->local_seq;

		cbd.status = TCP_CB_NEW_CONN;
		assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
		c->cb(&cbd);

		if (!data_size)
			break;

		/* If there is incoming data, fall through */
	case ESTABLISHED:
		/* Update window */
		tmp = ntohs(tcp->wnd) << c->peer_ws;
		if (tmp > c->mss && tmp > c->peer_wnd)
			c->zw_mode = 0;
		c->peer_wnd = tmp;

		if (tcp_seq_sub(seq, c->peer_seq) < 0) {
			if (tcp_seq_sub(seq + data_size, c->peer_seq) > 0) {
				/*
				 * This packet contains payload of which we
				 * have partially ACKed.
				 * Extract only the part we've not ACKed.
				 */
				tmp = seq + data_size - c->peer_seq;
				data += data_size - tmp;
				data_size = tmp;
			} else {
				/*
				 * Totally duplicated packet.
				 * Discard the payload and send an ACK to
				 * indicate our current ACK number
				 */
				c->ack_flag++;
				data_size = 0;
			}
		} else if (tcp_seq_sub(seq, c->peer_seq) > 0) {
			/*
			 * Incoming packet loss.
			 * Discard the payload and send an ACK to request
			 * retransmission.
			 */
			data_size = 0;
			c->ack_flag++;
		}

		if (tcp_seq_sub(ack, c->local_seq_acked) > 0) {
			/* clear retransmission state */
			c->rexmit_mode = 0;
			c->rexmit_wait_ack = 0;

			/* The peer has ACKed new data */
			c->local_seq_acked = ack;

			if (tcp_seq_sub(ack, c->ack_calc_rtt) >= 0 &&
				c->ack_calc_rtt) {
				/* Calculate new RTO */
				c->ack_calc_rtt = 0;
				rtt_calc(c);
			}

			if (tcp_seq_sub(c->local_seq_acked,
				c->local_seq_last) == c->txlen) {
				/* Current data chunk has been fully sent */
				c->tx = NULL;
				c->txlen = 0;
				cbd.status = TCP_CB_DATA_SENT;
				assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
				c->cb(&cbd);
			}
		} else {
			/*
			 * If the peer ACKed the data chunk we've already
			 * fully sent, just ignore it
			 */
			if (tcp_seq_sub(ack, c->local_seq_last) < 0)
				break;

			/* Currently no more unACKed data */
			if (c->local_seq == c->local_seq_acked)
				goto check_new_data;

			/*
			 * Record the transmission position the peer required,
			 * and enter the retransmission mode
			 */
			c->local_seq_acked = ack;
			c->rexmit_mode = 1;
		}

	check_new_data:
		if (data_size) {
			/*
			 * We have new data received
			 * Increase the next expected peer SEQ number
			 * Send an ACK to acknowledge the peer
			 */
			c->peer_seq += data_size;
			c->ack_flag++;

			cbd.status = TCP_CB_DATA_RCVD;
			cbd.data = data;
			cbd.datalen = data_size;
			assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
			c->cb(&cbd);
		}

		if (flags & TCP_FIN) {
			/* The peer is closing the connection */
			c->status = CLOSE_WAIT;
			cbd.status = TCP_CB_REMOTE_CLOSING;
			assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
			c->cb(&cbd);
		}

		break;
	case LAST_ACK:
		if (tcp_seq_sub(ack, c->local_seq + 1))
			return;

		c->status = CLOSE_WAIT;
		cbd.status = TCP_CB_REMOTE_CLOSED;
		assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
		c->cb(&cbd);
		tcp_conn_del(c);
		break;
	case FIN_WAIT_1:
		if (flags & TCP_ACK) {
			if (tcp_seq_sub(ack, c->local_seq + 1))
				return;
		}

		if ((flags & (TCP_ACK | TCP_FIN)) == (TCP_ACK | TCP_FIN)) {
			c->status = TIME_WAIT;
			c->local_seq++;
		} else if (flags & TCP_ACK) {
			c->status = FIN_WAIT_2;
			c->local_seq++;
		} else if (flags & TCP_FIN) {
			c->status = CLOSING;
		}
		break;
	case FIN_WAIT_2:
		if (flags & TCP_FIN)
			c->status = TIME_WAIT;
		break;
	case CLOSING:
		if (tcp_seq_sub(ack, c->local_seq + 1))
			return;

		c->status = TIME_WAIT;
		c->local_seq++;
		break;
	default:
		return;
	}
}

static void tcp_rexmit_init(struct tcp_conn *c)
{
	c->ts_rexmit = get_timer(0);
	c->ts = get_timer(0);
	c->num_rexmit = 0;
}

static void tcp_rexmit_reset(struct tcp_conn *c)
{
	c->ts = get_timer(0);
	c->num_rexmit++;
}

static int tcp_rexmit_check(struct tcp_conn *c, struct tcb_cb_data *cbd)
{
	ulong curts, timeout;

	if (get_timer(c->ts_rexmit) > TCP_REXMIT_MAX_CONN_DELAY) {
		/*
		* Reached maximum retransmission count,
		* resetting the connection
		*/
		tcp_send_packet(c, TCP_RST | TCP_ACK, c->local_seq,
			c->peer_seq, NULL, 0);

		/* Callback if the connection has established */
		switch (c->status) {
		case ESTABLISHED:
			cbd->status = TCP_CB_REMOTE_CLOSED;
			assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
			c->cb(cbd);
			break;
		case FIN_WAIT_1:
		case CLOSE_WAIT:
			cbd->status = TCP_CB_CLOSED;
			assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
			c->cb(cbd);
			break;
		default:;
		}

		tcp_conn_del(c);
		return -1;
	}

	/* Calculate RTO * 2^N */
	timeout = c->rto << c->num_rexmit;
	timeout = min(timeout, (ulong) TCP_REXMIT_MAX_SEG_DELAY);
	timeout += c->ts;

	curts = get_timer(0);

	if (time_after(curts, timeout))
		/* Timed out, retransmission needed */
		return 1;

	return 0;
}

static void tcp_conn_check(struct tcp_conn *c)
{
	const u8 *data = NULL;
	u32 datalen = 0;
	u8 flags = TCP_ACK;
	u32 sendseq;
	u32 datalen_sent, datalen_acked;
	u8 opt[8];
	struct tcb_cb_data cbd = {};

	/* Update timer */
	get_timer(0);

	cbd.conn = c;
	cbd.sip = c->ip_remote.s_addr;
	cbd.sp = c->port_remote;
	cbd.dp = c->port_local;
	cbd.pdata = c->pdata;

	switch (c->status) {
	case SYN_RCVD:
		switch (tcp_rexmit_check(c, &cbd)) {
		case -1:
			return;
		case 1:
			tcp_set_mss_opt(opt, c->mss);
			tcp_send_packet_opt(c, TCP_SYN | TCP_ACK,
				c->local_seq, c->peer_seq + 1,
				opt, sizeof(opt), NULL, 0);
			tcp_rexmit_reset(c);
		}

		break;
	case ESTABLISHED:
		sendseq = c->local_seq;

		datalen_sent = tcp_seq_sub(c->local_seq, c->local_seq_last);
		datalen_acked =
			tcp_seq_sub(c->local_seq_acked, c->local_seq_last);

		if (!c->rexmit_mode) {
			if (c->txlen > datalen_sent) {
				/* There is still data to be sent */
				data = c->tx + datalen_sent;
				datalen = min(c->txlen - datalen_sent,
					(u32) c->mss);

				if (datalen >= c->peer_wnd) {
					/* The peer may be zero-window */
					if (!c->zw_mode)
						tcp_rexmit_init(c);
					/* Enter zero-window mode */
					c->zw_mode = 1;
					/* And send no data */
					datalen = 0;
				} else {
					c->local_seq += datalen;
					c->peer_wnd -= datalen;
					c->ack_flag++;
					if (datalen == c->txlen - datalen_sent)
						flags |= TCP_PSH;
					tcp_rexmit_init(c);
				}
			} else if (c->txlen > datalen_acked) {
				/* There is data to be ACKed */
				switch (tcp_rexmit_check(c, &cbd)) {
				case -1:
					return;
				case 1:
					/*
					 * Timed out waiting for ACK, entering
					 * retransmission mode
					 */
					c->rexmit_mode = 1;
					c->rexmit_wait_ack = 0;
				}
			}
		}

		if (c->rexmit_mode) {
			if (c->rexmit_wait_ack) {
				switch (tcp_rexmit_check(c, &cbd)) {
				case -1:
					return;
				case 1:
					/*
					 * Timed out waiting for ACK of
					 * retransmitted segment, start for
					 * the next trial
					 */
					c->rexmit_wait_ack = 0;
				}
			}

			if (!c->rexmit_wait_ack) {
				sendseq = c->local_seq_acked;

				if (!c->zw_mode) {
					/*
					 * Send data if not in zero-window mode
					 */
					data = c->tx + datalen_acked;
					datalen = min(c->txlen - datalen_acked,
						(u32) c->mss);
					c->ack_flag++;
					c->rexmit_wait_ack = 1;
					tcp_rexmit_reset(c);
				}

				if (get_timer(c->ts_rtt) > TCP_RTT_INTERVAL) {
					/* Start calculating RTO */
					c->ack_calc_rtt = c->local_seq_acked +
						datalen;
					c->ts_rtt = get_timer(0);
				}
			}
		}

		if (!c->peer_wnd || c->zw_mode) {
			if (!c->zw_mode) {
				/* Enter zero-window mode */
				c->zw_mode = 1;
				tcp_rexmit_init(c);
			}

			if (c->txlen > datalen_sent) {
				/* Doing zero window probing */
				switch (tcp_rexmit_check(c, &cbd)) {
				case -1:
					return;
				case 1:
					/*
					 * If we have data unsend or unacked,
					 * send one byte to probe
					 */
					datalen = 1;
					sendseq = c->local_seq_acked;
					data = c->tx + datalen_acked;
					if (c->local_seq == c->local_seq_acked)
						c->local_seq += datalen;
					c->ack_flag++;
					tcp_rexmit_reset(c);
				}
			}
		}

		if (c->ack_flag) {
			c->ack_flag = 0;
			tcp_send_packet(c, flags, sendseq, c->peer_seq,
				data, datalen);
		}

		if (c->close_flag) {
			if (!c->tx || !c->txlen) {
				/*
				 * Close the connection after we've sent
				 * current data chunk
				 */
				tcp_send_packet(c, TCP_FIN | TCP_ACK,
					c->local_seq, c->peer_seq,
					NULL, 0);
				c->status = FIN_WAIT_1;
				tcp_rexmit_init(c);

				cbd.status = TCP_CB_CLOSING;
				assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
				c->cb(&cbd);
			}
		}

		break;
	case CLOSE_WAIT:
		tcp_send_packet_ctrl(c, TCP_FIN | TCP_ACK);
		c->status = LAST_ACK;
		tcp_rexmit_init(c);
		break;
	case LAST_ACK:
		switch (tcp_rexmit_check(c, &cbd)) {
		case -1:
			return;
		case 1:
			tcp_send_packet_ctrl(c, TCP_FIN | TCP_ACK);
			tcp_rexmit_reset(c);
		}

		break;
	case FIN_WAIT_1:
		switch (tcp_rexmit_check(c, &cbd)) {
		case -1:
			return;
		case 1:
			tcp_send_packet(c, TCP_FIN | TCP_ACK,
				c->local_seq, c->peer_seq, NULL, 0);
			tcp_rexmit_reset(c);
		}

		break;
	case TIME_WAIT:
		tcp_send_packet_ctrl(c, TCP_ACK);
		cbd.status = TCP_CB_CLOSED;
		assert((size_t) c->cb > CONFIG_SYS_SDRAM_BASE);
		c->cb(&cbd);
		tcp_conn_del(c);
		break;
	default:
		return;
	}
}

static ulong tcp_timeout_ms = 0;

void tcp_led_blink(void)
{
#ifdef CONFIG_WEBUI_FAILSAFE_ON_AUTOBOOT_FAIL
#if CONFIG_FAILSAFE_LED_GPIO_NUMBER >= 0
	static int value = 0;
	int ret;
	ret = gpio_request(CONFIG_FAILSAFE_LED_GPIO_NUMBER, "failsafe");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", CONFIG_FAILSAFE_LED_GPIO_NUMBER);
	}
	gpio_direction_input(CONFIG_FAILSAFE_LED_GPIO_NUMBER);
	gpio_direction_output(CONFIG_FAILSAFE_LED_GPIO_NUMBER, value);
	value = !value;
#endif
#endif
}

void tcp_periodic_check(void)
{
	struct list_head *lh, *n;
	struct tcp_conn *c;
	int num = 0;

	if(get_timer(0) - tcp_timeout_ms > CONFIG_SYS_HZ) {
		tcp_timeout_ms = get_timer(0);
		puts("T ");
		tcp_led_blink();
	}

	list_for_each_safe(lh, n, &conn_head) {
		c = list_entry(lh, struct tcp_conn, node);
		tcp_conn_check(c);
		num++;
	}

	if (tcp_stop && !num)
		net_state = NETLOOP_SUCCESS;
}

static int tcp_send_packet_opt(struct tcp_conn *c, u16 flags, u32 seq, u32 ack,
	void *opt, int opt_size, const void *payload, int payload_len)
{
	uchar *pkt;
	int eth_hdr_size;
	int pkt_hdr_size;
	struct tcp_hdr *tcp;

	seq = htonl(seq);
	ack = htonl(ack);

	pkt = (uchar *) net_tx_packet;

	eth_hdr_size = net_set_ether(pkt, c->ethaddr, PROT_IP);
	pkt += eth_hdr_size;

	net_set_ip_header(pkt, c->ip_remote, net_ip, IP_HDR_SIZE + TCP_HDR_SIZE + opt_size + payload_len, IPPROTO_TCP);
	pkt += IP_HDR_SIZE;

	tcp = (struct tcp_hdr *) pkt;
	tcp->src = c->port_local;
	tcp->dst = c->port_remote;
	tcp->flags = htons((((TCP_HDR_SIZE + opt_size) / 4) <<
		TCP_HDR_LEN_SHIFT) | (flags & TCP_FLAG_MASK));
	memcpy(&tcp->seq, &seq, 4);
	memcpy(&tcp->ack, &ack, 4);
	tcp->wnd = htons(c->mss);
	/* avoid compiler's optimization leading to an unaligned access */
	memset(&tcp->urg, 0, sizeof(tcp->urg));
	tcp->chksum = 0;

	pkt += TCP_HDR_SIZE;

	if (opt_size) {
		memcpy(pkt, opt, opt_size);
		pkt += opt_size;
	}

	memcpy(pkt, payload, payload_len);

	tcp->chksum = tcp_checksum_compute(tcp, TCP_HDR_SIZE + opt_size +
		payload_len, net_ip.s_addr, c->ip_remote.s_addr);

	pkt_hdr_size = eth_hdr_size + IP_HDR_SIZE + TCP_HDR_SIZE + opt_size;

	/* if MAC address was not discovered yet, do an ARP request */
	if (memcmp(c->ethaddr, net_null_ethaddr, 6) == 0) {
		/* save the ip and eth addr for the packet to send after arp */
		net_arp_wait_packet_ip = c->ip_remote;
		arp_wait_packet_ethaddr = c->ethaddr;

		/* size of the waiting packet */
		arp_wait_tx_packet_size = pkt_hdr_size + payload_len;

		/* and do the ARP request */
		arp_wait_try = 1;
		arp_wait_timer_start = get_timer(0);
		arp_request();
		return 0;	/* waiting */
	} else {
		return eth_send(net_tx_packet, pkt_hdr_size + payload_len);
	}
}

static int tcp_send_packet(struct tcp_conn *c, u16 flags, u32 seq, u32 ack,
	const void *payload, int payload_len)
{
	return tcp_send_packet_opt(c, flags, seq, ack, NULL, 0,
		payload, payload_len);
}

static int tcp_send_packet_ctrl(struct tcp_conn *c, u16 flags)
{
	return tcp_send_packet_opt(c, flags, c->local_seq, c->peer_seq + 1,
		NULL, 0, NULL, 0);
}

void *tcp_conn_set_pdata(const void *conn, void *pdata)
{
	struct tcp_conn *c = (struct tcp_conn *) conn;
	void *prev_pdata;

	if (!c)
		return NULL;

	prev_pdata = c->pdata;
	c->pdata = pdata;

	return prev_pdata;
}

int tcp_send_data(const void *conn, const void *data, u32 size)
{
	struct tcp_conn *c = (struct tcp_conn *) conn;
	u32 datalen_acked;

	if (!c || !data || !size)
		return -EINVAL;

	if (c->close_flag)
		return 1;

	datalen_acked = tcp_seq_sub(c->local_seq_acked, c->local_seq_last);

	if (c->tx && (c->txlen > datalen_acked))
		return 1;

	c->tx = data;
	c->txlen = size;
	c->local_seq_last = c->local_seq;
	c->local_seq_acked = c->local_seq;

	return 0;
}

int tcp_close_conn(const void *conn, int rst)
{
	struct tcp_conn *c = (struct tcp_conn *) conn;

	if (!c)
		return -EINVAL;

	if (rst) {
		tcp_send_packet(c, TCP_RST | TCP_ACK, c->local_seq,
				c->peer_seq, NULL, 0);
		tcp_conn_del(c);
	} else {
		c->close_flag = 1;
	}

	return 0;
}

void tcp_close_all_conn(void)
{
	struct list_head *lh, *n;
	struct tcp_conn *c;

	list_for_each_safe(lh, n, &conn_head) {
		c = list_entry(lh, struct tcp_conn, node);
		tcp_close_conn(c, 0);
	}

	tcp_stop = 1;
}

void tcp_reset_all_conn(void)
{
	struct list_head *lh, *n;
	struct tcp_conn *c;

	list_for_each_safe(lh, n, &conn_head) {
		c = list_entry(lh, struct tcp_conn, node);
		tcp_close_conn(c, 1);
	}

	tcp_stop = 1;
}

int tcp_conn_is_alive(const void *conn)
{
	struct tcp_conn *c = (struct tcp_conn *) conn;

	return c->status == ESTABLISHED;
}
