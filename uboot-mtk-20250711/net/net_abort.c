// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2026 Yuzhii0718
 *
 * Network boot abort support (uBootEnter.py / BreedEnter compatible)
 *
 * Let a host tool abort the boot countdown over the network and force
 * U-Boot into its web failsafe.
 *
 *   - UDP listener on port 37541
 *   - magic trigger payload "UBOOT:ABORT" or "BREED:ABORT" (11 bytes)
 *   - broadcast reply "<PROTO>:ABORTED" (13 bytes), src/dst port 37540
 *   - enter the blocking web failsafe (httpd) afterwards
 *
 * Disable at runtime:  setenv disable_net_abort 1
 */

#include <command.h>
#include <env.h>
#include <net.h>
#include <stdio.h>
#include <time.h>
#include <vsprintf.h>
#include <cyclic.h>
#include <linux/delay.h>

#include <net_abort.h>

#define NET_ABORT_PORT		37541	/* listener port for magic packet */
#define NET_ABORTED_PORT	37540	/* reply port (src and dst) */
#define NET_ABORT_MAGIC_UBOOT	"UBOOT:ABORT"
#define NET_ABORT_MAGIC_BREED	"BREED:ABORT"
#define NET_ABORT_MAGIC_LEN	11
#define NET_ABORTED_REPLY_UBOOT	"UBOOT:ABORTED"
#define NET_ABORTED_REPLY_BREED	"BREED:ABORTED"
#define NET_ABORTED_REPLY_LEN	13
#define NET_ABORT_WAIT_SEC	3	/* link settle + listen window */

static rxhand_f *net_abort_prev_udp_handler;
static bool net_abort_enabled;
static bool net_abort_pkt_received;
static bool net_abort_breed_trigger;

static struct in_addr net_abort_str_to_ip(const char *str, const char *def)
{
	if (str && str[0])
		return string_to_ip(str);
	if (def && def[0])
		return string_to_ip(def);
	return string_to_ip("0.0.0.0");
}

static void net_abort_udp_handler(uchar *pkt, unsigned int dport,
				  struct in_addr sip, unsigned int sport,
				  unsigned int len)
{
	/* Only react to the exact magic payload on the well known port */
	if (dport == NET_ABORT_PORT) {
		printf("netabort: UDP pkt sport %u len %u\n", sport, len);
		if (len == NET_ABORT_MAGIC_LEN &&
		    (!memcmp(pkt, NET_ABORT_MAGIC_UBOOT, NET_ABORT_MAGIC_LEN) ||
		     !memcmp(pkt, NET_ABORT_MAGIC_BREED, NET_ABORT_MAGIC_LEN))) {
			net_abort_breed_trigger =
				!memcmp(pkt, NET_ABORT_MAGIC_BREED,
					NET_ABORT_MAGIC_LEN);
			net_abort_pkt_received = true;
		}
	}

	/* Chain: keep previously installed UDP services working */
	if (net_abort_prev_udp_handler)
		net_abort_prev_udp_handler(pkt, dport, sip, sport, len);
}

void net_abort_prepare(void)
{
	const char *wait_str;
	int wait_sec = NET_ABORT_WAIT_SEC;
	ulong ts;

	if (env_get("disable_net_abort"))
		return;

	net_abort_enabled = false;
	net_abort_pkt_received = false;
	net_abort_breed_trigger = false;

	/* The web failsafe / httpd uses net_ip & net_netmask */
	net_ip = net_abort_str_to_ip(env_get("ipaddr"), CONFIG_IPADDR);
	net_netmask = net_abort_str_to_ip(env_get("netmask"), CONFIG_NETMASK);

	net_init();

	if (eth_init() != 0) {
		eth_halt();
		mdelay(300);
		if (eth_init() != 0) {
			printf("netabort: ethernet init failed, "
			       "network abort disabled\n");
			return;
		}
	}

	net_abort_prev_udp_handler = net_get_udp_handler();
	net_set_udp_handler(net_abort_udp_handler);
	net_abort_enabled = true;

	printf("netabort: check enabled (UDP port %d)\n", NET_ABORT_PORT);

	/*
	 * PHY autonegotiation takes longer than the boot menu countdown,
	 * so give the link time to settle and listen for the magic packet
	 * right away.  The host tool (uBootEnter.py) re-sends every 300 ms.
	 * Press any key (or setenv net_abort_wait 0) to skip.
	 */
	wait_str = env_get("net_abort_wait");
	if (wait_str && wait_str[0])
		wait_sec = simple_strtol(wait_str, NULL, 10);
	if (wait_sec < 0)
		wait_sec = 0;

	if (wait_sec > 0) {
		printf("netabort: waiting for trigger (%ds, key to skip)...\n",
		       wait_sec);
		while (wait_sec > 0 && !net_abort_pkt_received) {
			ts = get_timer(0);
			do {
				if (tstc()) {
					getchar();
					goto listen_done;
				}
				schedule();
				net_abort_poll();
				udelay(10000);
			} while (!net_abort_pkt_received && get_timer(ts) < 1000);
			if (!net_abort_pkt_received)
				--wait_sec;
		}
	}
listen_done:
	puts("\n");
}

