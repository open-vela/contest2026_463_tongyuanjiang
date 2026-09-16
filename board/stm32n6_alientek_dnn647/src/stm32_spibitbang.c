/****************************************************************************
 * boards/arm/stm32n6/stm32n6_alientek_dnn647/src/stm32_spibitbang.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI bitbang lower-half driver for DNN647 board.
 * Uses GPIO to bitbang SPI5 on:
 *   PE15 = SCK, PH7 = MOSI, PH8 = MISO, PH6 = CS
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>
#include <nuttx/spi/spi_bitbang.h>

#include "stm32_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SPI5 pin definitions for DNN647 WIRELESS interface:
 * PE15 = SPI5_SCK
 * PH7  = SPI5_MOSI
 * PH8  = SPI5_MISO
 * PH6  = NRF_CS (chip select, active low)
 */

#define GPIO_SPI_SCK   (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ | \
                        GPIO_OUTPUT_CLEAR | GPIO_PORTE | GPIO_PIN15)

#define GPIO_SPI_MOSI  (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ | \
                        GPIO_OUTPUT_CLEAR | GPIO_PORTH | GPIO_PIN7)

#define GPIO_SPI_MISO  (GPIO_INPUT | GPIO_PULLUP | GPIO_PORTH | GPIO_PIN8)

#define GPIO_SPI_CS    (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ | \
                        GPIO_OUTPUT_SET | GPIO_PORTH | GPIO_PIN6)

/* Default SPI clock delay (microseconds) for ~500kHz */

#define SPI_DEFAULT_HOLDTIME  1

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct stm32_spibb_priv_s
{
  uint32_t frequency;
  uint32_t holdtime;
  uint8_t  mode;
};

static struct stm32_spibb_priv_s g_spibb_priv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_spibb_select(FAR struct spi_bitbang_s *priv,
                               uint32_t devid, bool selected)
{
  /* CS is active low */
  stm32_gpiowrite(GPIO_SPI_CS, !selected);
}

static uint32_t stm32_spibb_setfrequency(FAR struct spi_bitbang_s *priv,
                                         uint32_t frequency)
{
  uint32_t old_freq = g_spibb_priv.frequency;

  if (frequency == 0)
    {
      frequency = 500000;
    }

  g_spibb_priv.frequency = frequency;
  /* holdtime in us = 1000000 / (2 * frequency) */
  g_spibb_priv.holdtime = 1000000 / (2 * frequency);
  if (g_spibb_priv.holdtime < 1)
    {
      g_spibb_priv.holdtime = 1;
    }

  return old_freq;
}

static void stm32_spibb_setmode(FAR struct spi_bitbang_s *priv,
                                enum spi_mode_e mode)
{
  g_spibb_priv.mode = mode;
}

static uint16_t stm32_spibb_exchange(FAR struct spi_bitbang_s *priv,
                                     uint16_t dataout)
{
  uint16_t datain = 0;
  uint16_t bit;
  uint8_t cpol;
  uint8_t cpha;

  /* Extract CPOL and CPHA from mode */
  cpol = (g_spibb_priv.mode == SPIDEV_MODE2 || g_spibb_priv.mode == SPIDEV_MODE3) ? 1 : 0;
  cpha = (g_spibb_priv.mode == SPIDEV_MODE1 || g_spibb_priv.mode == SPIDEV_MODE3) ? 1 : 0;

  /* Bit-bang 8 bits, MSB first */
  for (bit = 0; bit < 8; bit++)
    {
      /* Set MOSI */
      stm32_gpiowrite(GPIO_SPI_MOSI, (dataout & (1 << (7 - bit))) != 0);

      /* Phase handling */
      if (cpha == 0)
        {
          /* CPHA=0: data sampled on first edge */
          stm32_gpiowrite(GPIO_SPI_SCK, !cpol);
          up_udelay(g_spibb_priv.holdtime);
          if (stm32_gpioread(GPIO_SPI_MISO))
            {
              datain |= (1 << (7 - bit));
            }
          stm32_gpiowrite(GPIO_SPI_SCK, cpol);
          up_udelay(g_spibb_priv.holdtime);
        }
      else
        {
          /* CPHA=1: data sampled on second edge */
          stm32_gpiowrite(GPIO_SPI_SCK, !cpol);
          up_udelay(g_spibb_priv.holdtime);
          stm32_gpiowrite(GPIO_SPI_SCK, cpol);
          up_udelay(g_spibb_priv.holdtime);
          if (stm32_gpioread(GPIO_SPI_MISO))
            {
              datain |= (1 << (7 - bit));
            }
        }
    }

  /* Return SCK to idle state */
  stm32_gpiowrite(GPIO_SPI_SCK, cpol);

  return datain;
}

static uint8_t stm32_spibb_status(FAR struct spi_bitbang_s *priv,
                                  uint32_t devid)
{
  return 0;
}

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct spi_bitbang_ops_s g_spi_bb_ops =
{
  .select      = stm32_spibb_select,
  .setfrequency = stm32_spibb_setfrequency,
  .setmode     = stm32_spibb_setmode,
  .exchange    = stm32_spibb_exchange,
  .status      = stm32_spibb_status,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR struct spi_dev_s *stm32_spi_bitbang_initialize(void)
{
  /* Initialize GPIO pins */
  stm32_configgpio(GPIO_SPI_SCK);
  stm32_configgpio(GPIO_SPI_MOSI);
  stm32_configgpio(GPIO_SPI_MISO);
  stm32_configgpio(GPIO_SPI_CS);

  /* Set initial state: SCK low (mode 0), CS high (deselected) */
  stm32_gpiowrite(GPIO_SPI_SCK, false);
  stm32_gpiowrite(GPIO_SPI_MOSI, false);
  stm32_gpiowrite(GPIO_SPI_CS, true);

  g_spibb_priv.frequency = 500000;
  g_spibb_priv.holdtime = SPI_DEFAULT_HOLDTIME;
  g_spibb_priv.mode = SPIDEV_MODE0;

  return spi_create_bitbang(&g_spi_bb_ops, &g_spibb_priv);
}
