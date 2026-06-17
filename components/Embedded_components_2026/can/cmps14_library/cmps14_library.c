#include "cmps14_library.h"


static uint8_t cmps14_get_data(i2c_bus_t *bus, uint8_t reg_addr) {
    uint8_t data = 0;
    i2c_read_bytes(bus, CMPS14_I2C_ADDR, reg_addr, &data, 1);
    return data;
}

static uint16_t cmps14_get_data_16bit(i2c_bus_t *bus, uint8_t reg_addr) {
    uint8_t data[2] = {0};
    i2c_read_bytes(bus, CMPS14_I2C_ADDR, reg_addr, data, 2);
    return (uint16_t)((data[0] << 8) | data[1]);
}

// ================= BEARING (Yaw) =================
uint8_t cmps14_read_bearing_8bit(i2c_bus_t *bus) {
    return cmps14_get_data(bus, CMPS14_REG_BEARING_8BIT);
}

uint16_t cmps14_read_bearing_16bit(i2c_bus_t *bus) {
    return cmps14_get_data_16bit(bus, CMPS14_REG_BEARING_16BIT_HIGH);
}

// ================= PITCH =================
int8_t cmps14_read_pitch_8bit(i2c_bus_t *bus) {
    return (int8_t)cmps14_get_data(bus, CMPS14_REG_PITCH_8BIT);
}

int16_t cmps14_read_pitch_16bit(i2c_bus_t *bus) {
    return (int16_t)cmps14_get_data_16bit(bus, CMPS14_REG_PITCH_16BIT_HIGH);
}

// ================= ROLL =================
int8_t cmps14_read_roll_8bit(i2c_bus_t *bus) {
    return (int8_t)cmps14_get_data(bus, CMPS14_REG_ROLL_8BIT);
}

int16_t cmps14_read_roll_16bit(i2c_bus_t *bus) {
    return (int16_t)cmps14_get_data_16bit(bus, CMPS14_REG_ROLL_16BIT_HIGH);
}

// ================= Magnetometer =================
int16_t cmps14_read_mag_x_16bit(i2c_bus_t *bus) {
    return (int16_t)cmps14_get_data_16bit(bus, CMPS14_REG_MAG_X_16BIT_HIGH);
}

int16_t cmps14_read_mag_y_16bit(i2c_bus_t *bus) {
    return (int16_t)cmps14_get_data_16bit(bus, CMPS14_REG_MAG_Y_16BIT_HIGH);
}

int16_t cmps14_read_mag_z_16bit(i2c_bus_t *bus) {
    return (int16_t)cmps14_get_data_16bit(bus, CMPS14_REG_MAG_Z_16BIT_HIGH);
}

// ================= Accelerometer =================
int16_t cmps14_read_accel_x_16bit(i2c_bus_t *bus) {
    return (int16_t)cmps14_get_data_16bit(bus, CMPS14_REG_ACCEL_X_16BIT_HIGH);
}

int16_t cmps14_read_accel_y_16bit(i2c_bus_t *bus) {
    return (int16_t)cmps14_get_data_16bit(bus, CMPS14_REG_ACCEL_Y_16BIT_HIGH);
}

int16_t cmps14_read_accel_z_16bit(i2c_bus_t *bus) {
    return (int16_t)cmps14_get_data_16bit(bus, CMPS14_REG_ACCEL_Z_16BIT_HIGH);
}

// ================= Calibration =================
void cmps14_enable_auto_calibration(i2c_bus_t *bus) {
    i2c_write_byte(bus, CMPS14_I2C_ADDR, 0x00, 0x98);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_write_byte(bus, CMPS14_I2C_ADDR, 0x00, 0x95);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_write_byte(bus, CMPS14_I2C_ADDR, 0x00, 0x99);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_write_byte(bus, CMPS14_I2C_ADDR, 0x00, 0xFF);
}

void cmps14_disable_auto_calibration(i2c_bus_t *bus) {
    i2c_write_byte(bus, CMPS14_I2C_ADDR, 0x00, 0x98);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_write_byte(bus, CMPS14_I2C_ADDR, 0x00, 0x95);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_write_byte(bus, CMPS14_I2C_ADDR, 0x00, 0x99);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_write_byte(bus, CMPS14_I2C_ADDR, 0x00, 0x80);
}

bool cmps14_is_auto_calibration_enabled(i2c_bus_t *bus) {
    uint8_t status = 0;
    if (i2c_read_bytes(bus, CMPS14_I2C_ADDR, 0x1E, &status, 1) == ESP_OK) {
        return (status & 0x03) != 0;
    }
    return false;
}