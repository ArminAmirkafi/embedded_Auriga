#ifndef MY_UART_H
#define MY_UART_H
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define BUF_SIZE 1024

typedef struct{
    int buad_rate;
    uart_port_t port;
    bool do_you_need_event_handling;

} uart_setup_t;

void uart_setup(void*);

void uart_printf(uart_port_t uart_num , const char *format , ...);

#endif