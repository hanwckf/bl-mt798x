/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#ifndef _NET_COMMON_H_
#define _NET_COMMON_H_

#include <cdefs.h>
#include <stdint.h>
#include <stdbool.h>

#define HWADDR_LEN			6

#define PROT_IP				0x0800
#define PROT_ARP			0x0806

#define ARP_ETHER			1
#define ARP_PLEN_IP			4
#define ARP_OP_REQUEST			1
#define ARP_OP_REPLY			2

#define IP_FLAGS_DFRAG			0x4000

#define IPPROTO_UDP			17

struct ethernet_hdr {
	uint8_t dest[HWADDR_LEN];
	uint8_t src[HWADDR_LEN];
	uint16_t prot;
} __attribute__((packed));

#define ETHER_HDR_SIZE		(sizeof(struct ethernet_hdr))

struct arp_req_ip {
	uint8_t sha[HWADDR_LEN];
	uint8_t spa[ARP_PLEN_IP];
	uint8_t tha[HWADDR_LEN];
	uint8_t tpa[ARP_PLEN_IP];
} __attribute__((packed));

#define ARP_REQ_IP_HDR_SIZE	(sizeof(struct arp_req_ip))

struct arp_hdr {
	uint16_t hw_type;
	uint16_t prot;
	uint8_t hw_len;
	uint8_t prot_len;
	uint16_t op;
	uint8_t data[];
} __attribute__((packed));

#define ARP_HDR_SIZE		(sizeof(struct arp_hdr))

struct in_addr {
	union {
		struct {
			uint8_t s_b1;
			uint8_t s_b2;
			uint8_t s_b3;
			uint8_t s_b4;
		} s_un_b;

		uint32_t s_addr;
	} s_un;
};

struct ip_hdr {
	uint8_t hl_v;
	uint8_t tos;
	uint16_t len;
	uint16_t id;
	uint16_t off;
	uint8_t ttl;
	uint8_t prot;
	uint16_t sum;
	struct in_addr src;
	struct in_addr dst;
} __attribute__((packed));

#define IP_HDR_SIZE		(sizeof(struct ip_hdr))

struct udp_hdr {
	uint16_t src;
	uint16_t dst;
	uint16_t len;
	uint16_t sum;
} __attribute__((packed));

#define UDP_HDR_SIZE		(sizeof(struct udp_hdr))

#define ETHER_MTU		1500

uint16_t compute_ip_checksum(const void *vptr, uint32_t nbytes);

void net_set_ether(void *xet, const uint8_t *destaddr, const uint8_t *srcaddr,
		   uint16_t prot);

bool net_is_ether(const void *xet, uint32_t len, uint16_t prot);

void net_set_ip_header(void *pkt, struct in_addr dest, struct in_addr source,
		       uint16_t pkt_len, uint8_t proto);

bool ip_checksum_ok(const void *addr, uint32_t nbytes);

bool net_is_ip_packet(const void *pkt, uint32_t len, uint8_t proto);

void net_set_udp_header(void *pkt, struct in_addr dest, struct in_addr src,
			uint16_t dport, uint16_t sport, uint16_t len);

bool net_is_udp_packet(const void *pkt, uint32_t len, uint16_t dport);

#endif /* _NET_COMMON_H_ */
