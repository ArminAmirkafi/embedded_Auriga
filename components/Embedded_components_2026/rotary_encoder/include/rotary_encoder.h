#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_attr.h"
#include <stdint.h>
#include <stdbool.h>

//so it's right on interrupt handling stuff so relax
//before doing anything,make sure that  gpio_install_isr_service(ESP_INTR_FLAG_IRAM); is run once and only once
//you can check it out,whether used or not by boolean isr_service_installed.
#define COUNT_CLOCKWISE        -1
#define COUNT_COUNTERCLOCKWISE +1

typedef struct {
    gpio_num_t gpio_A;
    gpio_num_t gpio_B;
    volatile int32_t angle;
    int8_t direction; 
    uint8_t prev_state; 
} rotary_encoder_t;

//not necessary,only use it if you haven't called up the isr service.
//if having used already,then use gpio_isr_handler_add('pin', 'your intr_func' , 'your void pointer awaits to be cast') directly,otherwise you are prone to get ESP_ERR_INVALID_STATE error

esp_err_t rotary_encoder_isr_service_init(void);

//all needed functions for rotary encoder
esp_err_t rotary_encoder_init(rotary_encoder_t *enc);
void rotary_encoder_calibrate(rotary_encoder_t *enc, int32_t current_value);
int32_t rotary_encoder_get(rotary_encoder_t *enc);

#endif