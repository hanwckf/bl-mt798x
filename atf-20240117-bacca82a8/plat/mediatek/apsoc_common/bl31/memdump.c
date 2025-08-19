/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <endian.h>
#include <context.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <common/tf_crc32.h>
#include <drivers/console.h>
#include <drivers/delay_timer.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/mmio.h>
#include <mtk_eth.h>
#include <mtk_wdt.h>
#include <plat_mdump_def.h>
#include <net_common.h>
#include "bl31_common_setup.h"
#include "memdump.h"

#define PAYLOAD_OFFSET		(ETHER_HDR_SIZE + IP_HDR_SIZE + UDP_HDR_SIZE)
#define PAYLOAD_MAX_LEN		(ETHER_MTU - PAYLOAD_OFFSET)

static uint8_t macaddr[6] = { 0x00, 0x0c, 0x43, 0x00, 0x00, 0x01 };
static uint8_t dstaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
static struct in_addr sipaddr = { .s_un.s_un_b = { 192, 168, 1, 1 } };
static struct in_addr dipaddr = { .s_un.s_un_b = { 255, 255, 255, 255 } };
static uint8_t packet[ETHER_MTU], payload[PAYLOAD_MAX_LEN];

static uint32_t assoc_id;

/* Private exported functions from psci_system_off.c */
void __dead2 psci_system_reset(void);

static void net_send_packet(uint32_t len)
{
	int ret;

	net_set_ether(packet, dstaddr, macaddr, PROT_IP);

	memcpy(packet + PAYLOAD_OFFSET, payload, len);

	net_set_udp_header(packet + ETHER_HDR_SIZE, dipaddr, sipaddr,
			   MDUMP_DST_PORT, MDUMP_SRC_PORT, len);

	do {
		ret = mtk_eth_send(packet, PAYLOAD_OFFSET + len);
	} while (ret == -EPERM);
}

static void arp_receive(void *pkt, uint32_t len)
{
	struct arp_hdr *arp = pkt;
	struct arp_req_ip *arpreq = (void *)arp->data, *arpresp;
	int ret;

	if (len < ARP_HDR_SIZE + ARP_REQ_IP_HDR_SIZE)
		return;

	if (__ntohs(arp->hw_type) != ARP_ETHER)
		return;

	if (__ntohs(arp->prot) != PROT_IP)
		return;

	if (arp->hw_len != HWADDR_LEN)
		return;

	if (arp->prot_len != ARP_PLEN_IP)
		return;

	if (__ntohs(arp->op) != ARP_OP_REQUEST)
		return;

	if (memcmp(arpreq->tpa, &sipaddr, ARP_PLEN_IP))
		return;

	net_set_ether(packet, dstaddr, macaddr, PROT_ARP);

	memcpy(packet + ETHER_HDR_SIZE, arp, ARP_HDR_SIZE + ARP_REQ_IP_HDR_SIZE);

	arp = (void *)packet + ETHER_HDR_SIZE;
	arpresp = (void *)arp->data;

	arp->op = __htons(ARP_OP_REPLY);
	memcpy(arpresp->tha, arpreq->sha, HWADDR_LEN);
	memcpy(arpresp->tpa, arpreq->spa, ARP_PLEN_IP);
	memcpy(arpresp->sha, macaddr, HWADDR_LEN);
	memcpy(arpresp->spa, &sipaddr, ARP_PLEN_IP);

	do {
		ret = mtk_eth_send(packet, ETHER_HDR_SIZE + ARP_HDR_SIZE + ARP_REQ_IP_HDR_SIZE);
	} while (ret == -EPERM);
}

static void process_other_packets(void)
{
	uint32_t cnt = 32;
	void *rcvdpkt;
	int len;

	while (cnt) {
		len = mtk_eth_recv(&rcvdpkt);
		if (len == -EAGAIN)
			return;

		if (net_is_ether(rcvdpkt, len, PROT_ARP)) {
			arp_receive(rcvdpkt + ETHER_HDR_SIZE,
				    len - ETHER_HDR_SIZE);
		}

		mtk_eth_free_pkt(rcvdpkt);

		cnt--;
	}
}

