#include "my_uart.h"

QueueHandle_t uart_event=NULL;
    
uart_init(gpio_port_t port, int baud_rate){
    uart_config_t uart_conf={
        .baud_rate=buad_rate,
        .parity=UART_PARITY_ENABLE,
        .stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE,
        .data_bits=UART_DATA_8_BITS
    }   
    uart_param_config(port,&uart_conf);
    uart_driver_install(port,BUF_SIZE*2,0,0,uart_event,0);
}

uart_printf(const char* _msg,...){
    __va_list args;
    va_st
}