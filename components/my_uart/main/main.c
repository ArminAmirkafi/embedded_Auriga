#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "my_uart.h"

#define UART_PORT      UART_NUM_2
#define UART_BAUDRATE  115200

void app_main(void) {
    // Initialize UART
    uart_init(UART_PORT, UART_BAUDRATE);
    ESP_LOGI("MAIN", "UART initialized on port %d with baud %d", UART_PORT, UART_BAUDRATE);
    // Create RX task
    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, (void*)(intptr_t)UART_PORT, 10, NULL);
    // Create decode task

    // Wait a bit before sending frames
    vTaskDelay(pdMS_TO_TICKS(1000));
    // Send a test frame
    const char *test_msg = "Hello ESP!";

    // Loop to periodically send data
    while (1) {
        uart_printf(UART_PORT, "%s%d", test_msg,322);

        vTaskDelay(pdMS_TO_TICKS(2000));

    }
}