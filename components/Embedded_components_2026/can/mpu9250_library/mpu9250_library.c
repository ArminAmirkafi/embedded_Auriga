#include "mpu9250_library.h"

static const char *TAG = "MPU_LIB";
static i2c_bus_t *mpu_bus = NULL;

/* ================= I2C CONFIG ================= */
#define MPU_ADDR 0x68
#define MAG_ADDR 0x0C  // آدرس AK8963

/* ================= REGISTERS ================= */
#define REG_PWR_MGMT_1    0x6B
#define REG_INT_PIN_CFG   0x37 // برای فعال سازی Bypass
#define REG_ACCEL_XOUT_H  0x3B
#define REG_GYRO_CONFIG   0x1B
#define REG_ACCEL_CONFIG  0x1C
#define REG_MAG_XOUT_L    0x03
#define REG_MAG_CNTL1     0x0A // کنترل مگنتومتر
#define REG_MAG_ASAX      0x10 // ضریب حساسیت مگنتومتر (ASA)

/* ================= CONSTANTS ================= */
#define DEG2RAD 0.01745329251f
#define RAD2DEG 57.2957795f

/* ================= GLOBALS ================= */
static float ax, ay, az;
static float gx, gy, gz;
static float mx, my, mz;

static float gyro_bias_x = 0;
static float gyro_bias_y = 0;
static float gyro_bias_z = 0;

static float mag_scale_x, mag_scale_y, mag_scale_z; // ضرایب ASA خوانده شده از تراشه

/* quaternion */
static float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};
static float beta = 0.2f;   // Madgwick gain - تنظیم شده برای پاسخ سریعتر

/* ================= SENSOR READ ================= */
static void read_mpu(void)
{
    uint8_t buf[14];
    i2c_read_bytes(mpu_bus, MPU_ADDR, REG_ACCEL_XOUT_H, buf, 14);

    int16_t raw_ax = (buf[0] << 8) | buf[1];
    int16_t raw_ay = (buf[2] << 8) | buf[3];
    int16_t raw_az = (buf[4] << 8) | buf[5];
    int16_t raw_gx = (buf[8] << 8) | buf[9];
    int16_t raw_gy = (buf[10] << 8) | buf[11];
    int16_t raw_gz = (buf[12] << 8) | buf[13];

    // Accel: +-16g scale -> 2048 LSB/g (با توجه به 0x18 در REG_ACCEL_CONFIG)
    ax = raw_ax / 2048.0f;
    ay = raw_ay / 2048.0f;
    az = raw_az / 2048.0f;

    // Gyro: +-2000dps scale -> 16.4 LSB/dps (با توجه به 0x18 در REG_GYRO_CONFIG)
    gx = (raw_gx / 16.4f - gyro_bias_x) * DEG2RAD;
    gy = (raw_gy / 16.4f - gyro_bias_y) * DEG2RAD;
    gz = (raw_gz / 16.4f - gyro_bias_z) * DEG2RAD;

    // Deadzone filter
    if (fabs(gx) < 0.002f) gx = 0;
    if (fabs(gy) < 0.002f) gy = 0;
    if (fabs(gz) < 0.002f) gz = 0;
}

static void read_mag(void)
{
    uint8_t buf[7];
    i2c_read_bytes(mpu_bus, MAG_ADDR, REG_MAG_XOUT_L, buf, 7);

    // AK8963 Little Endian است
    int16_t raw_mx_mag = (buf[1] << 8) | buf[0];
    int16_t raw_my_mag = (buf[3] << 8) | buf[2];
    int16_t raw_mz_mag = (buf[5] << 8) | buf[4];
    
    // مقیاس خام (در مد 16-bit)
    // 1 LSB = 0.6uT در مد 16-bit
    const float raw_scale = 0.6f; 

    // اعمال ASA و مقیاس: ASA * Raw * 0.6
    float x_mag_raw_scaled = (float)raw_mx_mag * mag_scale_x * raw_scale;
    float y_mag_raw_scaled = (float)raw_my_mag * mag_scale_y * raw_scale;
    float z_mag_raw_scaled = (float)raw_mz_mag * mag_scale_z * raw_scale;
    
    // ******************* اصلاح تراز محوری (Misalignment) *******************
    // X_MPU = Y_Mag
    // Y_MPU = X_Mag
    // Z_MPU = -Z_Mag 
    
    mx = y_mag_raw_scaled; 
    my = x_mag_raw_scaled;
    mz = -z_mag_raw_scaled;
}

