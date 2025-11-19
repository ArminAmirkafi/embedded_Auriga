#ifndef MY_UART_H
#define MY_UART_H
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define BUF_SIZE 1024
#define UART_START 0x7E
#define UART_DONE   0x7F
//function 
void uart_init(gpio_port_t port, int baud_rate,uart_event_t* event_handle);
void uart_printf(const char* _msg,...);//omid will put his kir in it
void uart_receive();
void uart_event_handler();
printf();
void compress();
void decompress();
void crc16();
void rx_flush();
void rx();

#endif 