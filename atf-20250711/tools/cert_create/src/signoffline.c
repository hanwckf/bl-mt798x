/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
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

#include <openssl/asn1t.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>

#include "cert.h"
#include "debug.h"
#include "key.h"
#include "signoffline.h"

#define OFFSIGN_MSG_FILE		".msg"
#define OFFSIGN_SIG_FILE		".sig"
#define OFFSIGN_MAX_TMPFILE_LEN		256
#define OFFSIGN_MAX_CMDLINE_LEN		3 * OFFSIGN_MAX_TMPFILE_LEN + 128 + 1
#define OFFSIGN_PUBLIC_KEY_SUFFIX	"public_key.pem"
#define OFFSIGN_PRIVATE_KEY_SUFFIX	"private_key.pem"
#define OPENSSL_PKEYUTL_CMD		"openssl pkeyutl"
#define OPENSSL_PKEYUTL_OPER		"-sign"
#define OPENSSL_PKEYUTL_PADDING		"-pkeyopt rsa_padding_mode:pss"
#define OPENSSL_PKEYUTL_SALT		"-pkeyopt rsa_pss_saltlen"

struct offsign_s offsign;

static const char *hash_algs_str[] = {
	[HASH_ALG_SHA256] = "sha256",
	[HASH_ALG_SHA384] = "sha384",
	[HASH_ALG_SHA512] = "sha512",
};

const EVP_MD *get_digest(int alg);

int key_load_pub(cert_key_t *key)
{
	FILE *fp = NULL;

	if (!key->fn) {
		VERBOSE("Key not specified\n");
		return KEY_ERR_FILENAME;
	}

	fp = fopen(key->fn, "r");
	if (!fp) {
		WARN("Cannot open file %s\n", key->fn);
		return KEY_ERR_OPEN;
	}

	key->key = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
	if (!key->key) {
		ERROR("Cannot load key from %s\n", key->fn);
		return KEY_ERR_LOAD;
	}

	fclose(fp);

	return KEY_ERR_NONE;
}

static int msg_digest(void *dgst, const void *msg,
		      size_t len, const EVP_MD *md_info)
{
	int ret = 0;
	EVP_MD_CTX *ctx = NULL;

	ctx = EVP_MD_CTX_create();
	if (!ctx) {
		ERROR("EVP_MD_CTX_new() failed\n");
		return -EINVAL;
	}

	if (!EVP_DigestInit_ex(ctx, md_info, NULL)) {
		ERROR("EVP_DigestInit_ex() failed\n");
		ret = -EINVAL;
		goto end;
	}

	if (!EVP_DigestUpdate(ctx, msg, len)) {
		ERROR("EVP_DigestUpdate() failed\n");
		ret = -EINVAL;
		goto end;
	}

	if (!EVP_DigestFinal_ex(ctx, dgst, NULL)) {
		ERROR("EVP_DigestFinal_ex() failed\n");
		ret = -EINVAL;
	}

end:
	EVP_MD_CTX_destroy(ctx);

	return ret;
}

static int get_cert_digest(X509 *x, uint8_t *md, int md_alg)
{
	int ret = 0;
	long tbs_cert_len = 0;
	uint8_t *tbs_cert_p = NULL;

	/* get sign body */
	tbs_cert_len = i2d_re_X509_tbs(x, &tbs_cert_p);
	if (tbs_cert_len <= 0) {
		ERROR("i2d_re_X509_tbs() failed\n");
		ret = -EINVAL;
		goto end;
	}

	printf("TBSCertificate(sign body): %ld(bytes)\n", tbs_cert_len);

	ret = msg_digest(md, tbs_cert_p, tbs_cert_len, get_digest(md_alg));
end:
	if (tbs_cert_p)
		free(tbs_cert_p);

	return ret;
}

static int prepare_offline_sign(cert_t *cert, cert_key_t *ikey, uint8_t *md,
				int md_alg, uint8_t *sig)
{
	int ret = 0, len = 0;
	FILE *f = NULL;
	size_t length = 0, md_size = EVP_MD_size(get_digest(md_alg));
	char msg_file[OFFSIGN_MAX_TMPFILE_LEN] = {0};

	len = snprintf(msg_file, sizeof(msg_file), "%s%s", cert->fn,
							   OFFSIGN_MSG_FILE);
	if (len < 0)
		return -EINVAL;

	f = fopen(msg_file, "w");
	if (!f) {
		ERROR("Failed to open %s\n", msg_file);
		return -EINVAL;
	}

	length = fwrite(md, sizeof(uint8_t), md_size, f);
	if (!length || length != md_size) {
		ERROR("Failed to write to %s\n", msg_file);
		ret = -EINVAL;
	}

	fclose(f);

	return ret;
}

