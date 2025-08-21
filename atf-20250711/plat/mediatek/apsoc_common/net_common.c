// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#include <endian.h>
#include <string.h>
#include "net_common.h"

static uint16_t net_ip_id;

uint16_t compute_ip_checksum(const void *vptr, uint32_t nbytes)
{
	const uint16_t *ptr = vptr;
	int sum, oddbyte;

	sum = 0;
	while (nbytes > 1) {
		sum += *ptr++;
		nbytes -= 2;
	}
	if (nbytes == 1) {
		oddbyte = 0;
		((uint8_t *)&oddbyte)[0] = *(uint8_t *)ptr;
		((uint8_t *)&oddbyte)[1] = 0;
		sum += oddbyte;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	sum = ~sum & 0xffff;

	return sum;
}

void net_set_ether(void *xet, const uint8_t *destaddr, const uint8_t *srcaddr,
		   uint16_t prot)
{
	struct ethernet_hdr *et = xet;

	memcpy(et->dest, destaddr, HWADDR_LEN);
	memcpy(et->src, srcaddr, HWADDR_LEN);
	et->prot = __htons(prot);
}

bool net_is_ether(const void *xet, uint32_t len, uint16_t prot)
{
	const struct ethernet_hdr *et = xet;

	if (len < ETHER_HDR_SIZE)
		return false;

	return et->prot == __htons(prot);
}

void net_set_ip_header(void *pkt, struct in_addr dest, struct in_addr source,
		       uint16_t pkt_len, uint8_t proto)
{
	struct ip_hdr *ip = pkt;

	ip->hl_v = 0x45;
	ip->tos = 0;
	ip->len = __htons(pkt_len);
	ip->prot = proto;
	ip->id = __htons(net_ip_id++);
	ip->off = __htons(IP_FLAGS_DFRAG);
	ip->ttl = 255;
	ip->sum = 0;

	memcpy(&ip->src, &source, sizeof(struct in_addr));
	memcpy(&ip->dst, &dest, sizeof(struct in_addr));

	ip->sum = compute_ip_checksum(ip, IP_HDR_SIZE);
}

bool ip_checksum_ok(const void *addr, uint32_t nbytes)
{
	return !(compute_ip_checksum(addr, nbytes) & 0xfffe);
}

bool net_is_ip_packet(const void *pkt, uint32_t len, uint8_t proto)
{
	const struct ip_hdr *ip = pkt;

	/* Before we start poking the header, make sure it is there */
	if (len < IP_HDR_SIZE)
		return false;

	/* Check the packet length */
	if (len < be16toh(ip->len))
		return false;

	len = be16toh(ip->len);
	if (len < IP_HDR_SIZE)
		return false;

	/* Can't deal with anything except IPv4 */
	if ((ip->hl_v & 0xf0) != 0x40)
		return false;

	/* Can't deal with IP options (headers != 20 bytes) */
	if ((ip->hl_v & 0x0f) != 0x05)
		return false;

	/* Check the Checksum of the header */
	if (!ip_checksum_ok(ip, IP_HDR_SIZE))
		return false;

	if (ip->prot != proto)
		return false;

	return true;
}

void net_set_udp_header(void *pkt, struct in_addr dest, struct in_addr src,
			uint16_t dport, uint16_t sport, uint16_t len)
{
	struct udp_hdr *udp = pkt + IP_HDR_SIZE;

	if (len & 1)
		((uint8_t *)pkt)[UDP_HDR_SIZE + len] = 0;

	net_set_ip_header(pkt, dest, src, IP_HDR_SIZE + UDP_HDR_SIZE + len,
			  IPPROTO_UDP);

	udp->src = __htons(sport);
	udp->dst = __htons(dport);
	udp->len = __htons(UDP_HDR_SIZE + len);
	udp->sum = 0;
}

bool net_is_udp_packet(const void *pkt, uint32_t len, uint16_t dport)
{
	const struct udp_hdr *udp = pkt;

	if (udp->dst != __htons(dport))
		return false;

	if (__ntohs(udp->len) < UDP_HDR_SIZE + len)
		return false;

	return true;
}
