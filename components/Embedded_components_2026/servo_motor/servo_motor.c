#include "servo_motor.h"

esp_err_t servo_motor_init(servo_motor_t *servo, gpio_num_t pwm_pin) {
    if (!servo) return ESP_ERR_INVALID_ARG;
    esp_err_t err;

    // 1. Create timer (Configured for 1MHz so 1 tick = 1 microsecond)

    mcpwm_timer_config_t timer_config ={
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, 
        .period_ticks = SERVO_PWM_PERIOD_US, 
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    err = mcpwm_new_timer(&timer_config, &servo->timer);
    if (err != ESP_OK) return err;

    // 2. Create operator


    //??!
    mcpwm_operator_config_t oper_config = {
        .group_id = 0, // Must match timer's group_id    check the site
    };
    err = mcpwm_new_operator(&oper_config, &servo->oper);
    if (err != ESP_OK) return err;

    // 3.connect timer to operator
    err = mcpwm_operator_connect_timer(servo->oper, servo->timer);
    if (err != ESP_OK) return err;

    // 4.Create comparator (the pulse width is determined by the comparator value)

    mcpwm_comparator_config_t cmpr_config = {
        .flags.update_cmp_on_tez = true, //If you write a new compare value, don't apply it immediately.
        //instead, wait until the timer reaches zero, then load the new value into the hardware comparator.
    };


    err = mcpwm_new_comparator(servo->oper, &cmpr_config, &servo->cmpr);
    if (err != ESP_OK) return err;

    // 5. Create generator (Connects the logic to your physical GPIO pin)
    mcpwm_generator_config_t gen_config = {
        .gen_gpio_num = pwm_pin,
    };
    err = mcpwm_new_generator(servo->oper, &gen_config, &servo->gen);
    if (err != ESP_OK) return err;

    // 6. Set generator actions to create standard PWM signals
    // Go HIGH at the start of the timer cycle (0 ticks)
    err = mcpwm_generator_set_action_on_timer_event(servo->gen, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    if (err != ESP_OK) return err;
    
    // Go LOW when the timer hits the comparator value (pulse width ends)
    err = mcpwm_generator_set_action_on_compare_event(servo->gen, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, servo->cmpr, MCPWM_GEN_ACTION_LOW));
    if (err != ESP_OK) return err;

    // 7. Enable and start the timer
    err = mcpwm_timer_enable(servo->timer);
    if (err != ESP_OK) return err;
    
    return mcpwm_timer_start_stop(servo->timer, MCPWM_TIMER_START_NO_STOP);
}

esp_err_t servo_motor_set_angle(servo_motor_t *servo, float angle) {
    if (!servo) return ESP_ERR_INVALID_ARG;

    if (angle < SERVO_GLOBAL_MIN_ANGLE) {
        angle = SERVO_GLOBAL_MIN_ANGLE;
    } else if (angle > SERVO_GLOBAL_MAX_ANGLE) {
        angle = SERVO_GLOBAL_MAX_ANGLE;
    }

    // Because our timer resolution is exactly 1MHz (1 tick = 1us), 
    // The comparator value is literally just the pulse width in microseconds!

    uint32_t pulse_width_us = SERVO_MIN_PW_US + (uint32_t)(((SERVO_MAX_PW_US - SERVO_MIN_PW_US) * (angle / SERVO_GLOBAL_MAX_ANGLE)));
    return mcpwm_comparator_set_compare_value(servo->cmpr, pulse_width_us);
}