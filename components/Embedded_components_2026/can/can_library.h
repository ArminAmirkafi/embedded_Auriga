#ifndef CAN_LIBRARY_H
#define CAN_LIBRARY_H

#include <stdint.h>
#include <driver/twai.h>
#include <driver/gpio.h>
#include <esp_err.h>

typedef enum {
    CAN_MODE_NORMAL,
    CAN_MODE_NO_ACK,
    CAN_MODE_LISTEN_ONLY
} can_bus_mode_t;

typedef struct {
    gpio_num_t rx_io;
    gpio_num_t tx_io;
    uint32_t baudrate;
    can_bus_mode_t mode;
} can_bus_config_t;

esp_err_t can_bus_init(const can_bus_config_t *config);

esp_err_t can_bus_send(uint32_t id, const uint8_t *data, uint8_t len);

esp_err_t can_bus_receive(twai_message_t *rx_mag, uint32_t timeout_ms);

esp_err_t can_bus_send_large_data(uint32_t id, const uint8_t *data, uint16_t len);

void can_bus_reconstruct(const twai_message_t *msg, uint8_t *buffer);

#endif