#include "neo_pixel.h"
#include "esp_rom_sys.h"   //ets_delay_us use 

static inline uint32_t ns_to_ticks(uint32_t time){
        return (uint32_t)(((uint64_t)time * NEO_PIXEL_RMT_RES_HZ) / 1000000000ULL);
}

esp_err_t neopixel_init(neopixel_t *strip, gpio_num_t gpio){
        strip->gpio = gpio; //data link   
        strip->channel = NEO_PIXEL_RMT_CHANNEL;
        strip->brightness = 255;
        for (int i = 0; i < NEO_PIXEL_NUM_LEDS; i++)
            strip->led_buffer[i] = 0;

        rmt_config_t config = {
            .rmt_mode = RMT_MODE_TX, 
            .channel = strip->channel,
            .gpio_num = gpio,
            .clk_div = NEO_PIXEL_CLK_DIV,//trust me! 100ns tick is nice for lightning we need,keechaow!
            .mem_block_num = 1,
            .tx_config.loop_en = false,     //don't loop it
            .tx_config.carrier_en = false, //raw pulse width is enough for neopixel
            .tx_config.idle_output_en = true,  //leave it,unless you are a glich fan,so by doing it this way so,you'll be prompted to idle after sending,which has been set down here!
            .tx_config.idle_level = RMT_IDLE_LEVEL_LOW
        };

        ESP_ERROR_CHECK(rmt_config(&config));
        ESP_ERROR_CHECK(rmt_driver_install(strip->channel, 0, 0));
        return ESP_OK;
}

void neopixel_set_brightness(neopixel_t *strip, uint8_t brightness){
        strip->brightness = brightness;
}

void neopixel_set_pixel(neopixel_t *strip, uint16_t index,uint8_t r, uint8_t g, uint8_t b){
        if (index >= NEO_PIXEL_NUM_LEDS) return;

        r = (r * (strip->brightness)) / 255;
        g = (g * (strip->brightness)) / 255;
        b = (b * (strip->brightness)) / 255;

        // GRB order
        strip->led_buffer[index] =((uint32_t)g << 16)|((uint32_t)r << 8) |((uint32_t)b);
}

void neopixel_clear(neopixel_t *strip){
        for (int i = 0; i < NEO_PIXEL_NUM_LEDS; i++)
            strip->led_buffer[i] = 0;
    }

esp_err_t neopixel_show(neopixel_t *strip){
        uint32_t t0h = ns_to_ticks(NEO_PIXEL_T0H_NS);
        uint32_t t0l = ns_to_ticks(NEO_PIXEL_T0L_NS);
        uint32_t t1h = ns_to_ticks(NEO_PIXEL_T1H_NS);
        uint32_t t1l = ns_to_ticks(NEO_PIXEL_T1L_NS);

        int item_index =0;
        for (int i = 0; i < NEO_PIXEL_NUM_LEDS; i++) {

            uint32_t pixel = strip->led_buffer[i];
            for (int bit =23; bit >= 0; bit--) {

                if (pixel & (1<<bit)){
                    strip->rmt_items[item_index].level0 = 1;
                    strip->rmt_items[item_index].duration0 = t1h;
                    strip->rmt_items[item_index].level1 = 0;
                    strip->rmt_items[item_index].duration1 = t1l;
                } 
                else {
                    strip->rmt_items[item_index].level0 = 1;
                    strip->rmt_items[item_index].duration0 = t0h;
                    strip->rmt_items[item_index].level1 = 0;
                    strip->rmt_items[item_index].duration1 = t0l;
                }

                item_index++;
            }
        }

        ESP_ERROR_CHECK(rmt_write_items(strip->channel,strip->rmt_items,item_index,true));
        // Reset pulse (>50us low)
        esp_rom_delay_us(NEO_PIXEL_RESET_US);

        return ESP_OK;
    }


//your scenario is here!!
//do it on a loop with vtask delay

void singal_to_other_side(neopixel_t *strip)
{
    const int N = NEO_PIXEL_NUM_LEDS;
    const int length = 6;
    static int head = 0;

    neopixel_clear(strip);

    for (int k = 0; k < length; k++) {

        int index = (head - k + N) % N;

        float factor = (float)(length - k) / length;
        uint8_t brightness = (uint8_t)(255 * factor * factor);

        neopixel_set_pixel(strip, index, 0, brightness, 0);
    }

    neopixel_show(strip);

    head = (head + 1) % N;
}

void full_red(neopixel_t *strip)
{
    for (int i = 0; i < NEO_PIXEL_NUM_LEDS; i++) {
        neopixel_set_pixel(strip, i, 255, 0, 0);
    }

    neopixel_show(strip);
}

void full_blue(neopixel_t *strip)
{
    for (int i = 0; i < NEO_PIXEL_NUM_LEDS; i++) {
        neopixel_set_pixel(strip, i, 0, 0, 255);
    }

    neopixel_show(strip);
}

void stop_light(neopixel_t *strip, uint8_t r, uint8_t g, uint8_t b, uint16_t delay_ms){
    int N = NEO_PIXEL_NUM_LEDS;
    int center = N/2;

    neopixel_clear(strip);
    neopixel_show(strip);

    for (int i = 0; i <= center; i++) {

        int left = center - i;
        int right = center+i;

        if (left >= 0)
            neopixel_set_pixel(strip, left, r, g, b);

        if (right < N)
            neopixel_set_pixel(strip, right, r, g, b);

        neopixel_show(strip);
        esp_rom_delay_us(delay_ms * 1000);
    }
}