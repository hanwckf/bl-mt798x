/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef FW_DEC_H
#define FW_DEC_H

#define FW_DEC_KEY_IDX_ERR		1
#define FW_DEC_INVALID_PARAM_ERR	2
#define FW_DEC_PARAM_NOT_SET_ERR	3
#define FW_DEC_IMAGE_DEC_ERR		4

void fw_dec_init(void);

#ifdef MTK_FW_ENC_VIA_BL31
int fw_dec_set_iv(uintptr_t iv_paddr, uint32_t iv_size);
int fw_dec_set_key(uint32_t key_idx);
int fw_dec_image(uintptr_t image_paddr, uint32_t image_size);
#endif /* MTK_FW_ENC_VIA_BL31 */

#ifdef MTK_FW_ENC_VIA_OPTEE
int fw_dec_get_key(uint32_t key_idx, uintptr_t paddr, uint32_t len);
#endif /* MTK_FW_ENC_VIA_OPTEE */

#endif /* FW_DEC_H */