static bool check_mdump_response_header(struct mdump_response_header *resp)
{
	uint32_t old_crc = le32toh(resp->bh.checksum);

	if (le32toh(resp->bh.version) != MDUMP_PACKET_VERSION)
		return false;

	if (le32toh(resp->bh.assoc_id) != assoc_id)
		return false;

	if (le32toh(resp->type) >= __MDUMP_RESP_MAX)
		return false;

	resp->bh.checksum = 0;
	resp->bh.checksum = tf_crc32(0, (uint8_t *)resp, sizeof(*resp));

	return old_crc == resp->bh.checksum;
}

static bool __receive_mdump_response_header(struct mdump_response_header *rh,
					    uint32_t timeout_ms, uint8_t *mac,
					    struct in_addr *ip)
{
	uint64_t tmo = timeout_init_us(timeout_ms * 1000);
	struct in_addr addr;
	bool again = true;
	void *rcvdpkt;
	int len;

do_receive:
	do {
		len = mtk_eth_recv(&rcvdpkt);

		if (timeout_elapsed(tmo))
			return false;
	} while (len == -EAGAIN);

	if (net_is_ether(rcvdpkt, len, PROT_IP)) {
		if (!net_is_ip_packet(rcvdpkt + ETHER_HDR_SIZE,
				      len - ETHER_HDR_SIZE, IPPROTO_UDP))
			goto cleanup;

		if (!net_is_udp_packet(rcvdpkt + ETHER_HDR_SIZE + IP_HDR_SIZE,
				       sizeof(*rh), MDUMP_SRC_PORT))
			goto cleanup;

		memcpy(rh, rcvdpkt + PAYLOAD_OFFSET, sizeof(*rh));

		if (check_mdump_response_header(rh))
			again = false;
	} else if (net_is_ether(rcvdpkt, len, PROT_ARP)) {
		arp_receive(rcvdpkt + ETHER_HDR_SIZE, len - ETHER_HDR_SIZE);
	}

cleanup:
	memcpy(&addr, &((struct ip_hdr *)(rcvdpkt + ETHER_HDR_SIZE))->src,
	       ARP_PLEN_IP);

	if (mac || ip) {
		if (mac) {
			memcpy(mac, ((struct ethernet_hdr *)rcvdpkt)->src,
			       HWADDR_LEN);
		}

		if (ip) {
			memcpy(ip, &addr, ARP_PLEN_IP);
		}
	}

	mtk_eth_free_pkt(rcvdpkt);

	if (again) {
		if (timeout_elapsed(tmo))
			return false;

		goto do_receive;
	}

	if (dipaddr.s_un.s_addr != 0xffffffff) {
		if (addr.s_un.s_addr != dipaddr.s_un.s_addr)
			return false;
	}

	return true;
}

static bool receive_mdump_response_header(struct mdump_response_header *rh,
					  uint32_t timeout_ms)
{
	return __receive_mdump_response_header(rh, timeout_ms, NULL, NULL);
}

static void send_mdump_control_header(void)
{
	uint32_t i, len, nranges = ARRAY_SIZE(mdump_ranges);
	struct mdump_control_header *ch = (void *)payload;

	len = sizeof(struct mdump_control_header) +
	      nranges * sizeof(struct mdump_control_range);

	ch->bh.magic = htole32(MDUMP_MAGIC_CONTROL);
	ch->bh.checksum = 0;
	ch->bh.version = htole32(MDUMP_PACKET_VERSION);
	ch->bh.assoc_id = htole32(assoc_id);
	ch->platform = htole32(SOC_CHIP_ID);
	ch->num_ranges = htole32(nranges);

	for (i = 0; i < nranges; i++) {
		ch->ranges[i].addr = mdump_ranges[i].r.addr;
		ch->ranges[i].end = mdump_ranges[i].r.end;

		if (ch->ranges[i].end == DRAM_END)
			ch->ranges[i].end = DRAM_START + mtk_bl31_get_dram_size();

		ch->ranges[i].addr = htole32(ch->ranges[i].addr);
		ch->ranges[i].end = htole32(ch->ranges[i].end);
	}

	ch->bh.checksum = tf_crc32(0, payload, len);
	ch->bh.checksum = htole32(ch->bh.checksum);

	net_send_packet(len);
}

