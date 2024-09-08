/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _FAILSAFE_FW_TYPE_H_
#define _FAILSAFE_FW_TYPE_H_

typedef enum {
	FW_TYPE_GPT,
	FW_TYPE_BL2,
	FW_TYPE_FIP,
	FW_TYPE_FW,
	FW_TYPE_INITRD,
} failsafe_fw_t;

#endif
