/*
 * Copyright (c) 2022, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-CLause
 */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>

#include <openssl/asn1t.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>

#define SHA256_DIGEST_LEN		32
#define SHA384_DIGEST_LEN		48
#define SHA512_DIGEST_LEN		64

#define MAX_SIG_LEN			512

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

enum {
	HASH_ALG_SHA256 = 0,
	HASH_ALG_SHA384,
	HASH_ALG_SHA512
};

static const char *hash_algs[] = {
	[HASH_ALG_SHA256] = "SHA256",
	[HASH_ALG_SHA384] = "SHA384",
	[HASH_ALG_SHA512] = "SHA512",
};

static bool verbose;

void usage(void)
{
	printf("FIP offline sign tool\n");
	printf("Usage:\n");
	printf("Extract message to be signed from certificate:\n");
	printf("signoffline -c [certificate] -p [fip public key] -m [message]\n");
	printf("Use signature to assemble new certificate:\n");
	printf("signoffline -c [certificate] -p [fip public key] -s [signature] -o [new certificate]\n\n");
	printf("-c		[trusted_key_certificate]\n");
	printf("-f		[public key format, default is DER]\n");
	printf("-h		[print this message]\n");
	printf("-m		[message to be signed]\n");
	printf("-p		[fip public key]\n");
	printf("-o		[new certificate]\n");
	printf("-s		[signature]\n\n");
	exit(0);
}

void hex_print(const uint8_t *ptr, size_t len)
{
	int i;
	for (i = 0; i < len; i++) {
		printf("%02x", ptr[i]);
	}
	printf("\n");
}

static int get_hash_alg(const char *str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hash_algs); i++) {
		if (!strcmp(str, hash_algs[i]))
			return i;
	}

	return -1;
}

static int read_file(uint8_t *buf, size_t *len, size_t size, char *filename)
{
	int ret = 0;
	FILE *f = NULL;

	if (!buf || !len || !size || !filename)
		return -EINVAL;

	f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "Failed to open %s\n", filename);
		return -EINVAL;
	}

	*len = fread(buf, sizeof(uint8_t), size, f);
	if (*len <= 0) {
		fprintf(stderr, "Failed to read from %s\n", filename);
		ret = -EINVAL;
	}

	fclose(f);

	return ret;
}

static int write_file(uint8_t *buf, size_t *len, size_t size, char *filename)
{
	int ret = 0;
	FILE *f = NULL;

	if (!buf || !len || !size || !filename)
		return -EINVAL;

	f = fopen(filename, "w");
	if (!f) {
		fprintf(stderr, "Failed to open %s\n", filename);
		return -EINVAL;
	}

	*len = fwrite(buf, sizeof(uint8_t), size, f);
	if (*len <= 0) {
		fprintf(stderr, "Failed to write to %s\n", filename);
		ret = -EINVAL;
	}

	fclose(f);

	return ret;
}

static int msg_digest(void *dgst, const void *msg,
		      size_t len, const EVP_MD *md_info)
{
	int ret = 0;
	EVP_MD_CTX *ctx = NULL;

	if (!dgst || !msg || !len || !md_info)
		return -EINVAL;

	ctx = EVP_MD_CTX_create();
	if (!ctx) {
		fprintf(stderr, "EVP_MD_CTX_new() failed\n");
		return -EINVAL;
	}

	if (!EVP_DigestInit_ex(ctx, md_info, NULL)) {
		fprintf(stderr, "EVP_DigestInit_ex() failed\n");
		ret = -EINVAL;
		goto msg_digest_end;
	}

	if (!EVP_DigestUpdate(ctx, msg, len)) {
		fprintf(stderr, "EVP_DigestUpdate() failed\n");
		ret = -EINVAL;
		goto msg_digest_end;
	}

	if (!EVP_DigestFinal_ex(ctx, dgst, NULL)) {
		fprintf(stderr, "EVP_DigestFinal_ex() failed\n");
		ret = -EINVAL;
	}

msg_digest_end:
	EVP_MD_CTX_destroy(ctx);

	return ret;
}

