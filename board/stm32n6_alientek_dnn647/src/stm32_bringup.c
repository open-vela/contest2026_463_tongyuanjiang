/****************************************************************************
 * boards/arm/stm32n6/nucleo-n657x0-q/src/stm32_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <syslog.h>
#include <debug.h>

#include <nuttx/board.h>
#include <nuttx/leds/userled.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/spi/spi.h>
#include <nuttx/spi/spi_bitbang.h>

#include "nucleo-n657x0-q.h"

#include <arch/board/board.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 ****************************************************************************/

int stm32_bringup(void)
{
#if !defined(CONFIG_ARCH_LEDS) && defined(CONFIG_USERLED_LOWER)
  int ret;

  /* Register the LED driver */

  ret = userled_lower_initialize("/dev/userleds");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: userled_lower_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_I2C_BITBANG
  FAR struct i2c_master_s *i2c;
  int ret;

  /* Initialize I2C2 bitbang driver and register /dev/i2c0 */

  i2c = stm32_i2c_bitbang_initialize();
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "ERROR: stm32_i2c_bitbang_initialize() failed\n");
    }
  else
    {
      ret = i2c_register(i2c, 0);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: i2c_register() failed: %d\n", ret);
        }
      else
        {
          syslog(LOG_INFO, "I2C0 registered (bitbang, PE13=SCL, PE14=SDA)\n");
        }
    }
#endif


#ifdef CONFIG_SPI_BITBANG
  {
    FAR struct spi_dev_s *spi_dev;
    spi_dev = stm32_spi_bitbang_initialize();
    if (spi_dev == NULL)
      {
        syslog(LOG_ERR, "SPI0 init failed\n");
      }
    else
      {
        spi_register(spi_dev, 0);
        syslog(LOG_INFO, "SPI0 registered (bitbang, PE15=SCK, PH7=MOSI, PH8=MISO, PH6=CS)\n");
      }
  }
#endif

  return OK;
}
