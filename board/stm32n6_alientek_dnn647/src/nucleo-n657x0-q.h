/****************************************************************************
 * boards/arm/stm32n6/nucleo-n657x0-q/src/nucleo-n657x0-q.h
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

#ifndef __BOARDS_ARM_STM32N6_NUCLEO_N657X0_Q_SRC_NUCLEO_N657X0_Q_H
#define __BOARDS_ARM_STM32N6_NUCLEO_N657X0_Q_SRC_NUCLEO_N657X0_Q_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <stdint.h>

#include "stm32_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* LED definitions **********************************************************/

/* The Alientek DNN647 has two user LEDs:
 *
 *   LED1  PG10  Red
 *   LED2  PE10  Green
 *
 * - When the I/O is LOW,  the LED is on.
 * - When the I/O is HIGH, the LED is off.
 */

#define GPIO_LED1      (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ |                         GPIO_OUTPUT_SET | GPIO_PORTG | GPIO_PIN10)
#define GPIO_LED2      (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ |                         GPIO_OUTPUT_SET | GPIO_PORTE | GPIO_PIN10)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32_bringup(void);

#ifdef CONFIG_I2C_BITBANG
struct i2c_master_s;
FAR struct i2c_master_s *stm32_i2c_bitbang_initialize(void);
#endif

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_STM32N6_NUCLEO_N657X0_Q_SRC_NUCLEO_N657X0_Q_H */

 FAR struct spi_dev_s *stm32_spi_bitbang_initialize(void);