static int load_x509(X509 **cert, char *cert_file)
{
	int ret = 0;
	FILE *f = NULL;

	if (!cert || !cert_file)
		return -EINVAL;

	f = fopen(cert_file, "rb");
	if (!f) {
		fprintf(stderr, "Failed to open %s\n", cert_file);
		return -EINVAL;
	}

	/* DER */
	*cert = d2i_X509_fp(f, NULL);
	if (!(*cert)) {
		fprintf(stderr, "d2i_X509_fp() failed\n");
		ret = -EINVAL;
	}

	fclose(f);

	return ret;
}

static int load_pubk(EVP_PKEY **pkey, char *pubk_file, char *format)
{
	int ret = 0;
	FILE *f = NULL;

	if (!pkey || !pubk_file)
		return -EINVAL;

	f = fopen(pubk_file, "rb");
	if (!f) {
		fprintf(stderr, "Failed to open %s\n", pubk_file);
		return -EINVAL;
	}

	if (format && (!strcmp(format, "PEM") || !strcmp(format, "pem"))) {
		/* PEM */
		*pkey = PEM_read_PUBKEY(f, NULL, NULL, NULL);
		if (!(*pkey)) {
			fprintf(stderr, "PEM_read_PUBKEY() failed\n");
			ret = -EINVAL;
		}
	} else {
		/* DER */
		*pkey = d2i_PUBKEY_fp(f, NULL);
		if (!(*pkey)) {
			fprintf(stderr, "d2i_PUBKEY_fp() failed\n");
			ret = -EINVAL;
		}
	}

	fclose(f);

	return ret;
}

static int set_cert_pubk(X509 *cert, char *pubk_file, char *format)
{
	int ret = 0;
	int len = 0;
	EVP_PKEY *pkey = NULL;

	if (!cert || !pubk_file)
		return -EINVAL;

	ret = load_pubk(&pkey, pubk_file, format);
	if (ret)
		return -EINVAL;

	/* set correct pubk */
	if (!X509_set_pubkey(cert, pkey)) {
		fprintf(stderr, "X509_set_pubkey() failed\n");
		ret = -EINVAL;
		goto set_cert_pubk_end;
	}

	/* update tbs certificate */
	len = i2d_re_X509_tbs(cert, NULL);
	if (len <= 0) {
		fprintf(stderr, "i2d_re_X509_tbs() failed\n");
		ret = -EINVAL;
	}

set_cert_pubk_end:
	EVP_PKEY_free(pkey);

	return ret;
}

