#ifndef __SERVO_MOTOR_H__
#define __SERVO_MOTOR_H__

#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define SERVO_MIN_PW_US             1000
#define SERVO_MAX_PW_US             1815
#define SERVO_PWM_FREQ              50
#define SERVO_PWM_PERIOD_US         (1000000 / SERVO_PWM_FREQ) // 20000us
#define SERVO_GLOBAL_MIN_ANGLE      0.0f
#define SERVO_GLOBAL_MAX_ANGLE      255.0f

#ifdef __cplusplus
extern "C" {
#endif

// The new MCPWM API uses handles for the timer, operator, comparator, and generator.
typedef struct {
    mcpwm_timer_handle_t timer;
    mcpwm_oper_handle_t oper;
    mcpwm_cmpr_handle_t cmpr;
    mcpwm_gen_handle_t gen;
} servo_motor_t;

/**
 * @brief Initialize the servo motor using MCPWM.
 * @note MCPWM dynamically allocates timers and channels, so you only need the pin.
 */
esp_err_t servo_motor_init(servo_motor_t *servo, gpio_num_t pwm_pin);

/**
 * @brief Set the angle of the servo motor.
 */
esp_err_t servo_motor_set_angle(servo_motor_t *servo, float angle);

#ifdef __cplusplus
}
#endif

#endif  // __SERVO_MOTOR_H__
