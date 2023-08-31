/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * For licensing information, see the file 'LICENCE' in this directory.
 *
 * JFFS2 wrapper to the LZMA C SDK
 *
 */

#include <common.h>
#include <config.h>
#include <malloc.h>
#include <jffs2/jffs2.h>
#include <lzma/LzmaTools.h>
#include <lzma/LzmaDec.h>

#define LZMA_BEST_DICT(n) (((int)((n) / 2)) * 2)

#define LZMA_BEST_DICT_SIZE	LZMA_BEST_DICT(0x2000)
#define LZMA_BEST_LC		(0)
#define LZMA_BEST_LP		(0)
#define LZMA_BEST_PB		(0)

static Byte props[LZMA_PROPS_SIZE];
static int propsInit;

static void *SzAlloc(void *p, size_t size)
{
	return malloc(size);
}

static void SzFree(void *p, void *address)
{
	free(address);
}

static ISzAlloc lzma_alloc = {
	.Alloc = SzAlloc,
	.Free = SzFree
};

static void lzma_init_properties(void)
{
	int i;
	UInt32 dictSize = LZMA_BEST_DICT_SIZE;

	if (propsInit)
		return;

	propsInit = 1;

	props[0] = (Byte)((LZMA_BEST_PB * 5 + LZMA_BEST_LP) * 9 + LZMA_BEST_LC);

	for (i = 11; i <= 30; i++) {
		if (dictSize <= ((UInt32)2 << i)) {
			dictSize = (2 << i);
			break;
		}
		if (dictSize <= ((UInt32)3 << i)) {
			dictSize = (3 << i);
			break;
		}
	}

	for (i = 0; i < 4; i++)
		props[1 + i] = (Byte)(dictSize >> (8 * i));
}

int lzma_decompress(unsigned char *data_in, unsigned char *cpage_out,
		      __u32 srclen, __u32 destlen)
{
	int ret;
	SizeT dl = (SizeT)destlen;
	SizeT sl = (SizeT)srclen;
	ELzmaStatus status;

	lzma_init_properties();

	ret = LzmaDecode(cpage_out, &dl, data_in, &sl, props,
		LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &lzma_alloc);

	if (ret != SZ_OK || status == LZMA_STATUS_NOT_FINISHED ||
	    dl != (SizeT)destlen)
		return -1;

	return 0;
}