static int asn1_parse(const uint8_t *asn1_p, long length,
		      const uint8_t **sig_val_p, long *sig_val_len, long ofs)
{
	const uint8_t *p = NULL, *orig_p = NULL;
	long len = 0;
	int ret = 0, tag = 0, xclass = 0, hdr_len = 0;

	if (!asn1_p || !length || !sig_val_p || !sig_val_len)
		return -EINVAL;

	/* Certificate ::= SEQUENCE {
		TBSCertificate		SEQUENCE,
		SignatureAlgorithm	SEQUENCE,
		SignatureValue		BIT_STRING }
	*/
	p = asn1_p;
	orig_p = p;
	ret = ASN1_get_object(&p, &len, &tag, &xclass, length);
	if (ret & 0x80 || !(ret & V_ASN1_CONSTRUCTED) || !(tag == V_ASN1_SEQUENCE)) {
		fprintf(stderr, "Error at offset(%ld)\n", ofs + (long)(orig_p - asn1_p));
		return -EINVAL;
	}
	hdr_len = p - orig_p;
	length -= hdr_len;

	if (verbose)
		printf("%ld: hl=%d, len=%ld\n", ofs + (long)(orig_p - asn1_p), hdr_len, len);

	/* TBSCertificate ::= SEQUENCE */
	orig_p = p;
	ret = ASN1_get_object(&p, &len, &tag, &xclass, length);
	if (ret & 0x80 || !(ret & V_ASN1_CONSTRUCTED) || !(tag == V_ASN1_SEQUENCE)) {
		fprintf(stderr, "Error at offset(%ld)\n", ofs + (long)(orig_p - asn1_p));
		return -EINVAL;
	}
	hdr_len = p - orig_p;
	length -= hdr_len + len;
	p += len;

	if (verbose)
		printf("%ld: hl=%d, len=%ld\n", ofs + (long)(orig_p - asn1_p), hdr_len, len);

	/* SignatureAlgorithm ::= SEQUENCE */
	orig_p = p;
	ret = ASN1_get_object(&p, &len, &tag, &xclass, length);
	if (ret & 0x80 || !(ret & V_ASN1_CONSTRUCTED) || !(tag == V_ASN1_SEQUENCE)) {
		fprintf(stderr, "Error at offset(%ld)\n", ofs + (long)(orig_p - asn1_p));
		return -EINVAL;
	}
	hdr_len = p - orig_p;
	length -= hdr_len + len;
	p += len;

	if (verbose)
		printf("%ld: hl=%d, len=%ld\n", ofs + (long)(orig_p - asn1_p), hdr_len, len);

	/* SignatureValue ::= BIT STRING */
	orig_p = p;
	ret = ASN1_get_object(&p, &len, &tag, &xclass, length);
	if (ret & 0x80 || !(tag == V_ASN1_BIT_STRING)) {
		fprintf(stderr, "Error at offset(%ld)\n", ofs + (long)(orig_p - asn1_p));
		return -EINVAL;
	}
	hdr_len = p - orig_p;

	if (verbose)
		printf("%ld: hl=%d, len=%ld\n", ofs + (long)(orig_p - asn1_p), hdr_len, len);

	/* leading byte always 0 */
	*sig_val_p = p + 1;
	*sig_val_len = len - 1;

	return 0;
}

static int certificate_digest(X509 *cert, char *msg_file)
{
	int ret = 0, hash_nid = 0, hash_alg = 0;
	long md_len = 0, tbs_cert_len = 0;
	size_t len = 0;
	uint8_t dgst[SHA512_DIGEST_LEN] = {0};
	uint8_t *tbs_cert_p = NULL;
	const EVP_MD *md_info = NULL;

	if (!cert || !msg_file)
		return -EINVAL;

	tbs_cert_len = i2d_re_X509_tbs(cert, NULL);
	if (tbs_cert_len <= 0) {
		fprintf(stderr, "i2d_re_X509_tbs() failed\n");
		return -EINVAL;
	}

	tbs_cert_p = calloc(tbs_cert_len, sizeof(uint8_t));
	if (!tbs_cert_p)
		return -ENOMEM;

	/* get sign body */
	tbs_cert_len = i2d_re_X509_tbs(cert, &tbs_cert_p);
	if (tbs_cert_len <= 0) {
		fprintf(stderr, "i2d_re_X509_tbs() failed\n");
		ret = -EINVAL;
		goto certificate_digest_end;
	}
	tbs_cert_p -= tbs_cert_len;

	printf("TBSCertificate: %ld(bytes)\n",tbs_cert_len);

	if (!X509_get_signature_info(cert, &hash_nid, NULL, NULL, NULL)) {
		fprintf(stderr, "X509_get_signature_info() failed\n");
		ret = -EINVAL;
		goto certificate_digest_end;
	}

	hash_alg = get_hash_alg(OBJ_nid2sn(hash_nid));
	if (hash_alg < 0) {
		ret = -EINVAL;
		goto certificate_digest_end;
	}

	if (hash_alg == HASH_ALG_SHA384) {
		md_info = EVP_sha384();
		md_len = SHA384_DIGEST_LEN;
	} else if (hash_alg == HASH_ALG_SHA512) {
		md_info = EVP_sha512();
		md_len = SHA512_DIGEST_LEN;
	} else {
		md_info = EVP_sha256();
		md_len = SHA256_DIGEST_LEN;
	}

	ret = msg_digest(dgst, tbs_cert_p, tbs_cert_len, md_info);
	if (ret)
		goto certificate_digest_end;

	printf("Digest(SHA%ld)=", md_len << 3);
	hex_print(dgst, md_len);

	ret = write_file(dgst, &len, md_len, msg_file);

certificate_digest_end:
	free(tbs_cert_p);

	return ret;
}

