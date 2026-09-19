/****************************************************************************
 * app/spi_app/spi_app_main.c
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

/* NSH `spi_app` 命令：通过 SPI bitbang 驱动读写外设寄存器。
 *
 * 用法：
 *   spi_app probe           探测 SPI 总线上的设备
 *   spi_app read <reg>      读取寄存器（发送 reg，接收 1 字节）
 *   spi_app write <reg> <val>  写入寄存器（发送 reg+val）
 *   spi_app test            数据收发稳定性测试（20 次）
 *   spi_app help            显示帮助
 *
 * SPI5 引脚（DNN647 WIRELESS 接口）：
 *   PE15 = SCK, PH7 = MOSI, PH8 = MISO, PH6 = CS（Active Low）
 *
 * 验证标准：
 *   1. 可成功探测外接设备地址（probe）
 *   2. 寄存器读写正常（read/write）
 *   3. 数据收发稳定，无总线卡死、无通信超时（test 20 次）
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <nuttx/spi/spi.h>
#include <nuttx/spi/spi_transfer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SPI_DEV_PATH  "/dev/spi0"

/* 常用 SPI 命令 */
#define CMD_JEDEC_ID    0x9F   /* JEDEC ID（SPI Flash 标准） */
#define CMD_READ_STATUS 0x05   /* 读状态寄存器 */
#define CMD_NOP         0x00   /* NOP */
#define CMD_NRF_STATUS  0xFF   /* NRF24L01 STATUS */

#define TEST_COUNT      20

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("USAGE:\n");
  printf("  %s probe              Probe SPI bus\n", progname);
  printf("  %s read <reg>         Read register\n", progname);
  printf("  %s write <reg> <val>  Write register\n", progname);
  printf("  %s test               Stability test (%d exchanges)\n",
         progname, TEST_COUNT);
  printf("  %s help               Show help\n", progname);
  printf("\nSPI5 pins: PE15=SCK, PH7=MOSI, PH8=MISO, PH6=CS\n");
}

/* 通过 SPI 总线交换一个字节（全双工） */
static int spi_exchange_byte(int fd, uint8_t tx, FAR uint8_t *rx)
{
  struct spi_trans_s trans;
  struct spi_sequence_s seq;

  memset(&trans, 0, sizeof(trans));
  memset(&seq, 0, sizeof(seq));

  trans.nwords    = 1;
  trans.txbuffer  = &tx;
  trans.rxbuffer  = rx;
  trans.deselect  = false;

  seq.dev        = 0;
  seq.mode       = SPIDEV_MODE0;
  seq.nbits      = 8;
  seq.ntrans     = 1;
  seq.frequency  = 500000;
  seq.trans      = &trans;

  return ioctl(fd, SPIIOC_TRANSFER, (unsigned long)(uintptr_t)&seq);
}

/* 发送命令后连续读取 n 个字节（CS 保持有效） */
static void spi_read_multi(int fd, uint8_t cmd, FAR uint8_t *buf, int n)
{
  int i;
  /* 先发送命令字节 */
  uint8_t dummy;
  spi_exchange_byte(fd, cmd, &dummy);
  /* 连续读 n 个字节 */
  for (i = 0; i < n; i++)
    {
      spi_exchange_byte(fd, 0x00, &buf[i]);
    }
}

