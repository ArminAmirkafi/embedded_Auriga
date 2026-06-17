#include "can_library.h"
#include "esp_log.h"

// Log tag for debugging
static const char *TAG = "CAN_BUS";

esp_err_t can_bus_init(const can_bus_config_t *config) {
    twai_mode_t selected_mode;
    switch (config->mode) {
        case CAN_MODE_NO_ACK:
            selected_mode = TWAI_MODE_NO_ACK;
            break;
        case CAN_MODE_LISTEN_ONLY:
            selected_mode = TWAI_MODE_LISTEN_ONLY;
            break;
        case CAN_MODE_NORMAL:
        default:
            selected_mode = TWAI_MODE_NORMAL;
            break;
    }

    // 1. General Configuration: Sets the TX/RX pins and operating mode
    twai_general_config_t g_config = {
        .controller_id = 0,
        .mode = selected_mode,
        .tx_io = config->tx_io,
        .rx_io = config->rx_io,
        .clkout_io = TWAI_IO_UNUSED,
        .bus_off_io = TWAI_IO_UNUSED,
        .tx_queue_len = 15,  
        .rx_queue_len = 15, 
        .alerts_enabled = TWAI_ALERT_NONE,
        .clkout_divider = 0
    };

    // 2. Timing Configuration: Sets the network speed (Baudrate)
    twai_timing_config_t t_config;
    if (config->baudrate == 1000000) {
        t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();
    } 
    else if (config->baudrate == 500000) {
        t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
    } 
    else if (config->baudrate == 250000) {
        t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
    } 
    else {
        t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_125KBITS();
        ESP_LOGW(TAG, "Baudrate not standard, defaulting to 125K");
    }

    // 3. Filter Configuration: Accept all incoming messages for now
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TWAI driver");
        return ESP_FAIL;
    }

    // Start TWAI driver
    if (twai_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start TWAI driver");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "CAN Driver started successfully");
    return ESP_OK;
}

esp_err_t can_bus_send(uint32_t id, const uint8_t *data, uint8_t len) {
    // Check if data length is within CAN standard limits (0-8 bytes)
    if (len > 8) {
        ESP_LOGE(TAG, "Data length exceeds 8 bytes");
        return ESP_ERR_INVALID_ARG;
    }

    twai_message_t message = {0};
    message.identifier = id;           // Set the CAN ID
    message.data_length_code = len;    // Set the number of bytes to send
    message.extd = 0;                  // Use Standard ID (11-bit)
    message.rtr = 0;                   // Send a Data Frame (not a remote request)
    message.ss = 0;                    // Single shot disabled
    message.self = 0;                  // Self reception disabled
    

    // Copy the payload into the message structure
    for (int i = 0; i < len; i++) {
        message.data[i] = data[i];
    }

    // Transmit message with a 1-second timeout
    return twai_transmit(&message, pdMS_TO_TICKS(1000));
}

esp_err_t can_bus_receive(twai_message_t *rx_msg, uint32_t timeout_ms) {
    // Wait for a message to arrive in the queue
    return twai_receive(rx_msg, pdMS_TO_TICKS(timeout_ms));
}

esp_err_t can_bus_deinit(void) {
    // Stop the driver first, then uninstall
    ESP_ERROR_CHECK(twai_stop());
    ESP_LOGI(TAG, "Driver stopped");
    return twai_driver_uninstall();
}

esp_err_t can_bus_send_large_data(uint32_t id, const uint8_t *data, uint16_t len) {
    uint16_t sent = 0; // the number of data packet that sends to reciever
    uint8_t seq = 0; //the number of data packet

    while(sent < len)
    {
        uint8_t packet[8];
        packet[0] = seq;

        uint8_t take = 0;

        if(len - sent > 7) {
            take = 7;
        }
        else {
            take = len - sent;
        }

        memcpy(&packet[1], &data[sent], take);

        if(can_bus_send(id, packet, take + 1) != ESP_OK) {
            return ESP_FAIL;
        }

        sent += take;
        seq++;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_OK;
}

void can_bus_reconstruct(const twai_message_t *msg, uint8_t *buffer) {
    uint8_t seq = msg->data[0];

    uint8_t payload_len = msg->data_length_code - 1;

    uint16_t offset = seq * 7;

    memcpy(&buffer[offset], &msg->data[1], payload_len);
}