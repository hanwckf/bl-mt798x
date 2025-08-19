#
# Copyright (C) 2022, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

I2C_DEFAULT_SPEED	:=	10000

# I2C GPIO
I2C_GPIO		:=	1
I2C_GPIO_SDA_PIN	:=	4
I2C_GPIO_SCL_PIN	:=	3

# I2C Devices
I2C_DEVICE_RT5190	:=	1
I2C_DEVICE_DS3231	:=	1

MTK_I2C_SOURCES	:=	$(APSOC_COMMON)/drivers/i2c/mt_i2c.c

ifeq ($(I2C_GPIO), 1)
MTK_I2C_SOURCES	+=	$(APSOC_COMMON)/drivers/i2c/i2c_gpio.c
endif

ifeq ($(I2C_DEVICE_RT5190), 1)
MTK_I2C_SOURCES	+=	$(APSOC_COMMON)/drivers/i2c/rt5190.c
endif

ifeq ($(I2C_DEVICE_DS3231), 1)
MTK_I2C_SOURCES	+=	$(APSOC_COMMON)/drivers/i2c/ds3231.c
endif

PLAT_INCLUDES	+=	-I$(APSOC_COMMON)/drivers/i2c/
BL2_SOURCES	+=	${MTK_I2C_SOURCES}
CPPFLAGS	+=      -DI2C_SUPPORT					\
			-DI2C_GPIO_SDA_PIN=${I2C_GPIO_SDA_PIN}		\
			-DI2C_GPIO_SCL_PIN=${I2C_GPIO_SCL_PIN}		\
			-DI2C_DEFAULT_SPEED=${I2C_DEFAULT_SPEED}
