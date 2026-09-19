/****************************************************************************
 * app/gpio_app/gpio_app_main.c
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

/* NSH `gpio_app` 命令：读写任意 STM32 引脚电平，并可配置输入/输出模式。
 *
 * 用法：
 *   gpio_app mode <port><pin> <in|out> [pullup|pulldown]
 *      配置引脚方向。例：gpio_app mode PG10 out
 *   gpio_app read <port><pin>
 *      读取引脚电平（0/1）。例：gpio_app read PG10
 *   gpio_app write <port><pin> <0|1>
 *      写入引脚电平（仅对输出引脚有效）。例：gpio_app write PG10 1
 *   gpio_app help
 *      显示帮助
 *
 * 引脚命名格式：端口字母 + 引脚号，如 PA0、PG10、PE14。
 * 支持端口 A–H（STM32N6 实际可用端口）。
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "stm32_gpio.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("USAGE:\n");
  printf("  %s mode <port><pin> <in|out> [pullup|pulldown]\n", progname);
  printf("      Configure pin direction.\n");
  printf("      Example: %s mode PG10 out\n", progname);
  printf("  %s read <port><pin>\n", progname);
  printf("      Read pin level (0 or 1).\n");
  printf("      Example: %s read PG10\n", progname);
  printf("  %s write <port><pin> <0|1>\n", progname);
  printf("      Write pin level (output pins only).\n");
  printf("      Example: %s write PG10 1\n", progname);
  printf("  %s help\n", progname);
  printf("\nPin naming: <port letter A-H><pin number 0-15>\n");
  printf("Examples: PA0, PG10, PE14, PH7\n");
}

/* 解析引脚名，如 "PG10" -> port='G', pin=10
 * 返回 GPIO_PORTx | GPIO_PINy 宏值（不含方向等属性）。
 * 失败返回 -1。*/

static int parse_pin(FAR const char *s, FAR uint32_t *portbits,
                     FAR uint32_t *pinbits)
{
  char port;
  int pin;
  uint32_t p;
  uint32_t pb;

  if (s == NULL || s[0] == '\0' || s[1] == '\0')
    {
      return -1;
    }

  port = toupper((unsigned char)s[0]);
  if (port < 'A' || port > 'H')
    {
      printf("ERROR: Invalid port '%c' (must be A-H)\n", s[0]);
      return -1;
    }

  /* 解析引脚号 */
  if (sscanf(s + 1, "%d", &pin) != 1 || pin < 0 || pin > 15)
    {
      printf("ERROR: Invalid pin number in '%s' (must be 0-15)\n", s);
      return -1;
    }

  /* 根据端口字母映射到 GPIO_PORTx 宏 */
  switch (port)
    {
      case 'A': p = GPIO_PORTA; break;
      case 'B': p = GPIO_PORTB; break;
      case 'C': p = GPIO_PORTC; break;
      case 'D': p = GPIO_PORTD; break;
      case 'E': p = GPIO_PORTE; break;
      case 'F': p = GPIO_PORTF; break;
      case 'G': p = GPIO_PORTG; break;
      case 'H': p = GPIO_PORTH; break;
      default:
        printf("ERROR: Unsupported port '%c'\n", port);
        return -1;
    }

  /* GPIO_PINn 宏 */
  switch (pin)
    {
      case 0:  pb = GPIO_PIN0;  break;
      case 1:  pb = GPIO_PIN1;  break;
      case 2:  pb = GPIO_PIN2;  break;
      case 3:  pb = GPIO_PIN3;  break;
      case 4:  pb = GPIO_PIN4;  break;
      case 5:  pb = GPIO_PIN5;  break;
      case 6:  pb = GPIO_PIN6;  break;
      case 7:  pb = GPIO_PIN7;  break;
      case 8:  pb = GPIO_PIN8;  break;
      case 9:  pb = GPIO_PIN9;  break;
      case 10: pb = GPIO_PIN10; break;
      case 11: pb = GPIO_PIN11; break;
      case 12: pb = GPIO_PIN12; break;
      case 13: pb = GPIO_PIN13; break;
      case 14: pb = GPIO_PIN14; break;
      case 15: pb = GPIO_PIN15; break;
      default: return -1;
    }

  *portbits = p;
  *pinbits  = pb;
  return OK;
}

