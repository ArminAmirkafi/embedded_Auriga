#ifndef DC_MOTOR_MCPWM_H
#define DC_MOTOR_MCPWM_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/mcpwm_prelude.h"
#include "esp_err.h"

#define ROTATE_CLOCKWISE true
#define ROTATE_COUNTERCLOCKWISE false

// Struct holding the state, replacing the C++ class private variables
typedef struct {
    gpio_num_t enable_pin;
    gpio_num_t direction_pin;
    gpio_num_t pwm_pin;
    int mcpwm_group_id;
    uint32_t frequency_hz;
    uint32_t limit;
    
    mcpwm_timer_handle_t timer;
    mcpwm_oper_handle_t oper;
    mcpwm_cmpr_handle_t comparator;
    mcpwm_gen_handle_t generator;
} dc_motor_t;


void dc_motor_init(dc_motor_t *motor, gpio_num_t enable_pin, gpio_num_t direction_pin, gpio_num_t pwm_pin, uint32_t resolution_bits, uint32_t freq_hz, int group_id);
void dc_motor_begin(dc_motor_t *motor);
void dc_motor_set_direction(dc_motor_t *motor, bool direction);
void dc_motor_set_speed(dc_motor_t *motor, int16_t speed);

#endif // DC_MOTOR_MCPWM_H