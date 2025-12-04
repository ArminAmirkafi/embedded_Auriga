#ifndef MY_UART_H
#define MY_UART_H
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define UART_FRAME_MAX 256
#define RX_BUF 256
#define TX_BUF 256

// Structure for decoded frames


// ==========================
// Public API
// ==========================

// Initialize UART + framing
void uart_init(uart_port_t port, int baud);


// Same as printf but sent as a framed packet
void uart_printf(uart_port_t port, const char *format, ...);

// RX parser task (handles UART events)
void uart_rx_task(void *arg);

// Queue for consuming decoded frames
QueueHandle_t uart_get_frame_queue(void);

// Decode task example implementation
void uart_decode_task(void *arg);

// Debug function: how many bytes are pending in RX buffer
void RX_used_byetes(uart_port_t port);

#endif // MY_UART_H