/* 根据方向字符串构造 GPIO 配置字 */
static uint32_t build_cfg(uint32_t port, uint32_t pin,
                          FAR const char *mode,
                          FAR const char *pull)
{
  uint32_t cfg;

  if (strcmp(mode, "in") == 0)
    {
      cfg = GPIO_INPUT | GPIO_FLOAT | port | pin;
    }
  else if (strcmp(mode, "out") == 0)
    {
      cfg = GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ |
            GPIO_OUTPUT_CLEAR | port | pin;
    }
  else
    {
      printf("ERROR: Invalid mode '%s' (use 'in' or 'out')\n", mode);
      return 0;
    }

  if (pull != NULL)
    {
      /* 清除默认的上下拉位，再设置对应上下拉 */
      cfg &= ~(GPIO_PULLUP | GPIO_PULLDOWN | GPIO_FLOAT);
      if (strcmp(pull, "pullup") == 0)
        {
          cfg |= GPIO_PULLUP;
        }
      else if (strcmp(pull, "pulldown") == 0)
        {
          cfg |= GPIO_PULLDOWN;
        }
      else
        {
          printf("WARNING: Unknown pull '%s', ignored\n", pull);
        }
    }

  return cfg;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  uint32_t port;
  uint32_t pin;
  uint32_t cfg;
  bool value;

  if (argc < 2)
    {
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "-h") == 0)
    {
      show_usage(argv[0]);
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "mode") == 0)
    {
      if (argc < 4)
        {
          printf("ERROR: 'mode' requires <port><pin> <in|out> [pullup|pulldown]\n");
          return EXIT_FAILURE;
        }

      if (parse_pin(argv[2], &port, &pin) < 0)
        {
          return EXIT_FAILURE;
        }

      cfg = build_cfg(port, pin, argv[3], argc > 4 ? argv[4] : NULL);
      if (cfg == 0)
        {
          return EXIT_FAILURE;
        }

      stm32_configgpio(cfg);
      printf("Configured %s as %s%s\n", argv[2], argv[3],
             argc > 4 ? argv[4] : "");
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "read") == 0)
    {
      if (argc < 3)
        {
          printf("ERROR: 'read' requires <port><pin>\n");
          return EXIT_FAILURE;
        }

      if (parse_pin(argv[2], &port, &pin) < 0)
        {
          return EXIT_FAILURE;
        }

      /* 读模式：配置为输入后读取电平 */
      cfg = GPIO_INPUT | GPIO_FLOAT | port | pin;
      stm32_configgpio(cfg);
      value = stm32_gpioread(port | pin);
      printf("%s = %d\n", argv[2], value ? 1 : 0);
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "write") == 0)
    {
      if (argc < 4)
        {
          printf("ERROR: 'write' requires <port><pin> <0|1>\n");
          return EXIT_FAILURE;
        }

      if (parse_pin(argv[2], &port, &pin) < 0)
        {
          return EXIT_FAILURE;
        }

      if (strcmp(argv[3], "0") == 0)
        {
          value = false;
        }
      else if (strcmp(argv[3], "1") == 0)
        {
          value = true;
        }
      else
        {
          printf("ERROR: Invalid value '%s' (use 0 or 1)\n", argv[3]);
          return EXIT_FAILURE;
        }

      /* 写模式：确保为输出 */
      cfg = GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ |
            GPIO_OUTPUT_CLEAR | port | pin;
      stm32_configgpio(cfg);
      stm32_gpiowrite(port | pin, value);
      printf("%s <- %d\n", argv[2], value ? 1 : 0);
      return EXIT_SUCCESS;
    }

  printf("ERROR: Unknown command '%s'\n", argv[1]);
  show_usage(argv[0]);
  return EXIT_FAILURE;
}
