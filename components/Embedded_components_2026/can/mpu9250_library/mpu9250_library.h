#include <stdint.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c_library.h"

#ifndef MPU9250_LIBRARY_H
#define MPU9250_LIBRARY_H

typedef struct {
    float roll;
    float pitch;
    float yaw;
} mpu9250_rpy_t;

/**
 * @brief راه اندازی MPU9250 و کالیبراسیون اولیه ژیروسکوپ
 * این تابع I2C را تنظیم کرده و مگنتومتر را فعال می‌کند.
 */
void mpu9250_init(i2c_bus_t *bus);

/**
 * @brief خواندن سنسورها و اجرای الگوریتم Madgwick
 * @param dt زمان سپری شده بین دو فراخوانی (به ثانیه)
 */
void mpu9250_update(float dt);

/**
 * @brief دریافت زوایای محاسبه شده
 * @return ساختار شامل Roll, Pitch, Yaw
 */
mpu9250_rpy_t mpu9250_get_rpy(void);

#endif // MPU9250_LIBRARY_H