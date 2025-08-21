/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef _MTK_ETH_PHY_INTERNAL_H_
#define _MTK_ETH_PHY_INTERNAL_H_

#include <stdint.h>
#include "phy.h"
#include "compat.h"

/* generic phy API */
void gen_phy_get_status(uint8_t phy, struct genphy_status *status);
int gen_phy_wait_ready(uint8_t phy, uint32_t timeout_ms);

#endif /* _MTK_ETH_PHY_INTERNAL_H_ */