static void send_mdump_mem_packet(uint32_t index, uintptr_t paddr,
				  const void *ptr, size_t len)
{
	struct mdump_range_header *rh = (void *)payload;
	size_t pktlen;

	pktlen = sizeof(struct mdump_range_header) + len;

	rh->bh.magic = htole32(MDUMP_MAGIC_RANGE);
	rh->bh.checksum = 0;
	rh->bh.version = htole32(MDUMP_PACKET_VERSION);
	rh->bh.assoc_id = htole32(assoc_id);
	rh->index = htole32(index);
	rh->size = htole32(len);
	rh->offset = htole64(paddr);

	memcpy(rh->data, ptr, len);

	rh->bh.checksum = tf_crc32(0, payload, pktlen);
	rh->bh.checksum = htole32(rh->bh.checksum);

	net_send_packet(pktlen);
}

static void send_mdump_mem_range(uint32_t index, uintptr_t paddr,
				 uintptr_t vaddr, size_t len,
				 bool show_progress)
{
	uint32_t chksz, percentage = 0, last_percentage = 0;
	const void *ptr = (const void *)vaddr;
	size_t len_sent = 0;

	while (len_sent < len) {
		chksz = sizeof(payload) - sizeof(struct mdump_range_header);
		chksz &= ~(sizeof(uint32_t) - 1);

		if (chksz > len - len_sent)
			chksz = len - len_sent;

		send_mdump_mem_packet(index, paddr + len_sent, ptr, chksz);

		len_sent += chksz;
		ptr += chksz;

		percentage = (uint64_t)len_sent * 100ULL / len;
		if (show_progress && percentage > last_percentage) {
			last_percentage = percentage;
#if LOG_LEVEL >= LOG_LEVEL_NOTICE
			printf("\r");
#endif
			NOTICE("MDUMP: %u%% completed.", percentage);
		}

		process_other_packets();
	}

#if LOG_LEVEL >= LOG_LEVEL_NOTICE
	if (show_progress)
		printf("\n");
#endif
}

static bool is_excluded_device_addr(const struct mdump_range_exclude *excludes,
				    uintptr_t addr, uint32_t len)
{
	uint32_t i;

	if (!excludes)
		return false;

	for (i = 0; i < excludes->count; i++) {
		if (addr >= excludes->r[i].addr && addr + len <= excludes->r[i].end)
			return true;
	}

	return false;
}

