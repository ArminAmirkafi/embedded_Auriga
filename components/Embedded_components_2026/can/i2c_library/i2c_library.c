#include "i2c_library.h"

static SemaphoreHandle_t i2c_mutex = NULL;

esp_err_t i2c_bus_init(i2c_bus_t *bus) {
    if(!bus) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = bus->sda_pin,
        .scl_io_num = bus->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = bus->clk_speed
    };

    ESP_ERROR_CHECK(i2c_param_config(bus->port, &cfg));
    ESP_ERROR_CHECK(i2c_driver_install(bus->port, cfg.mode, 0, 0, 0));

    if(!i2c_mutex) {
        i2c_mutex = xSemaphoreCreateMutex();
    }

    return ESP_OK;
}

esp_err_t i2c_bus_deinit(i2c_bus_t *bus) {
    if(!bus) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_driver_delete(bus->port);
}

esp_err_t i2c_write_bytes(i2c_bus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
    if(!bus || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(bus->port, cmd, bus->timeout);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(i2c_mutex);
    return ret;
}

esp_err_t i2c_write_byte(i2c_bus_t *bus, uint8_t addr, uint8_t reg, uint8_t data) {
    return i2c_write_bytes(bus , addr , reg , &data , 1);
}

esp_err_t i2c_read_bytes(i2c_bus_t *bus, uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
    if(!bus || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    } 

    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);

    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);

    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(bus->port, cmd, bus->timeout);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(i2c_mutex);
    return ret;
}