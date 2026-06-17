#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"



#define UART_FRAME_MAX 512
#define UART_RX_BUF    512
#define UART_TX_BUF    512
#define SERILIZED_STRING_SIZE 100
#define SERILIZED_INT_SIZE

static const char *TAG = "MY_UART_LIB";
static QueueHandle_t event_handle = NULL;
static QueueHandle_t frame_queue = NULL;

typedef struct {
    char text[UART_FRAME_MAX];
    uint8_t len;
} uart_frame_msg_t;


typedef struct {
    char string[50];
    int number[50];

} Queue_item_t;


// --------------------------------------------------------
// UART INIT
// --------------------------------------------------------
void uart_init(uart_port_t port, int baud) {
    gpio_num_t tx, rx;
    switch (port) {
        case UART_NUM_0: tx = GPIO_NUM_1; rx = GPIO_NUM_3; break;
        case UART_NUM_1: tx = GPIO_NUM_10; rx = GPIO_NUM_9; break;
        case UART_NUM_2: tx = GPIO_NUM_17; rx = GPIO_NUM_16; break;
        default: printf("Invalid UART port!\n"); return;
    }

    uart_config_t conf = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(port, &conf));
    ESP_ERROR_CHECK(uart_set_pin(port, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(port, UART_RX_BUF, UART_TX_BUF, 20, &event_handle, 0));

    frame_queue = xQueueCreate(10, sizeof(uart_frame_msg_t));
}

// --------------------------------------------------------
// FRAME STATE MACHINE
// --------------------------------------------------------
typedef enum { WAIT_START, READ_LEN, READ_DATA, READ_CRC, WAIT_END } frame_state_t;

typedef struct {
    frame_state_t state;
    uint8_t buf[UART_FRAME_MAX];
    int len;
    int pos;
    uint8_t crc;
} bin_frame_t;

static void frame_init(bin_frame_t *f) {
    f->state = WAIT_START;
    f->pos = 0;
    f->len = 0;
    f->crc = 0;
}

// --------------------------------------------------------
// CRC8 (POLY 0x07)
// --------------------------------------------------------
static uint8_t crc8(uint8_t *data, int len) {
    uint8_t crc = 0x00;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// --------------------------------------------------------
// SEND FRAME
// --------------------------------------------------------
void uart_send_frame(uart_port_t port, uint8_t *payload, uint8_t len) {
    if (len > UART_FRAME_MAX - 4) return; // safety

    uint8_t frame[UART_FRAME_MAX];
    int pos = 0;

    frame[pos++] = 0x7E;          // start byte
    frame[pos++] = len;           // length
    memcpy(&frame[pos], payload, len);
    pos += len;
    frame[pos++] = crc8(&frame[1], len + 1); // CRC of length + payload
    frame[pos++] = 0x7F;          // end byte

    uart_write_bytes(port, (char *)frame, pos);
}

// --------------------------------------------------------
// UART PRINTF -> framed send
// --------------------------------------------------------
void uart_printf(uart_port_t port, const char *format, ...) {
    char buffer[UART_FRAME_MAX];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, UART_FRAME_MAX, format, args);
    va_end(args);

    if (len > 0)
        uart_send_frame(port, (uint8_t *)buffer, len);
}

// --------------------------------------------------------
// FRAME PARSER
// --------------------------------------------------------
bool frame_feed(bin_frame_t *f, uint8_t b, uart_frame_msg_t *msg) {
    switch (f->state) {
        case WAIT_START: if (b == 0x7E) f->state = READ_LEN; break;
        case READ_LEN:
            if (b == 0) { frame_init(f); break; }
            f->len = b;
            f->pos = 0;
            f->state = READ_DATA;
            break;
        case READ_DATA:
            f->buf[f->pos++] = b;
            if (f->pos >= f->len) f->state = READ_CRC;
            break;
        case READ_CRC:
            f->crc = b;
            f->state = WAIT_END;
            break;
        case WAIT_END:
            if (b == 0x7F) {
                uint8_t temp[1 + f->len];
                temp[0] = f->len;
                memcpy(&temp[1], f->buf, f->len);
                uint8_t calc = crc8(temp, 1 + f->len);
                if (calc == f->crc) {
                    memcpy(msg->text, f->buf, f->len);
                    msg->text[f->len] = '\0';
                    msg->len = f->len;
                    frame_init(f);
                    return true;
                }
            }
            frame_init(f);
            break;
    }
    return false;
}

// --------------------------------------------------------
// UART RX TASK
// --------------------------------------------------------
void uart_rx_task(void *arg) {
    uart_port_t port = (uart_port_t)(intptr_t)arg; 

    bin_frame_t frame;
    frame_init(&frame);

    uart_frame_msg_t msg;
    uint8_t data[128];
    uart_event_t event;

    while (1) {
        if (xQueueReceive(event_handle, &event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA: {
                    int len = uart_read_bytes(port, data, event.size, portMAX_DELAY);

                    printf("RX %d bytes: ", len);

                    for (int i = 0; i < len; i++) {
                        uint8_t b = data[i];
                        printf("%02X ", b);

                        if (frame_feed(&frame, b, &msg)) {
                            xQueueSend(frame_queue, &msg, 0);
                            printf("\nPayload received: '%s'\n", msg.text);
                        }
                    }
                    printf("\n");
                    break;
                }

                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    uart_flush_input(port);
                    xQueueReset(event_handle);
                    ESP_LOGW(TAG, "UART buffer overflow/reset");
                    break;

                default:
                    break;
            }
        }
    }
}

// --------------------------------------------------------
// UART DECODE TASK all in one peace
// --------------------------------------------------------
void uart_decode_task(void *arg) {
    uart_frame_msg_t msg;
    while (1) {
        if (xQueueReceive(frame_queue, &msg, portMAX_DELAY)) {
            printf("Decoded payload: ");
            for (int i = 0; i < msg.len; i++) {
                printf("%c", msg.text[i]);
            }
            printf("\n");
        }
    }
}