static void send_mdump_device_packet(uint32_t index, uintptr_t paddr,
				     uintptr_t base, size_t len,
				     const struct mdump_range_exclude *excludes)
{
	struct mdump_range_header *rh = (void *)payload;
	uint8_t *data = rh->data;
	size_t pktlen;
	uint32_t val;

	pktlen = sizeof(struct mdump_range_header) + len;

	rh->bh.magic = htole32(MDUMP_MAGIC_RANGE);
	rh->bh.checksum = 0;
	rh->bh.version = htole32(MDUMP_PACKET_VERSION);
	rh->bh.assoc_id = htole32(assoc_id);
	rh->index = htole32(index);
	rh->size = htole32(len);
	rh->offset = htole64(paddr);

	while (len) {
		val = 0;

		if (!is_excluded_device_addr(excludes, base, sizeof(uint32_t)))
			val = mmio_read_32(base);

		memcpy(data, &val, sizeof(uint32_t));

		data += sizeof(uint32_t);
		base += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	rh->bh.checksum = tf_crc32(0, payload, pktlen);
	rh->bh.checksum = htole32(rh->bh.checksum);

	net_send_packet(pktlen);
}

static void send_mdump_device_range(uint32_t index, uintptr_t paddr,
				    uintptr_t vaddr, size_t len,
				    const struct mdump_range_exclude *excludes,
				    bool show_progress)
{
	uint32_t chksz, percentage = 0, last_percentage = 0;
	size_t len_sent = 0;

	while (len_sent < len) {
		chksz = sizeof(payload) - sizeof(struct mdump_range_header);
		if (chksz > len - len_sent)
			chksz = len - len_sent;

		chksz &= ~(sizeof(uint32_t) - 1);

		send_mdump_device_packet(index, paddr, vaddr, chksz, excludes);

		len_sent += chksz;
		vaddr += chksz;
		paddr += chksz;

		percentage = (uint64_t)len_sent * 100ULL / len;
		if (show_progress && percentage > last_percentage) {
			last_percentage = percentage;
#if LOG_LEVEL >= LOG_LEVEL_NOTICE
			printf("\r");
#endif
			NOTICE("MDUMP: %u%% completed.", percentage);
		}

		process_other_packets();
	}

#if LOG_LEVEL >= LOG_LEVEL_NOTICE
	if (show_progress)
		printf("\n");
#endif
}

static void send_mdump_range_end(uint32_t index)
{
	struct mdump_range_end_header *reh = (void *)payload;

	reh->bh.magic = htole32(MDUMP_MAGIC_RANGE_END);
	reh->bh.checksum = 0;
	reh->bh.version = htole32(MDUMP_PACKET_VERSION);
	reh->bh.assoc_id = htole32(assoc_id);
	reh->index = htole32(index);

	reh->bh.checksum = tf_crc32(0, payload, sizeof(struct mdump_range_end_header));
	reh->bh.checksum = htole32(reh->bh.checksum);

	net_send_packet(sizeof(struct mdump_range_end_header));
}

static void send_mdump_ranges(void)
{
	uint32_t i, nranges = ARRAY_SIZE(mdump_ranges), type;
	uintptr_t mapped_addr, paddr, end, r_addr, r_end;
	struct mdump_response_header resp;
	size_t len;
	bool rcvd;
	int ret;

	for (i = 0; i < nranges; i++) {
		paddr = mdump_ranges[i].r.addr;
		end = mdump_ranges[i].r.end;
		if (end == DRAM_END)
			end = DRAM_START + mtk_bl31_get_dram_size();

		if (mdump_ranges[i].is_device) {
			paddr &= ~(sizeof(uint32_t) - 1);
			end = (end + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
		}

		mapped_addr = paddr;
		len = end - paddr;

		if (mdump_ranges[i].need_map) {
			ret = mmap_add_dynamic_region(paddr, paddr, len,
						      MT_MEMORY | MT_RO);
			if (ret) {
				ERROR("MDUMP: Failed to map range 0x%zx - 0x%zx, error %d\n",
				      paddr, end, ret);
				continue;
			}
		}

		NOTICE("MDUMP: Sending %s range %u: 0x%zx - 0x%zx\n",
		       mdump_ranges[i].is_device ? "device" : "memory",
		       i, paddr, end);

		if (!mdump_ranges[i].is_device)
			send_mdump_mem_range(i, paddr, mapped_addr, len, true);
		else
			send_mdump_device_range(i, paddr, mapped_addr, len,
						mdump_ranges[i].excludes, true);

		/* Check response for missing packets */
		while (true) {
			send_mdump_range_end(i);

			rcvd = receive_mdump_response_header(&resp, 500);
			if (!rcvd)
				continue;

			type = le32toh(resp.type);

			if (type == MDUMP_RESP_RANGE_ACK)
				break;

			if (type == MDUMP_RESP_RANGE_REXMIT && le32toh(resp.range.index) == i) {
				r_addr = (uintptr_t)le64toh(resp.range.r.addr);
				r_end = (uintptr_t)le64toh(resp.range.r.end);

				if (r_addr < paddr || r_end > end || r_addr > r_end)
					continue;

				VERBOSE("MDUMP: Re-sending %s range %u: 0x%zx - 0x%zx\n",
					mdump_ranges[i].is_device ? "device" : "memory", i, r_addr, r_end);

				if (!mdump_ranges[i].is_device) {
					send_mdump_mem_range(i, r_addr, mapped_addr + r_addr - paddr, r_end - r_addr,
							     false);
				} else {
					send_mdump_device_range(i, r_addr, mapped_addr + r_addr - paddr, r_end - r_addr,
								mdump_ranges[i].excludes, false);
				}
			}
		}

		NOTICE("MDUMP: %s range %u sent\n",
		       mdump_ranges[i].is_device ? "Device" : "Memory",
		       i);

		if (mdump_ranges[i].need_map) {
			ret = mmap_remove_dynamic_region(mapped_addr, len);
			if (ret) {
				ERROR("MDUMP: Failed to unmap range 0x%zx - 0x%zx, error %d\n",
				      paddr, end, ret);
				return;
			}
		}
	}
}

DEFINE_SYSREG_RW_FUNCS(tpidr_el1)
DEFINE_SYSREG_RW_FUNCS(contextidr_el1)
DEFINE_SYSREG_RW_FUNCS(tpidr_el0)
DEFINE_SYSREG_RW_FUNCS(tpidrro_el0)

static void send_mdump_context_real(cpu_context_t *ctx)
{
	struct mdump_context_header *ch = (void *)payload;
	uint32_t i;

	ch->bh.magic = htole32(MDUMP_MAGIC_CONTEXT);
	ch->bh.checksum = 0;
	ch->bh.version = htole32(MDUMP_PACKET_VERSION);
	ch->bh.assoc_id = htole32(assoc_id);

	for (i = 0; i <= 30; i++)
		ch->gpr[i] = htole64(ctx->gpregs_ctx.ctx_regs[i]);

	ch->pstate.sp_el0 = htole64(read_ctx_reg(&ctx->gpregs_ctx, CTX_GPREG_SP_EL0));
	ch->pstate.pc = htole64(read_ctx_reg(&ctx->el3state_ctx, CTX_ELR_EL3));
	ch->pstate.cpsr = htole64(read_ctx_reg(&ctx->el3state_ctx, CTX_SPSR_EL3));

	ch->pstate.elr_el1 = htole64(read_elr_el1());
	ch->pstate.elr_el2 = htole64(read_elr_el2());
	ch->pstate.spsr_el1 = htole64(read_spsr_el1());
	ch->pstate.spsr_el2 = htole64(read_spsr_el2());
	ch->pstate.sp_el1 = htole64(read_sp_el1());
	ch->pstate.sp_el2 = htole64(read_sp_el2());

	ch->sysreg.sctlr_el3 = htole64(read_sctlr_el3());
	ch->sysreg.tcr_el3 = htole64(read_tcr_el3());
	ch->sysreg.ttbr0_el3 = htole64(read_ttbr0_el3());
	ch->sysreg.mair_el3 = htole64(read_mair_el3());
	ch->sysreg.scr_el3 = htole64(read_scr_el3());
	ch->sysreg.esr_el3 = htole64(read_esr_el3());
	ch->sysreg.far_el3 = htole64(read_far_el3());
	ch->sysreg.vbar_el3 = htole64(read_vbar_el3());

	ch->sysreg.sctlr_el2 = htole64(read_sctlr_el2());
	ch->sysreg.tcr_el2 = htole64(read_tcr_el2());
	ch->sysreg.ttbr0_el2 = htole64(read_ttbr0_el2());
	ch->sysreg.mair_el2 = htole64(read_mair_el2());
	ch->sysreg.esr_el2 = htole64(read_esr_el2());
	ch->sysreg.tpidr_el2 = htole64(read_tpidr_el2());
	ch->sysreg.hcr_el2 = htole64(read_hcr_el2());
	ch->sysreg.vttbr_el2 = htole64(read_vttbr_el2());
	ch->sysreg.vtcr_el2 = htole64(read_vtcr_el2());
	ch->sysreg.far_el2 = htole64(read_far_el2());
	ch->sysreg.vbar_el2 = htole64(read_vbar_el2());
	ch->sysreg.dbgvcr32_el2 = htole64(read_dbgvcr32_el2());
	ch->sysreg.hpfar_el2 = htole64(read_hpfar_el2());
	ch->sysreg.hstr_el2 = htole64(read_hstr_el2());
	ch->sysreg.mdcr_el2 = htole64(read_mdcr_el2());
	ch->sysreg.vmpidr_el2 = htole64(read_vmpidr_el2());
	ch->sysreg.vpidr_el2 = htole64(read_vpidr_el2());

	ch->sysreg.sctlr_el1 = htole64(read_sctlr_el1());
	ch->sysreg.tcr_el1 = htole64(read_tcr_el1());
	ch->sysreg.ttbr0_el1 = htole64(read_ttbr0_el1());
	ch->sysreg.ttbr1_el1 = htole64(read_ttbr1_el1());
	ch->sysreg.mair_el1 = htole64(read_mair_el1());
	ch->sysreg.esr_el1 = htole64(read_esr_el1());
	ch->sysreg.tpidr_el1 = htole64(read_tpidr_el1());
	ch->sysreg.par_el1 = htole64(read_par_el1());
	ch->sysreg.far_el1 = htole64(read_far_el1());
	ch->sysreg.contextidr_el1 = htole64(read_contextidr_el1());
	ch->sysreg.vbar_el1 = htole64(read_vbar_el1());

	ch->sysreg.tpidr_el0 = htole64(read_tpidr_el0());
	ch->sysreg.tpidrro_el0 = htole64(read_tpidrro_el0());

	ch->bh.checksum = tf_crc32(0, payload, sizeof(struct mdump_context_header));
	ch->bh.checksum = htole32(ch->bh.checksum);

	net_send_packet(sizeof(struct mdump_context_header));
}

static void send_mdump_context(void *handle)
{
	struct mdump_response_header resp;
	bool rcvd;

	NOTICE("MDUMP: Starting CPU context transmission\n");

	while (true) {
		send_mdump_context_real(handle);

		rcvd = receive_mdump_response_header(&resp, 1000);
		if (!rcvd)
			continue;

		if (le32toh(resp.type) != MDUMP_RESP_CONTEXT_ACK)
			continue;

		break;
	}

	NOTICE("MDUMP: CPU context sent\n");
}

static void send_mdump_end(void)
{
	struct mdump_end_header *eh = (void *)payload;

	eh->bh.magic = htole32(MDUMP_MAGIC_END);
	eh->bh.checksum = 0;
	eh->bh.version = htole32(MDUMP_PACKET_VERSION);
	eh->bh.assoc_id = htole32(assoc_id);

	eh->bh.checksum = tf_crc32(0, payload, sizeof(struct mdump_end_header));
	eh->bh.checksum = htole32(eh->bh.checksum);

	net_send_packet(sizeof(struct mdump_end_header));
}

static void mdump_control_check(void)
{
	struct mdump_response_header resp;
	uint8_t rcv_hwaddr[HWADDR_LEN];
	struct in_addr rcv_ip;
	bool rcvd;

	NOTICE("MDUMP: Starting control negotiation\n");

	while (true) {
		send_mdump_control_header();

		rcvd = __receive_mdump_response_header(&resp, 1000, rcv_hwaddr,
						       &rcv_ip);
		if (!rcvd)
			continue;

		if (le32toh(resp.type) != MDUMP_RESP_CONTROL_ACK)
			continue;

		break;
	}

	memcpy(dstaddr, rcv_hwaddr, HWADDR_LEN);
	memcpy(&dipaddr, &rcv_ip, sizeof(rcv_ip));

	NOTICE("MDUMP: Receiver MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       dstaddr[0], dstaddr[1], dstaddr[2], dstaddr[3], dstaddr[4],
	       dstaddr[5]);
	NOTICE("MDUMP: Receiver IP: %u.%u.%u.%u\n", dipaddr.s_un.s_un_b.s_b1,
	       dipaddr.s_un.s_un_b.s_b2, dipaddr.s_un.s_un_b.s_b3,
	       dipaddr.s_un.s_un_b.s_b4);

	NOTICE("MDUMP: Control negotiation succeeded\n");
}

static void mdump_end_check(void)
{
	struct mdump_response_header resp;
	uint32_t cnt = 10;
	bool rcvd;

	NOTICE("MDUMP: Sending end of session header\n");

	while (cnt--) {
		send_mdump_end();

		rcvd = receive_mdump_response_header(&resp, 1000);
		if (!rcvd)
			continue;

		if (le32toh(resp.type) != MDUMP_RESP_END_ACK)
			continue;

		NOTICE("MDUMP: End header sent\n");
		return;
	}

	NOTICE("MDUMP: No response from host\n");
}

static void mdump_setup_session(void)
{
	uint32_t rnd[2] = { 0 }, i, retry = 10;
	uint8_t *p = (uint8_t *)&rnd[1];
	uint64_t cnt;
	int ret;

	for (i = 0; i < ARRAY_SIZE(rnd); i++) {
		while (retry) {
			ret = plat_get_rnd(&rnd[i]);
			if (!ret)
				break;

			mdelay(10);
			retry--;
		}
	}

	if (!rnd[0] || !rnd[1]) {
		cnt = read_cntpct_el0();
		while (cnt >> 32)
			cnt = (cnt >> 32) + (uint32_t)cnt;
	}

	if (!rnd[0] && !rnd[1]) {
		rnd[0] = (uint32_t)cnt;
		rnd[1] = ~rnd[0];
	} else if (!rnd[0]) {
		rnd[0] = (uint32_t)cnt;
	} else if (!rnd[1]) {
		rnd[1] = (uint32_t)cnt;
	}

	assoc_id = rnd[0];

	memcpy(macaddr + 3, p, 3);
	sipaddr.s_un.s_un_b.s_b4 = p[3];

	switch (sipaddr.s_un.s_un_b.s_b4) {
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 255:
		sipaddr.s_un.s_un_b.s_b4 = 1;
		break;
	}

	NOTICE("MDUMP: Assoc ID: %08x\n", assoc_id);
	NOTICE("MDUMP: Sender MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4],
	       macaddr[5]);
	NOTICE("MDUMP: Sender IP: %u.%u.%u.%u\n", sipaddr.s_un.s_un_b.s_b1,
	       sipaddr.s_un.s_un_b.s_b2, sipaddr.s_un.s_un_b.s_b3,
	       sipaddr.s_un.s_un_b.s_b4);
}

static bool validate_mdump_ranges(void)
{
	uint64_t end;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(mdump_ranges); i++) {
		end = mdump_ranges[i].r.end;

		if (end == DRAM_END)
			end = DRAM_START + mtk_bl31_get_dram_size();

		if (end < mdump_ranges[i].r.addr) {
			ERROR("MDUMP: Region %u has its end address below its start address\n", i);
			return false;
		}
	}

	return true;
}

void do_mem_dump(int panic_timeout, void *handle)
{
	int ret;

	console_switch_state(CONSOLE_FLAG_BOOT);

	NOTICE("MDUMP: Disable watchdog\n");
	mtk_wdt_control(false);

	if (!mtk_bl31_get_dram_size()) {
		ERROR("MDUMP: DRAM size not passwd by BL2\n");
		goto out;
	}

	if (!validate_mdump_ranges())
		goto out;

	NOTICE("MDUMP: Flush and invalidate L1/L2 cache\n");
	dsb();
	isb();
	dcsw_op_all(DCCISW);
	dsb();
	isb();

	NOTICE("MDUMP: Initialize ethernet\n");
	ret = mtk_eth_init();
	if (ret) {
		ERROR("MDUMP: Ethernet initialization failed with %d\n", ret);
		goto out;
	}
	NOTICE("MDUMP: Ethernet has been successfully initialized\n");

	mdump_setup_session();

	mtk_eth_write_hwaddr(macaddr);
	mtk_eth_start();

	NOTICE("MDUMP: Ethernet started\n");

	NOTICE("MDUMP: Waiting for port auto-negotiation complete\n");
	ret = mtk_eth_wait_connection_ready(5000);
	if (ret) {
		ERROR("MDUMP: Failed to wait for ethernet connection ready\n");
		goto out;
	}
	NOTICE("MDUMP: Port link up\n");

	mdump_control_check();

	send_mdump_context(handle);

	send_mdump_ranges();
	NOTICE("MDUMP: All ranges sent\n");

	mdump_end_check();

	NOTICE("MDUMP: Cleanup ethernet\n");
	mtk_eth_cleanup();

out:
	if (panic_timeout) {
		NOTICE("Rebooting ...\n");
		psci_system_reset();
	}

	NOTICE("System halted\n");

	while (1)
		;
}