/* 探测 SPI 总线设备 */
static int do_probe(int fd)
{
  uint8_t rx;
  int ret;

  printf("=== SPI Bus Probe ===\n");
  printf("Device: %s (SPI5 bitbang)\n", SPI_DEV_PATH);
  printf("Pins: PE15=SCK, PH7=MOSI, PH8=MISO, PH6=CS\n\n");

  /* 测试 1: NOP 响应 */
  printf("[Test 1] Send NOP (0x00), read response...\n");
  ret = spi_exchange_byte(fd, CMD_NOP, &rx);
  if (ret < 0)
    {
      printf("  FAIL: SPI transfer error\n");
      return -1;
    }
  printf("  Response: 0x%02x\n", rx);

  /* 测试 2: JEDEC ID */
  printf("\n[Test 2] Read JEDEC ID (cmd 0x9F)...\n");
  {
    uint8_t id[3] = { 0 };
    spi_read_multi(fd, CMD_JEDEC_ID, id, 3);
    printf("  Manufacturer ID: 0x%02x\n", id[0]);
    printf("  Memory Type:     0x%02x\n", id[1]);
    printf("  Capacity:         0x%02x\n", id[2]);
    if (id[0] != 0xFF && id[0] != 0x00)
      {
        printf("  -> SPI Flash detected!\n");
      }
  }

  /* 测试 3: 状态寄存器 */
  printf("\n[Test 3] Read Status (cmd 0x05)...\n");
  {
    uint8_t status;
    spi_exchange_byte(fd, CMD_READ_STATUS, &status);
    spi_exchange_byte(fd, 0x00, &status);
    printf("  Status register: 0x%02x\n", status);
  }

  /* 测试 4: NRF24L01 STATUS */
  printf("\n[Test 4] NRF STATUS (cmd 0xFF)...\n");
  {
    uint8_t nrf_status;
    spi_exchange_byte(fd, CMD_NRF_STATUS, &nrf_status);
    printf("  NRF STATUS: 0x%02x\n", nrf_status);
    if ((nrf_status & 0x0F) != 0x0F && nrf_status != 0x00 &&
        nrf_status != 0xFF)
      {
        printf("  -> NRF24L01 wireless module detected!\n");
      }
  }

  printf("\nProbe complete.\n");
  return 0;
}

/* 稳定性测试 */
static int do_test(int fd)
{
  uint8_t rx;
  int ret;
  int i;
  int success = 0;

  printf("=== SPI Stability Test ===\n");
  printf("Running %d exchanges...\n\n", TEST_COUNT);

  for (i = 0; i < TEST_COUNT; i++)
    {
      uint8_t tx = (uint8_t)(i + 1);
      ret = spi_exchange_byte(fd, tx, &rx);
      if (ret < 0)
        {
          printf("  [%2d] FAIL: transfer error (tx=0x%02x)\n", i + 1, tx);
        }
      else
        {
          printf("  [%2d] TX=0x%02x  RX=0x%02x\n", i + 1, tx, rx);
          success++;
        }
      usleep(100000); /* 100ms */
    }

  printf("\nResult: %d/%d successful transfers\n", success, TEST_COUNT);
  if (success == TEST_COUNT)
    {
      printf("PASS: No bus deadlock, no timeout, all transfers completed.\n");
    }
  else
    {
      printf("WARNING: %d transfers failed.\n", TEST_COUNT - success);
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;

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

  fd = open(SPI_DEV_PATH, O_RDWR);
  if (fd < 0)
    {
      printf("ERROR: Cannot open %s\n", SPI_DEV_PATH);
      return EXIT_FAILURE;
    }

  if (strcmp(argv[1], "probe") == 0)
    {
      do_probe(fd);
    }
  else if (strcmp(argv[1], "test") == 0)
    {
      do_test(fd);
    }
  else if (strcmp(argv[1], "read") == 0)
    {
      if (argc < 3)
        {
          printf("ERROR: 'read' requires <reg>\n");
          close(fd);
          return EXIT_FAILURE;
        }
      uint8_t reg = (uint8_t)strtol(argv[2], NULL, 0);
      uint8_t rx;
      spi_exchange_byte(fd, reg, &rx);
      printf("Reg 0x%02x: 0x%02x\n", reg, rx);
    }
  else if (strcmp(argv[1], "write") == 0)
    {
      if (argc < 4)
        {
          printf("ERROR: 'write' requires <reg> <val>\n");
          close(fd);
          return EXIT_FAILURE;
        }
      uint8_t reg = (uint8_t)strtol(argv[2], NULL, 0);
      uint8_t val = (uint8_t)strtol(argv[3], NULL, 0);
      uint8_t rx;
      spi_exchange_byte(fd, reg, &rx);
      spi_exchange_byte(fd, val, &rx);
      printf("Wrote 0x%02x to reg 0x%02x (resp: 0x%02x)\n", val, reg, rx);
    }
  else
    {
      printf("ERROR: Unknown command '%s'\n", argv[1]);
      show_usage(argv[0]);
    }

  close(fd);
  return EXIT_SUCCESS;
}
