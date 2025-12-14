#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_attr.h"

#define ENC_A GPIO_NUM_33
#define ENC_B GPIO_NUM_32

typedef enum {
    ENC_CW  = 1,
    ENC_CCW = -1
} enc_dir_t;

typedef struct {
    enc_dir_t dir;
} enc_event_t;

static QueueHandle_t enc_queue;
static volatile uint32_t last_isr_time = 0;

static void IRAM_ATTR enc_isr(void *arg)
{
    uint32_t now = xTaskGetTickCountFromISR();
    if (now - last_isr_time < pdMS_TO_TICKS(0.0001)) {
        return;
    }
    last_isr_time = now;
    
    int a = gpio_get_level(ENC_A);
    int b = gpio_get_level(ENC_B);

    enc_event_t evt;
    evt.dir = (a == b) ? ENC_CCW : ENC_CW;

    BaseType_t hp_task_woken = pdFALSE;
    xQueueSendFromISR(enc_queue, &evt, &hp_task_woken);

    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
}

static void enc_task(void *arg)
{
    int32_t pos = 0;
    enc_event_t evt;

    while (1) {
        if (xQueueReceive(enc_queue, &evt, portMAX_DELAY)) {
            pos += evt.dir;
            printf("%s | position: %ld\n",evt.dir == ENC_CW ? "CW " : "CCW",pos);
        }
    }
}

static void enc_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENC_A) | (1ULL << ENC_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(ENC_A, enc_isr, NULL);
}


