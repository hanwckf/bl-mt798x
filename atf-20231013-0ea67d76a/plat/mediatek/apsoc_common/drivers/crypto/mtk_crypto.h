#ifndef __MTK_CRYPTO_H__
#define __MTK_CRYPTO_H__

#include <lib/mmio.h>

#define MTK_CRYPTO_SUCC			0
#define MTK_CRYPTO_ERR_INVAL		1
#define MTK_CRYPTO_ERR_UNK_OPER		2

uint32_t sej_encrypt(uint8_t *in, uint8_t *out, unsigned int len);
uint32_t sej_decrypt(uint8_t *in, uint8_t *out, unsigned int len);

#endif /* __MTK_CRYPTO_H__ */
