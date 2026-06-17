#include "rotary_encoder.h"
#include "freertos/FreeRTOS.h"

static bool isr_service_installed = false;


//isr handler
static void IRAM_ATTR encoder_isr_A(void *arg){
    rotary_encoder_t *enc = (rotary_encoder_t *)arg;
    if (!enc) return;
    bool A = gpio_get_level(enc->gpio_A);
    bool B = gpio_get_level(enc->gpio_B);
    if (!(A ^ B))
        enc->angle += enc->direction;
    else
        enc->angle -= enc->direction;
}

static void IRAM_ATTR encoder_isr_B(void *arg){
    rotary_encoder_t *enc = (rotary_encoder_t *)arg;
    if (!enc) return;

    bool A = gpio_get_level(enc->gpio_A);
    bool B = gpio_get_level(enc->gpio_B);

    if (A ^ B)
        enc->angle += enc->direction;
    else
        enc->angle -= enc->direction;
}



//public functions
esp_err_t rotary_encoder_isr_service_init(void){
    if (isr_service_installed)
        return ESP_OK;

    esp_err_t err = gpio_install_isr_service(isr_service_installed?0:ESP_INTR_FLAG_IRAM);//visit esp_intr_alloc.h for more details on the flag
    if (err == ESP_OK)
        isr_service_installed = true;

    return err;
}

esp_err_t rotary_encoder_init(rotary_encoder_t *enc){
    if (!enc)
        return ESP_ERR_INVALID_ARG;

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << enc->gpio_A) | (1ULL << enc->gpio_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };                                                                      

    ESP_ERROR_CHECK(gpio_config(&io));
    ESP_ERROR_CHECK(gpio_isr_handler_add(enc->gpio_A, encoder_isr_A, enc));

    ESP_ERROR_CHECK(gpio_isr_handler_add(enc->gpio_B, encoder_isr_B, enc));
    return ESP_OK;
}

void rotary_encoder_calibrate(rotary_encoder_t *enc, int32_t current_value){
    enc->angle = current_value;
}
int32_t rotary_encoder_get(rotary_encoder_t *enc)
{
    return enc->angle;
}