static int sign_offline(cert_t *cert, cert_key_t *ikey, uint8_t *md,
			int md_alg, uint8_t *sig)
{
	int len = 0, ret = 0;
	const char *pub_key_name = ikey->fn;
	char key_name[OFFSIGN_MAX_TMPFILE_LEN] = {0};
	char cmd[OFFSIGN_MAX_CMDLINE_LEN] = {0};
	char *p = NULL;

	if ((EVP_PKEY_base_id(ikey->key) == EVP_PKEY_RSA)) {
		p = strstr(pub_key_name, OFFSIGN_PUBLIC_KEY_SUFFIX);
		if (!p)
			return -EINVAL;

		strncpy(key_name, pub_key_name, p - pub_key_name);
		strncpy(key_name + (p - pub_key_name), OFFSIGN_PRIVATE_KEY_SUFFIX,
			strlen(OFFSIGN_PRIVATE_KEY_SUFFIX));

		printf("Cert: %s\n", cert->cn);
		printf("IKey: %s\n", ikey->desc);
		printf("Algo: rsa%d + %s\n", EVP_PKEY_bits(ikey->key),
					     hash_algs_str[md_alg]);

		len = snprintf(cmd, sizeof(cmd),
			"%s %s -in %s%s -inkey %s -out %s%s -pkeyopt digest:%s %s %s:%d",
								OPENSSL_PKEYUTL_CMD,
								OPENSSL_PKEYUTL_OPER,
								cert->fn,
								OFFSIGN_MSG_FILE,
								key_name,
								cert->fn,
								OFFSIGN_SIG_FILE,
								hash_algs_str[md_alg],
								OPENSSL_PKEYUTL_PADDING,
								OPENSSL_PKEYUTL_SALT,
								EVP_MD_size(get_digest(md_alg)));
		if (len < 0)
			return -EINVAL;

		printf("%s\n", cmd);

		if (system(cmd) == -1) {
			ERROR("Failed to sign FIT: %s\n", OPENSSL_PKEYUTL_CMD);
			ret = -EINVAL;
		}
	} else if (EVP_PKEY_base_id(ikey->key) == EVP_PKEY_EC) {
		const EC_KEY *ec_key = EVP_PKEY_get0_EC_KEY(ikey->key);
		if (!ec_key) {
			ERROR("Failed to get EC_KEY\n");
			ret = -EINVAL;
			return ret;
		}

		const EC_GROUP *ec_group = EC_KEY_get0_group(ec_key);
		if (!ec_group) {
			ERROR("Failed to get EC_GROUP\n");
			ret = -EINVAL;
			return ret;
		}

		int curve_name = EC_GROUP_get_curve_name(ec_group);
		switch (curve_name) {
		case NID_X9_62_prime256v1:
			break;
		case NID_secp384r1:
			break;
		case NID_brainpoolP256r1:
			break;
		case NID_brainpoolP256t1:
			break;
		default:
			break;
		}
	}

	return ret;
}

static int post_offline_sign(cert_t *cert, cert_key_t *ikey, uint8_t *md,
			     int md_alg, uint8_t *sig)
{
	int ret = 0, len = 0;
	size_t length = 0;
	FILE *f = NULL;
	int key_size = EVP_PKEY_bits(ikey->key) >> 3;
	char sig_file[OFFSIGN_MAX_TMPFILE_LEN] = {0};

	len = snprintf(sig_file, sizeof(sig_file), "%s%s", cert->fn,
							   OFFSIGN_SIG_FILE);
	if (len < 0)
		return -EINVAL;

	f = fopen(sig_file, "rb");
	if (!f) {
		ERROR("Failed to open %s\n", sig_file);
		return -EINVAL;
	}

	length = fread(sig, sizeof(uint8_t), key_size, f);
	if (!length) {
		ERROR("Failed to read from %s\n", sig_file);
		fclose(f);
		return -EINVAL;
	}

	if (length != key_size) {
		ERROR("Signature length is invalid: len(%lu), key_size(%d)\n", length, key_size);
		ret = -EINVAL;
	}

	fclose(f);

	return ret;
}

static int _offline_sign(cert_t *cert, cert_key_t *ikey, uint8_t *md,
			 int md_alg, uint8_t *sig)
{
	int ret;

	ret = prepare_offline_sign(cert, ikey, md, md_alg, sig);
	if (ret) {
		ERROR("prepare_offline_sign() failed\n");
		return ret;
	}

	ret = sign_offline(cert, ikey, md, md_alg, sig);
	if (ret) {
		ERROR("sign_offline() failed\n");
		return ret;
	}

	ret = post_offline_sign(cert, ikey, md, md_alg, sig);
	if (ret) {
		ERROR("post_offline_sign() failed\n");
		return ret;
	}

	return 0;
}

