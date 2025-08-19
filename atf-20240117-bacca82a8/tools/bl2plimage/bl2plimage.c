// SPDX-License-Identifier: BSD-3-Clause
/*
 * Generate BL2 payload header
 *
 * Copyright (C) 2021 MediaTek Inc.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>

#include <sys/stat.h>

#include <bl2pl.h>

#if defined(__MINGW32__) || defined(__CYGWIN__) || defined(__MSYS__) || \
    defined(WIN32)
#define bswap32(x)	\
	(((((uint32_t)(x)) >> 24) & 0xff) | \
	(((((uint32_t)(x)) >> 16) & 0xff) << 8) | \
	(((((uint32_t)(x)) >> 8) & 0xff) << 16) | \
	((((uint32_t)(x)) & 0xff) << 24))

#define __cpu_to_le32(x)	(x)
#define __cpu_to_be32(x)	bswap32(x)
#else
#include <asm/byteorder.h>
#endif

static const char *input_file;
static const char *output_file;
static uint32_t load_addr;
static bool big_endian;

static void err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void usage(FILE *con, const char *progname, int exitcode)
{
	const char *prog;
	size_t len;

	len = strlen(progname);
	prog = progname + len - 1;

	while (prog > progname) {
		if (*prog == '\\' || *prog == '/') {
			prog++;
			break;
		}

		prog--;
	}

	fprintf(con, "BL2 payload image tool\n");
	fprintf(con, "\n");
	fprintf(con, "Usage: %s [options] <input_file> <output_file>\n", prog);
	fprintf(con, "\n");
	fprintf(con, "Options:\n");
	fprintf(con, "\t-h         display help message\n");
	fprintf(con, "\t-a <addr>  load address\n");
	fprintf(con, "\t-b         use big-endian\n");
	fprintf(con, "\n");

	exit(exitcode);
}

static int parse_args(int argc, char *argv[])
{
	int opt;

	static const char *optstring = "a:hb";

	if (argc == 1)
		usage(stdout, argv[0], 0);

	opterr = 0;

	while ((opt = getopt(argc, argv, optstring)) >= 0) {
		switch (opt) {
		case 'a':
			if (!isxdigit(optarg[0])) {
				err("Invalid load address - %s\n", optarg);
				return -EINVAL;
			}

			load_addr = strtoul(optarg, NULL, 16);
			break;
		case 'b':
			big_endian = true;
			break;
		case 'h':
			usage(stdout, argv[0], 0);
			break;
		default:
			usage(stderr, argv[0], EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		err("Input file not specified\n");
		return -EINVAL;
	}

	if (optind + 1 >= argc) {
		err("Output file not specified\n");
		return -EINVAL;
	}

	input_file = argv[optind];
	output_file = argv[optind + 1];

	return 0;
}

static int assemble_file(void)
{
	size_t len;
	int fd, ret;
	struct stat st;
	struct bl2pl_hdr hdr;
	uint8_t buff[0x10000];
	FILE *fin, *fout = NULL;

	fin = fopen(input_file, "rb");
	if (!fin) {
		err("Unable to open input file %s\n", input_file);
		ret = -1;
		goto cleanup;
	}

	fd = fileno(fin);
	if (fd < 0) {
		err("Unable to get file descriptor\n");
		ret = -errno;
		goto cleanup;
	}

	ret = fstat(fd, &st);
	if (ret < 0) {
		err("Unable to get file stat\n");
		ret = -errno;
		goto cleanup;
	}

	if (st.st_size > UINT_MAX) {
		err("Input file is too large\n");
		ret = -1;
		goto cleanup;
	}

	if (big_endian) {
		hdr.magic = __cpu_to_be32(BL2PL_HDR_MAGIC);
		hdr.load_addr = __cpu_to_be32(load_addr);
		hdr.size = __cpu_to_be32(st.st_size);
	} else {
		hdr.magic = __cpu_to_le32(BL2PL_HDR_MAGIC);
		hdr.load_addr = __cpu_to_le32(load_addr);
		hdr.size = __cpu_to_le32(st.st_size);
	}

	hdr.unused = 0;

	fout = fopen(output_file, "wb");
	if (!fout) {
		err("Unable to open output file %s\n", output_file);
		ret = -1;
		goto cleanup;
	}

	if (fwrite(&hdr, sizeof(hdr), 1, fout) != 1) {
		err("Failed to write header\n");
		ret = -ferror(fout);
		goto cleanup;
	}

	while (true) {
		len = fread(buff, 1, sizeof(buff), fin);
		if (len < sizeof(buff)) {
			if (!feof(fin)) {
				err("Failed to read file\n");
				ret = -ferror(fin);
				goto cleanup;
			}

			if (!len)
				break;
		}

		if (fwrite(buff, 1, len, fout) != len) {
			err("Failed to write file\n");
			ret = -ferror(fout);
			goto cleanup;
		}
	}

	ret = 0;

cleanup:
	if (fin)
		fclose(fin);

	if (fout)
		fclose(fout);

	return ret;
}

int main(int argc, char *argv[])
{
	if (parse_args(argc, argv))
		return 1;

	if (assemble_file())
		return 2;

	return 0;
}
