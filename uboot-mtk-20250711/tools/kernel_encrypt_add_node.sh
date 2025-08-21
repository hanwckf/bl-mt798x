## SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2022 MediaTek Incorporation. All Rights Reserved.
#
# Author: guan-gm.lin <guan-gm.lin@mediatek.com>
#

fdtput $1 /cipher/key-$2-$3 -cp
fdtput $1 /cipher/key-$2-$3 -tx key-len 0x20
fdtput $1 /cipher/key-$2-$3 -tx key 0x0
