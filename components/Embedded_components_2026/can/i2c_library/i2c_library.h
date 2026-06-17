#ifndef I2C_LIBRARY_H
#define I2C_LIBRARY_H

#include "driver/i2c.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    i2c_port_t port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t clk_speed;
    TickType_t timeout;
} i2c_bus_t;

esp_err_t i2c_bus_init(i2c_bus_t *bus);

esp_err_t i2c_bus_deinit(i2c_bus_t *bus);

esp_err_t i2c_write_byte(i2c_bus_t *bus, uint8_t addr, uint8_t reg, uint8_t data);

esp_err_t i2c_write_bytes(i2c_bus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);

esp_err_t i2c_read_bytes(i2c_bus_t *bus, uint8_t addr, uint8_t reg, uint8_t *data, size_t len);

#endif