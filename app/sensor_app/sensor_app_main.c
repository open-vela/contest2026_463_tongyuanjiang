/****************************************************************************
 * Contest 2026 team 463 - QMI8658A 6-axis IMU sensor demo
 * Reads accelerometer and gyroscope raw data from QMI8658A via I2C.
 * No attitude calculation, just raw register values.
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <nuttx/i2c/i2c_master.h>

/* QMI8658A I2C 7-bit address (SA0=0 on DNN647 board) */
#define QMI8658A_ADDR        0x6a

/* Register addresses */
#define QMI8658A_WHO_AM_I    0x00  /* Chip ID, expect 0x05 */
#define QMI8658A_CTRL1       0x02
#define QMI8658A_CTRL2       0x03  /* Accelerometer config */
#define QMI8658A_CTRL3       0x04  /* Gyroscope config */
#define QMI8658A_CTRL7       0x08  /* Sensor enable */
#define QMI8658A_AX_L        0x35  /* Accel X low (start of 12-byte block) */

/* Expected chip ID */
#define QMI8658A_CHIP_ID     0x05

/* I2C frequency 400kHz */
#define I2C_FREQ             400000

/* Number of samples to read */
#define SAMPLE_COUNT         20

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Write a single byte to a register */
static int qmi_write_reg(int fd, uint8_t reg, uint8_t val)
{
  uint8_t buf[2];
  buf[0] = reg;
  buf[1] = val;

  struct i2c_msg_s msg;
  msg.addr      = QMI8658A_ADDR;
  msg.flags     = 0;
  msg.buffer    = buf;
  msg.length    = 2;
  msg.frequency = I2C_FREQ;

  struct i2c_transfer_s xfer;
  xfer.msgv = &msg;
  xfer.msgc = 1;

  return ioctl(fd, I2CIOC_TRANSFER, (unsigned long)(uintptr_t)&xfer);
}

/* Read N bytes from a register (write reg addr, repeated start, read data) */
static int qmi_read_reg(int fd, uint8_t reg, uint8_t *buf, int len)
{
  struct i2c_msg_s msgs[2];

  /* Write register address */
  msgs[0].addr      = QMI8658A_ADDR;
  msgs[0].flags     = 0;
  msgs[0].buffer    = &reg;
  msgs[0].length    = 1;
  msgs[0].frequency = I2C_FREQ;

  /* Read data */
  msgs[1].addr      = QMI8658A_ADDR;
  msgs[1].flags     = I2C_M_READ;
  msgs[1].buffer    = buf;
  msgs[1].length    = len;
  msgs[1].frequency = I2C_FREQ;

  struct i2c_transfer_s xfer;
  xfer.msgv = msgs;
  xfer.msgc = 2;

  return ioctl(fd, I2CIOC_TRANSFER, (unsigned long)(uintptr_t)&xfer);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int fd;
  uint8_t id;
  uint8_t data[12];
  int ret;
  int i;

  /* Open I2C device */
  fd = open("/dev/i2c0", O_RDWR);
  if (fd < 0)
    {
      printf("ERROR: Failed to open /dev/i2c0\n");
      return EXIT_FAILURE;
    }

  /* Read WHO_AM_I to verify device */
  ret = qmi_read_reg(fd, QMI8658A_WHO_AM_I, &id, 1);
  if (ret < 0)
    {
      printf("ERROR: I2C transfer failed (WHO_AM_I)\n");
      close(fd);
      return EXIT_FAILURE;
    }

  printf("QMI8658A WHO_AM_I: 0x%02x", id);
  if (id == QMI8658A_CHIP_ID)
    {
      printf(" (OK)\n");
    }
  else
    {
      printf(" (WARNING: expected 0x%02x)\n", QMI8658A_CHIP_ID);
    }

  /* Initialize QMI8658A */
  printf("Initializing QMI8658A...\n");

  /* Disable sensors during config */
  qmi_write_reg(fd, QMI8658A_CTRL7, 0x00);
  usleep(10000);

  /* CTRL2: accel ODR=500Hz (0x4<<4), range=±8g (0x2<<2) => 0x48 */
  qmi_write_reg(fd, QMI8658A_CTRL2, 0x48);

  /* CTRL3: gyro ODR=500Hz (0x4<<4), range=±512dps (0x2<<2) => 0x48 */
  qmi_write_reg(fd, QMI8658A_CTRL3, 0x48);

  /* CTRL7: enable accel (bit0) + gyro (bit1) */
  qmi_write_reg(fd, QMI8658A_CTRL7, 0x03);
  usleep(50000);

  /* Read and display raw data */
  printf("\nReading 6-axis raw data (%d samples, 500ms interval):\n\n",
         SAMPLE_COUNT);
  printf("  AX       AY       AZ     |   GX       GY       GZ\n");
  printf("--------  ------   ------  |  ------   ------   ------\n");

  for (i = 0; i < SAMPLE_COUNT; i++)
    {
      /* Read 12 bytes: AX_L(0x35) .. GZ_H(0x40) */
      ret = qmi_read_reg(fd, QMI8658A_AX_L, data, 12);
      if (ret < 0)
        {
          printf("ERROR: Failed to read sensor data (sample %d)\n", i);
          break;
        }

      /* Parse raw int16 values (little-endian: low byte first) */
      int16_t ax = (int16_t)((data[1]  << 8) | data[0]);
      int16_t ay = (int16_t)((data[3]  << 8) | data[2]);
      int16_t az = (int16_t)((data[5]  << 8) | data[4]);
      int16_t gx = (int16_t)((data[7]  << 8) | data[6]);
      int16_t gy = (int16_t)((data[9]  << 8) | data[8]);
      int16_t gz = (int16_t)((data[11] << 8) | data[10]);

      printf("%8d  %8d  %8d  | %8d  %8d  %8d\n",
             ax, ay, az, gx, gy, gz);
      fflush(stdout);
      usleep(500000);
    }

  printf("\nDone.\n");
  printf("Raw accel: ±8g range, 4096 LSB/g (1g ≈ 4096)\n");
  printf("Raw gyro:  ±512dps range, 64 LSB/dps (1dps ≈ 64)\n");

  /* Disable sensors */
  qmi_write_reg(fd, QMI8658A_CTRL7, 0x00);
  close(fd);
  return 0;
}
