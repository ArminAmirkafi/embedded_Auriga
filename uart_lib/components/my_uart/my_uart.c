#include "my_uart.h"

QueueHandle_t queue=NULL;

uart_setup(void *setup_ptr){
    uart_setup_t* uart_setup_p=(uart_setup_t*)setup_ptr;
    uart_config_t uart_conf={
        .baud_rate=uart_setup_p->buad_rate,
        .parity=UART_PARITY_ENABLE,
        .stop_bits=UART_STOP_BITS_1,
        .source_clk=
    }   
    if(uart_setup_p->do_you_need_event_handling){
        queue=
    }

}