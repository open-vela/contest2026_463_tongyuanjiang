/****************************************************************************
 * boards/arm/stm32n6/stm32n6_alientek_dnn647/src/stm32_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#include "stm32n6_alientek_dnn647.h"

#include <arch/board/board.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 *   板级后期初始化，注册 /dev/userleds、I2C0、SPI0 等设备节点。
 *
 ****************************************************************************/

int stm32_bringup(void)
{
  int ret;

#ifdef CONFIG_USERLED_LOWER
  /* Register the user LED driver -> /dev/userleds */

  ret = userled_lower_initialize("/dev/userleds");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: userled_lower_initialize() failed: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "userleds registered at /dev/userleds\n");
    }
#endif

#ifdef CONFIG_I2C_BITBANG
  {
    FAR struct i2c_master_s *i2c;

    /* Initialize I2C bitbang driver and register /dev/i2c0 */

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
        ret = spi_register(spi_dev, 0);
        if (ret < 0)
          {
            syslog(LOG_ERR, "ERROR: spi_register() failed: %d\n", ret);
          }
        else
          {
            syslog(LOG_INFO, "SPI0 registered (bitbang, PE15=SCK, PH7=MOSI, PH8=MISO, PH6=CS)\n");
          }
      }
  }
#endif

  return OK;
}
