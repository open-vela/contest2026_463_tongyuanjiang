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
#define QMI8658A_WHO_AM_I    0x00
#define QMI8658A_CTRL2       0x03  /* Accelerometer config */
#define QMI8658A_CTRL3       0x04  /* Gyroscope config */
#define QMI8658A_CTRL7       0x08  /* Sensor enable */
#define QMI8658A_AX_L        0x35  /* Accel X low */
#define QMI8658A_AX_H        0x36
#define QMI8658A_AY_L        0x37
#define QMI8658A_AY_H        0x38
#define QMI8658A_AZ_L        0x39
#define QMI8658A_AZ_H        0x3a
#define QMI8658A_GX_L        0x3b  /* Gyro X low */
#define QMI8658A_GX_H        0x3c
#define QMI8658A_GY_L        0x3d
#define QMI8658A_GY_H        0x3e
#define QMI8658A_GZ_L        0x3f
#define QMI8658A_GZ_H        0x40

#define QMI8658A_CHIP_ID     0x05
#define I2C_FREQ             400000
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

/* Read a single byte from a register (uses repeated-start) */
static int qmi_read_byte(int fd, uint8_t reg, uint8_t *val)
{
  struct i2c_msg_s msgs[2];

  msgs[0].addr      = QMI8658A_ADDR;
  msgs[0].flags     = I2C_M_NOSTOP;
  msgs[0].buffer    = &reg;
  msgs[0].length    = 1;
  msgs[0].frequency = I2C_FREQ;

  msgs[1].addr      = QMI8658A_ADDR;
  msgs[1].flags     = I2C_M_READ;
  msgs[1].buffer    = val;
  msgs[1].length    = 1;
  msgs[1].frequency = I2C_FREQ;

  struct i2c_transfer_s xfer;
  xfer.msgv = msgs;
  xfer.msgc = 2;

  return ioctl(fd, I2CIOC_TRANSFER, (unsigned long)(uintptr_t)&xfer);
}

/* Read a 16-bit value from a register pair (little-endian) */
static int qmi_read_reg16(int fd, uint8_t reg_l, uint8_t reg_h, int16_t *val)
{
  uint8_t lo, hi;
  int ret;

  ret = qmi_read_byte(fd, reg_l, &lo);
  if (ret < 0)
    return ret;

  ret = qmi_read_byte(fd, reg_h, &hi);
  if (ret < 0)
    return ret;

  *val = (int16_t)((hi << 8) | lo);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int fd;
  uint8_t id;
  int ret;
  int i;

  fd = open("/dev/i2c0", O_RDWR);
  if (fd < 0)
    {
      printf("ERROR: Failed to open /dev/i2c0\n");
      return EXIT_FAILURE;
    }

  /* Verify device identity */
  ret = qmi_read_byte(fd, QMI8658A_WHO_AM_I, &id);
  if (ret < 0)
    {
      printf("ERROR: I2C read failed\n");
      close(fd);
      return EXIT_FAILURE;
    }

  printf("QMI8658A WHO_AM_I: 0x%02x", id);
  if (id == QMI8658A_CHIP_ID)
    printf(" (OK)\n");
  else
    printf(" (WARNING: expected 0x%02x)\n", QMI8658A_CHIP_ID);

  /* Initialize QMI8658A */
  printf("Initializing QMI8658A...\n");

  qmi_write_reg(fd, QMI8658A_CTRL7, 0x00);  /* disable sensors */
  usleep(10000);

  /* CTRL2: accel ODR=500Hz, range=±8g */
  qmi_write_reg(fd, QMI8658A_CTRL2, 0x48);

  /* CTRL3: gyro ODR=500Hz, range=±512dps */
  qmi_write_reg(fd, QMI8658A_CTRL3, 0x48);

  /* CTRL7: enable accel + gyro */
  qmi_write_reg(fd, QMI8658A_CTRL7, 0x03);
  usleep(100000);

  /* Read and display raw data */
  printf("\nReading 6-axis raw data (%d samples, 500ms interval):\n\n",
         SAMPLE_COUNT);
  printf("  AX       AY       AZ     |   GX       GY       GZ\n");
  printf("--------  ------   ------  |  ------   ------   ------\n");

  for (i = 0; i < SAMPLE_COUNT; i++)
    {
      int16_t ax, ay, az, gx, gy, gz;

      qmi_read_reg16(fd, QMI8658A_AX_L, QMI8658A_AX_H, &ax);
      qmi_read_reg16(fd, QMI8658A_AY_L, QMI8658A_AY_H, &ay);
      qmi_read_reg16(fd, QMI8658A_AZ_L, QMI8658A_AZ_H, &az);
      qmi_read_reg16(fd, QMI8658A_GX_L, QMI8658A_GX_H, &gx);
      qmi_read_reg16(fd, QMI8658A_GY_L, QMI8658A_GY_H, &gy);
      qmi_read_reg16(fd, QMI8658A_GZ_L, QMI8658A_GZ_H, &gz);

      printf("%8d  %8d  %8d  | %8d  %8d  %8d\n",
             ax, ay, az, gx, gy, gz);
      fflush(stdout);
      usleep(500000);
    }

  printf("\nDone.\n");
  printf("Raw accel: ±8g range, 4096 LSB/g (1g ≈ 4096)\n");
  printf("Raw gyro:  ±512dps range, 64 LSB/dps (1dps ≈ 64)\n");

  /* Disable sensors and cleanup */
  qmi_write_reg(fd, QMI8658A_CTRL7, 0x00);
  close(fd);
  return 0;
}
