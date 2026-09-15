/****************************************************************************
 * boards/arm/stm32n6/stm32n6_alientek_dnn647/src/stm32_i2cbitbang.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * I2C bitbang lower-half driver for DNN647 board.
 * Uses GPIO to bitbang I2C2 on PD14(SCL) and PD4(SDA).
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/i2c/i2c_bitbang.h>

#include "stm32_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* I2C2 pin definitions for DNN647:
 * PD14 = I2C2_SCL (10K pull-up on board)
 * PD4  = I2C2_SDA (10K pull-up on board)
 */

#define GPIO_I2C2_SCL  (GPIO_OUTPUT | GPIO_OPENDRAIN | GPIO_SPEED_2MHZ |                         GPIO_OUTPUT_SET | GPIO_PORTD | GPIO_PIN14)

#define GPIO_I2C2_SDA  (GPIO_OUTPUT | GPIO_OPENDRAIN | GPIO_SPEED_2MHZ |                         GPIO_OUTPUT_SET | GPIO_PORTD | GPIO_PIN4)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32_i2c_lower_s
{
  struct i2c_bitbang_lower_dev_s lower;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_i2c_initialize(FAR struct i2c_bitbang_lower_dev_s *lower)
{
  stm32_configgpio(GPIO_I2C2_SCL);
  stm32_configgpio(GPIO_I2C2_SDA);
}

static void stm32_i2c_set_scl(FAR struct i2c_bitbang_lower_dev_s *lower,
                              bool high)
{
  stm32_gpiowrite(GPIO_I2C2_SCL, high);
}

static void stm32_i2c_set_sda(FAR struct i2c_bitbang_lower_dev_s *lower,
                              bool high)
{
  stm32_gpiowrite(GPIO_I2C2_SDA, high);
}

static bool stm32_i2c_get_scl(FAR struct i2c_bitbang_lower_dev_s *lower)
{
  return stm32_gpioread(GPIO_I2C2_SCL);
}

static bool stm32_i2c_get_sda(FAR struct i2c_bitbang_lower_dev_s *lower)
{
  return stm32_gpioread(GPIO_I2C2_SDA);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct i2c_bitbang_lower_ops_s g_i2c_lower_ops =
{
  .initialize = stm32_i2c_initialize,
  .set_scl    = stm32_i2c_set_scl,
  .set_sda    = stm32_i2c_set_sda,
  .get_scl    = stm32_i2c_get_scl,
  .get_sda    = stm32_i2c_get_sda,
};

static struct stm32_i2c_lower_s g_i2c_lower =
{
  .lower =
  {
    .ops  = &g_i2c_lower_ops,
    .priv = NULL,
  },
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR struct i2c_master_s *stm32_i2c_bitbang_initialize(void)
{
  return i2c_bitbang_initialize(&g_i2c_lower.lower);
}
