#ifndef IMU__H__
#define IMU__H__

#include <xc.h>
#include "i2c_master_noint.h"

#define IMU_WHOAMI 0x0F
#define IMU_W_ADDR 0b11010110
#define IMU_R_ADDR 0b11010111
#define IMU_CTRL1_XL 0x10
#define IMU_CTRL2_G 0x11
#define IMU_CTRL3_C 0x12
#define IMU_OUT_TEMP_L 0x20

void init_LSM6DS33();
void imu_read(unsigned char reg, signed short * data, int len);

#endif