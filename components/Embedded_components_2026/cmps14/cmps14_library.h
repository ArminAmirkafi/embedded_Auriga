#ifndef CMPS14_LIBRARY_H
#define CMPS14_LIBRARY_H

#include "i2c_library.h"

#define CMPS14_I2C_ADDR                   0x60

#define CMPS14_REG_COMMAND                0x00
#define CMPS14_REG_BEARING_8BIT           0x01
#define CMPS14_REG_BEARING_16BIT_HIGH     0x02

#define CMPS14_REG_PITCH_8BIT             0x04
#define CMPS14_REG_ROLL_8BIT              0x05

#define CMPS14_REG_MAG_X_16BIT_HIGH       0x06
#define CMPS14_REG_MAG_Y_16BIT_HIGH       0x08
#define CMPS14_REG_MAG_Z_16BIT_HIGH       0x0A

#define CMPS14_REG_LIN_ACCEL_X_16BIT_HIGH 0x0C
#define CMPS14_REG_LIN_ACCEL_Y_16BIT_HIGH 0x0E
#define CMPS14_REG_LIN_ACCEL_Z_16BIT_HIGH 0x10

#define CMPS14_REG_GYRO_X_16BIT_HIGH      0x12
#define CMPS14_REG_GYRO_Y_16BIT_HIGH      0x14
#define CMPS14_REG_GYRO_Z_16BIT_HIGH      0x16

#define CMPS14_REG_PITCH_16BIT_HIGH       0x1A
#define CMPS14_REG_ROLL_16BIT_HIGH        0x1C

#define CMPS14_REG_CALIB_STATUS           0x1E

#define CMPS14_REG_ACCEL_X_16BIT_HIGH     0x1F
#define CMPS14_REG_ACCEL_Y_16BIT_HIGH     0x21
#define CMPS14_REG_ACCEL_Z_16BIT_HIGH     0x23

uint16_t cmps14_read_bearing_16bit(i2c_bus_t *bus);
uint8_t  cmps14_read_bearing_8bit(i2c_bus_t *bus);
int16_t  cmps14_read_pitch_16bit(i2c_bus_t *bus);
int8_t   cmps14_read_pitch_8bit(i2c_bus_t *bus);
int16_t  cmps14_read_roll_16bit(i2c_bus_t *bus);
int8_t   cmps14_read_roll_8bit(i2c_bus_t *bus);

int16_t  cmps14_read_mag_x_16bit(i2c_bus_t *bus);
int16_t  cmps14_read_mag_y_16bit(i2c_bus_t *bus);
int16_t  cmps14_read_mag_z_16bit(i2c_bus_t *bus);

int16_t  cmps14_read_lin_accel_x_16bit(i2c_bus_t *bus);
int16_t  cmps14_read_lin_accel_y_16bit(i2c_bus_t *bus);
int16_t  cmps14_read_lin_accel_z_16bit(i2c_bus_t *bus);

int16_t  cmps14_read_gyro_x_16bit(i2c_bus_t *bus);
int16_t  cmps14_read_gyro_y_16bit(i2c_bus_t *bus);
int16_t  cmps14_read_gyro_z_16bit(i2c_bus_t *bus);

int16_t  cmps14_read_accel_x_16bit(i2c_bus_t *bus);
int16_t  cmps14_read_accel_y_16bit(i2c_bus_t *bus);
int16_t  cmps14_read_accel_z_16bit(i2c_bus_t *bus);

void cmps14_enable_auto_calibration(i2c_bus_t *bus);
void cmps14_disable_auto_calibration(i2c_bus_t *bus);
bool cmps14_is_auto_calibration_enabled(i2c_bus_t *bus);

#endif