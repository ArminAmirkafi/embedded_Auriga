#include "dc_motor.h"
// 1MHz tick resolution (1 tick = 1us)
static void configure_gpio(gpio_num_t pin, gpio_mode_t mode) {
    gpio_config_t io_conf ={
        .pin_bit_mask = (1ULL << pin),
        .mode = mode,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

void dc_motor_init(dc_motor_t *motor, gpio_num_t enable_pin, gpio_num_t direction_pin, gpio_num_t pwm_pin, uint32_t resolution_bits, uint32_t freq_hz, int group_id) {
    motor->enable_pin = enable_pin;
    motor->direction_pin = direction_pin;
    motor->pwm_pin = pwm_pin;
    motor->mcpwm_group_id = group_id;
    motor->frequency_hz = freq_hz;
    
    motor->limit =1; 
    for(int i = 0; i < resolution_bits; i++) {
        motor->limit *= 2;   
    }
    motor->timer = NULL;
    motor->oper = NULL;
    motor->comparator = NULL;
    motor->generator = NULL;
}

void dc_motor_begin(dc_motor_t *motor) {
    // 1. Setup GPIOs
    configure_gpio(motor->enable_pin, GPIO_MODE_OUTPUT);
    configure_gpio(motor->direction_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(motor->enable_pin, 1);  

    // 2. Create timer
    mcpwm_timer_config_t timer_config = {
        .group_id = motor->mcpwm_group_id,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, 
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = 1000000 / motor->frequency_hz,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &motor->timer));

    // 3. Create operator
    mcpwm_operator_config_t operator_config = {
        .group_id = motor->mcpwm_group_id,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &motor->oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motor->oper, motor->timer));

    // 4. Create comparator
    mcpwm_comparator_config_t comparator_config = {
        .flags = { .update_cmp_on_tez = true },
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(motor->oper, &comparator_config, &motor->comparator));

    // 5. Create generator
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = motor->pwm_pin,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(motor->oper, &generator_config, &motor->generator));

    // 6. Set generator actions (High on zero, Low on compare match)
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(motor->generator, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
        
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(motor->generator, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, motor->comparator, MCPWM_GEN_ACTION_LOW)));

    // 7. Enable and start timer
    ESP_ERROR_CHECK(mcpwm_timer_enable(motor->timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(motor->timer, MCPWM_TIMER_START_NO_STOP));

    // 8. Initialize at 0 speed
    dc_motor_set_speed(motor, 0);
}

void dc_motor_set_direction(dc_motor_t *motor, bool direction) {
    gpio_set_level(motor->direction_pin, direction ? 1 : 0);
}

void dc_motor_set_speed(dc_motor_t *motor, int16_t speed) {
    

    if (speed > 0) {
        dc_motor_set_direction(motor, ROTATE_CLOCKWISE);         
    } else if (speed < 0) {
        dc_motor_set_direction(motor, ROTATE_COUNTERCLOCKWISE); 
    } 

    // 2. Convert the signed speed into a positive duty cycle (e.g., -512 becomes 512)
    uint16_t duty_cycle = abs(speed);
    
    if (duty_cycle > motor->limit) {
        duty_cycle = motor->limit;
    }
    
    uint32_t tl = 1000000 / motor->frequency_hz;
    uint32_t compare_val = (tl *duty_cycle) / motor->limit;
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motor->comparator, compare_val));
}