static int asn1_parse(const uint8_t *asn1_p, long length, const uint8_t **sig_val_p,
		      long *sig_val_len, long ofs)
{
	long len = 0;
	const uint8_t *p = NULL, *orig_p = NULL;
	int ret = 0, tag = 0, xclass = 0, hdr_len = 0;

	/* Certificate ::= SEQUENCE {
		TBSCertificate		SEQUENCE,
		SignatureAlgorithm	SEQUENCE,
		SignatureValue		BIT_STRING }
	*/
	p = asn1_p;
	orig_p = p;
	ret = ASN1_get_object(&p, &len, &tag, &xclass, length);
	if (ret & 0x80 || !(ret & V_ASN1_CONSTRUCTED) || !(tag == V_ASN1_SEQUENCE)) {
		ERROR("Error at offset(%ld)\n", ofs + (long)(orig_p - asn1_p));
		return -EINVAL;
	}
	hdr_len = p - orig_p;
	length -= hdr_len;

	VERBOSE("%ld: hl=%d, len=%ld\n", ofs + (long)(orig_p - asn1_p), hdr_len, len);

	/* TBSCertificate ::= SEQUENCE */
	orig_p = p;
	ret = ASN1_get_object(&p, &len, &tag, &xclass, length);
	if (ret & 0x80 || !(ret & V_ASN1_CONSTRUCTED) || !(tag == V_ASN1_SEQUENCE)) {
		ERROR("Error at offset(%ld)\n", ofs + (long)(orig_p - asn1_p));
		return -EINVAL;
	}
	hdr_len = p - orig_p;
	length -= hdr_len + len;
	p += len;

	VERBOSE("%ld: hl=%d, len=%ld\n", ofs + (long)(orig_p - asn1_p), hdr_len, len);

	/* SignatureAlgorithm ::= SEQUENCE */
	orig_p = p;
	ret = ASN1_get_object(&p, &len, &tag, &xclass, length);
	if (ret & 0x80 || !(ret & V_ASN1_CONSTRUCTED) || !(tag == V_ASN1_SEQUENCE)) {
		ERROR("Error at offset(%ld)\n", ofs + (long)(orig_p - asn1_p));
		return -EINVAL;
	}
	hdr_len = p - orig_p;
	length -= hdr_len + len;
	p += len;

	VERBOSE("%ld: hl=%d, len=%ld\n", ofs + (long)(orig_p - asn1_p), hdr_len, len);

	/* SignatureValue ::= BIT STRING */
	orig_p = p;
	ret = ASN1_get_object(&p, &len, &tag, &xclass, length);
	if (ret & 0x80 || !(tag == V_ASN1_BIT_STRING)) {
		ERROR("Error at offset(%ld)\n", ofs + (long)(orig_p - asn1_p));
		return -EINVAL;
	}
	hdr_len = p - orig_p;

	VERBOSE("%ld: hl=%d, len=%ld\n", ofs + (long)(orig_p - asn1_p), hdr_len, len);

	/* leading byte always 0 */
	*sig_val_p = p + 1;
	*sig_val_len = len - 1;

	return 0;
}

static int put_signature(X509 **x, cert_t *cert, uint8_t *sig, int sig_len)
{
	int ret = 0;
	long sig_val_len = 0, cert_len = 0;
	uint8_t *p = NULL, *sig_val_p = NULL, *cert_p = NULL;

	cert_len = i2d_X509(*x, &cert_p);
	if (cert_len <= 0) {
		ERROR("i2d_X509() failed\n");
		ret = -EINVAL;
		goto end;
	}

	/* get signature position */
	ret = asn1_parse(cert_p, cert_len,
			 (const uint8_t **)&sig_val_p, &sig_val_len, 0);
	if (ret)
		goto end;

	if (sig_len != sig_val_len) {
		ERROR("signature size(%d) not equal to sequence size(%ld)\n",
			sig_len, sig_val_len);
		ret = -EINVAL;
		goto end;
	}

	/* set correct signature value */
	memcpy(cert_p + (sig_val_p - cert_p), sig, sig_len);

	/* update the content of cert. */
	X509_free(*x);
	*x = NULL;

	p = cert_p;
	if (!d2i_X509(x, (const unsigned char **)&p, cert_len)) {
		ERROR("d2i_X509() failed\n");
		ret = -EINVAL;
	}

end:
	if (cert_p)
		free(cert_p);

	return ret;
}

int offline_sign(X509 **x, cert_t *cert, cert_key_t *ikey, int md_alg)
{
	int ret, rc = 0;
	int key_size = EVP_PKEY_bits(ikey->key) >> 3;
	uint8_t md[SHA512_DIGEST_LENGTH] = { 0 };
	uint8_t *sig = NULL;

	sig = calloc(key_size, sizeof(uint8_t));
	if (!sig)
		return rc;

	printf("\nSign %s offline\n", cert->cn);

	ret = get_cert_digest(*x, md, md_alg);
	if (ret) {
		ERROR("Cannot get certificate's digest: %d\n", ret);
		goto END;
	}

	ret = _offline_sign(cert, ikey, md, md_alg, sig);
	if (ret) {
		ERROR("Cannot sign message: %d\n", ret);
		goto END;
	}

	ret = put_signature(x, cert, sig, key_size);
	if (ret) {
		ERROR("Cannot put signature back: %d\n", ret);
		goto END;
	}

	rc = 1;
END:
	free(sig);

	return rc;
}