static int put_signature(X509 *cert, char *sig_file, char *out_file)
{
	int ret = 0;
	long sig_val_len = 0, cert_len = 0;
	size_t len = 0, sig_len = 0;
	uint8_t sig[MAX_SIG_LEN] = {0};
	uint8_t *sig_val_p = NULL, *cert_p = NULL;

	if (!cert || !sig_file || !out_file)
		return -EINVAL;

	ret = read_file(sig, &sig_len, sizeof(sig), sig_file);
	if (ret)
		return ret;

	cert_len = i2d_X509(cert, NULL);
	if (cert_len <= 0) {
		fprintf(stderr, "i2d_X509() failed\n");
		return -EINVAL;
	}

	cert_p = calloc(cert_len, sizeof(uint8_t));
	if (!cert_p)
		return -ENOMEM;

	cert_len = i2d_X509(cert, &cert_p);
	if (cert_len <= 0) {
		fprintf(stderr, "i2d_X509() failed\n");
		ret = -EINVAL;
		goto put_signature_end;
	}
	cert_p -= cert_len;

	/* get signature position */
	ret = asn1_parse(cert_p, cert_len,
			 (const uint8_t **)&sig_val_p, &sig_val_len, 0);
	if (ret)
		goto put_signature_end;

	if (sig_len != sig_val_len) {
		fprintf(stderr, "%s: signature size(%lu) not equal to sequence size(%ld)\n",
				__func__, sig_len, sig_val_len);
		ret = -EINVAL;
		goto put_signature_end;
	}

	/* set correct signature value */
	memcpy(cert_p + (sig_val_p - cert_p), sig, sig_len);

	printf("New Signature Value(%lu)=", sig_len << 3);
	hex_print(sig, sig_len);

	ret = write_file(cert_p, &len, cert_len, out_file);

put_signature_end:
	free(cert_p);

	return ret;
}

int main(int argc, char *argv[])
{
	int ret = 0, opt = 0;
	char *cert_file = NULL, *msg_file = NULL, *sig_file = NULL;
	char *out_file = NULL, *pubk_file = NULL, *key_format = NULL;
	X509 *cert = NULL;

	while ((opt = getopt(argc, argv, "c:f:hm:o:p:s:v")) > 0) {
		switch (opt) {
		case 'c':
			cert_file = optarg;
			break;
		case 'f':
			key_format = optarg;
			break;
		case 'h':
			usage();
			break;
		case 'm':
			msg_file = optarg;
			break;
		case 'o':
			out_file = optarg;
			break;
		case 'p':
			pubk_file = optarg;
			break;
		case 's':
			sig_file = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage();
			break;
		}
	}

	if (!cert_file || !pubk_file || (!msg_file && (!sig_file || !out_file)))
		usage();

	ret = load_x509(&cert, cert_file);
	if (ret)
		goto end;

	ret = set_cert_pubk(cert, pubk_file, key_format);
	if (ret)
		goto end;

	if (msg_file)
		ret = certificate_digest(cert, msg_file);
	else
		ret = put_signature(cert, sig_file, out_file);

end:
	if (cert)
		X509_free(cert);

	return ret;
}