/* ================= GYRO CALIBRATION ================= */
static void calibrate_gyro(void)
{
    const int samples = 500;
    float sx = 0, sy = 0, sz = 0;
    ESP_LOGI(TAG, "Calibrating Gyro... Keep still!");

    for(int i=0; i<samples; i++){
        uint8_t buf[6];
        i2c_read_bytes(mpu_bus, MPU_ADDR, 0x43, buf, 6);
        int16_t rx = (buf[0]<<8)|buf[1];
        int16_t ry = (buf[2]<<8)|buf[3];
        int16_t rz = (buf[4]<<8)|buf[5];
        sx += rx/16.4f; // 2000 dps
        sy += ry/16.4f;
        sz += rz/16.4f;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    gyro_bias_x = sx/samples;
    gyro_bias_y = sy/samples;
    gyro_bias_z = sz/samples;
    ESP_LOGI(TAG, "Calibration Done. Bias (dps): X=%.2f Y=%.2f Z=%.2f", gyro_bias_x, gyro_bias_y, gyro_bias_z);
}

/* ================= MADGWICK COMPLETE ================= */
static void madgwick_update(float dt)
{
    float q1=q[0], q2=q[1], q3=q[2], q4=q[3];
    float norm;
    float hx, hy, _2bx, _2bz, _4bx, _4bz;
    float s1, s2, s3, s4;
    float qDot1, qDot2, qDot3, qDot4;

    float _2q1 = 2.0f * q1;
    float _2q2 = 2.0f * q2;
    float _2q3 = 2.0f * q3;
    float _2q4 = 2.0f * q4;

    // نرمالایز شتاب
    norm = sqrtf(ax*ax + ay*ay + az*az);
    if(norm == 0.0f) return;
    ax /= norm; ay /= norm; az /= norm;

    // نرمالایز مگنتومتر
    norm = sqrtf(mx*mx + my*my + mz*mz);
    if(norm == 0.0f) return;
    mx /= norm; my /= norm; mz /= norm;

    float _2q1q3 = 2.0f * q1 * q3;
    float _2q3q4 = 2.0f * q3 * q4;
    float q1q1 = q1 * q1;
    float q1q2 = q1 * q2;
    float q1q3 = q1 * q3;
    float q1q4 = q1 * q4;
    float q2q2 = q2 * q2;
    float q2q3 = q2 * q3;
    float q2q4 = q2 * q4;
    float q3q3 = q3 * q3;
    float q3q4 = q3 * q4;
    float q4q4 = q4 * q4;

    hx = mx * q1q1 - (2.f * q1 * my) * q4 + (2.f * q1 * mz) * q3 + mx * q2q2 + (2.f * q2 * my) * q3 + (2.f * q2 * mz) * q4 - mx * q3q3 - mx * q4q4;
    hy = (2.f * q1 * mx) * q4 + my * q1q1 - (2.f * q1 * mz) * q2 + (2.f * q2 * mx) * q3 - my * q2q2 + my * q3q3 + (2.f * q3 * mz) * q4 - my * q4q4;
    _2bx = sqrtf(hx * hx + hy * hy);
    _2bz = -(2.f * q1 * mx) * q3 + (2.f * q1 * my) * q2 + mz * q1q1 + (2.f * q2 * mx) * q4 - mz * q2q2 + (2.f * q3 * my) * q4 - mz * q3q3 + mz * q4q4;
    _4bx = 2.0f * _2bx;
    _4bz = 2.0f * _2bz;

    s1 = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s2 = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s3 = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s4 = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);

    norm = sqrtf(s1*s1 + s2*s2 + s3*s3 + s4*s4);
    s1 /= norm; s2 /= norm; s3 /= norm; s4 /= norm;

    qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
    qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
    qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
    qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

    q1 += qDot1 * dt;
    q2 += qDot2 * dt;
    q3 += qDot3 * dt;
    q4 += qDot4 * dt;

    norm = sqrtf(q1*q1 + q2*q2 + q3*q3 + q4*q4);
    q[0] = q1 / norm;
    q[1] = q2 / norm;
    q[2] = q3 / norm;
    q[3] = q4 / norm;
}

/* ================= PUBLIC ================= */
void mpu9250_init(i2c_bus_t *bus)
{
    mpu_bus = bus;

    i2c_write_byte(mpu_bus, MPU_ADDR, REG_PWR_MGMT_1, 0x80);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_write_byte(mpu_bus, MPU_ADDR, REG_PWR_MGMT_1, 0x00);
    vTaskDelay(pdMS_TO_TICKS(100));

    i2c_write_byte(mpu_bus, MPU_ADDR, REG_GYRO_CONFIG, 0x18);  
    i2c_write_byte(mpu_bus, MPU_ADDR, REG_ACCEL_CONFIG, 0x18); 

    i2c_write_byte(mpu_bus, MPU_ADDR, REG_INT_PIN_CFG, 0x02);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t asa_buf[3];
    i2c_write_byte(mpu_bus, MAG_ADDR, REG_MAG_CNTL1, 0x1F); 
    vTaskDelay(pdMS_TO_TICKS(100));
    
    i2c_read_bytes(mpu_bus, MAG_ADDR, REG_MAG_ASAX, asa_buf, 3); 
    
    mag_scale_x = ((float)(asa_buf[0] - 128) / 256.0f) + 1.0f;
    mag_scale_y = ((float)(asa_buf[1] - 128) / 256.0f) + 1.0f;
    mag_scale_z = ((float)(asa_buf[2] - 128) / 256.0f) + 1.0f;

    i2c_write_byte(mpu_bus, MAG_ADDR, REG_MAG_CNTL1, 0x16);
    vTaskDelay(pdMS_TO_TICKS(10));

    calibrate_gyro();
}

void mpu9250_update(float dt)
{
    read_mpu();
    read_mag();
    madgwick_update(dt);
}

mpu9250_rpy_t mpu9250_get_rpy(void)
{
    mpu9250_rpy_t r;
    
    // Roll (چرخش حول محور X)
    r.roll  = atan2f(2*(q[0]*q[1]+q[2]*q[3]), 1-2*(q[1]*q[1]+q[2]*q[2])) * RAD2DEG;
    
    // Pitch (چرخش حول محور Y)
    // از asin برای جلوگیری از gimbal lock استفاده می‌شود
    r.pitch = asinf(2*(q[0]*q[2]-q[3]*q[1])) * RAD2DEG;
    
    // Yaw (چرخش حول محور Z)
    r.yaw   = atan2f(2*(q[0]*q[3]+q[1]*q[2]), 1-2*(q[2]*q[2]+q[3]*q[3])) * RAD2DEG;
    
    return r;
}