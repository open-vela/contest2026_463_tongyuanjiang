/****************************************************************************
 * boards/arm/stm32n6/stm32n6_alientek_dnn647/src/stm32_userleds.c
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

/* 本文件在 CONFIG_ARCH_LEDS=y 时也参与编译，这样 /dev/userleds
 * 设备节点与 OS 状态自动指示 LED 可同时工作：
 *   - stm32_autoleds.c  负责将 OS 事件（IDLE/IRQ/PANIC…）映射到 LED
 *   - stm32_userleds.c  负责通过标准 userled_lower 半层驱动暴露
 *                       /dev/userleds，供 NSH `leds` 命令读写引脚电平
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <debug.h>

#include <sys/param.h>

#include <nuttx/board.h>
#include <arch/board/board.h>

#include "stm32_gpio.h"
#include "stm32n6_alientek_dnn647.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This array maps an LED number to GPIO pin configuration and is indexed by
 * BOARD_LED_<color>
 */

static const uint32_t g_ledcfg[BOARD_NLEDS] =
{
  GPIO_LED1,
  GPIO_LED2,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_userled_initialize
 *
 * Description:
 *   初始化板载 LED 对应的 GPIO 为输出。即使 CONFIG_ARCH_LEDS=y 也提供，
 *   以便 userled_lower 驱动可以注册 /dev/userleds 供 shell 访问。
 *
 ****************************************************************************/

uint32_t board_userled_initialize(void)
{
  int i;

  /* Configure LED1 and LED2 GPIOs for output */

  for (i = 0; i < nitems(g_ledcfg); i++)
    {
      stm32_configgpio(g_ledcfg[i]);
    }

  return BOARD_NLEDS;
}

/****************************************************************************
 * Name: board_userled
 *
 * Description:
 *   设置单个 LED 亮灭。Active Low：输出低电平点亮 LED。
 *
 ****************************************************************************/

void board_userled(int led, bool ledon)
{
  if ((unsigned)led < nitems(g_ledcfg))
    {
      /* Active Low */

      stm32_gpiowrite(g_ledcfg[led], !ledon);
    }
}

/****************************************************************************
 * Name: board_userled_all
 *
 * Description:
 *   通过位图一次性设置所有 LED。Active Low：位 1 = 点亮（输出低）。
 *
 ****************************************************************************/

void board_userled_all(uint32_t ledset)
{
  int i;

  /* Configure LED1 and LED2 GPIOs.  Active Low: pin LOW = LED on. */

  for (i = 0; i < nitems(g_ledcfg); i++)
    {
      stm32_gpiowrite(g_ledcfg[i], !(ledset & (1 << i)));
    }
}