void net_abort_poll(void)
{
	if (!net_abort_enabled || net_abort_pkt_received)
		return;

	/* Pump received packets through the legacy net stack */
	eth_rx();
}

bool net_abort_detected(void)
{
	return net_abort_pkt_received;
}

void net_abort_finish(void)
{
	if (!net_abort_enabled)
		return;

	/* Unhook only if we are still the active handler */
	if (net_get_udp_handler() == net_abort_udp_handler)
		net_set_udp_handler(net_abort_prev_udp_handler);
	net_abort_enabled = false;

	if (net_abort_pkt_received) {
		const char *reply = net_abort_breed_trigger ?
			NET_ABORTED_REPLY_BREED : NET_ABORTED_REPLY_UBOOT;

		printf("netabort: triggered (%s), entering web failsafe\n",
		       net_abort_breed_trigger ? "BREED" : "UBOOT");

		/* Reply "<PROTO>:ABORTED" to broadcast:37540 from :37540 */
		memcpy(net_tx_packet + net_eth_hdr_size() + IP_UDP_HDR_SIZE,
		       reply, NET_ABORTED_REPLY_LEN);
		net_send_udp_packet((uchar *)net_bcast_ethaddr,
				    string_to_ip("255.255.255.255"),
				    NET_ABORTED_PORT, NET_ABORTED_PORT,
				    NET_ABORTED_REPLY_LEN);

		eth_halt();
		mdelay(500);

		/* Blocking web failsafe: HTTP:80 + DHCP + DNS */
		run_command("httpd", 0);
	} else {
		eth_halt();
	}
}

static int do_netabort(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "status")) {
		printf("netabort: %s (UDP port %d)\n",
		       net_abort_enabled ? "enabled" : "disabled",
		       NET_ABORT_PORT);
		if (net_abort_pkt_received)
			printf("netabort: trigger received (%s)\n",
			       net_abort_breed_trigger ? "BREED" : "UBOOT");
		else
			puts("netabort: trigger not received\n");
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "listen")) {
		const char *old_wait;
		char buf[16];
		int secs = -1;

		if (argc > 2) {
			secs = simple_strtol(argv[2], NULL, 10);
			if (secs < 0)
				secs = -1;
		}
		if (secs >= 0) {
			/* Override the net_abort_wait window temporarily */
			old_wait = env_get("net_abort_wait");
			snprintf(buf, sizeof(buf), "%d", secs);
			env_set("net_abort_wait", buf);
		}

		net_abort_pkt_received = false;
		net_abort_prepare();

		if (secs >= 0)
			env_set("net_abort_wait", old_wait ? old_wait : "");

		if (!net_abort_enabled) {
			printf("netabort: listen unavailable "
			       "(setenv disable_net_abort to re-enable)\n");
			return CMD_RET_FAILURE;
		}

		net_abort_finish();
		return CMD_RET_SUCCESS;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	netabort,	CONFIG_SYS_MAXARGS,	1,	do_netabort,
	"network boot abort (uBootEnter.py / BreedEnter compatible)",
	"listen [secs] - listen for the UBOOT:ABORT / BREED:ABORT magic\n"
	"	packets (UDP broadcast port 37541, sent by uBootEnter.py or\n"
	"	BreedEnter) and enter the blocking web failsafe (httpd) once\n"
	"	one is received. The matching <PROTO>:ABORTED reply is sent.\n"
	"	secs sets the link settle + listen window (default 3).\n"
	"netabort status - show abort detection state\n"
);
