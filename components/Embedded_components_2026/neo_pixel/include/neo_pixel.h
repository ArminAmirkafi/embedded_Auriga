#ifndef NEO_PIXEL_H
#define NEO_PIXEL_H

#include "driver/rmt.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "esp_err.h"


//80MHZ is a defualt value for APB clock,(12.5 ticks) and we can divide it to get the desired resolution 

#define NEO_PIXEL_RMT_CHANNEL       RMT_CHANNEL_0  //you have 8 channels. set the block you wanna use here!

#define NEO_PIXEL_NUM_LEDS          24
#define NEO_PIXEL_CLK_DIV           8   
#define NEO_PIXEL_RMT_RES_HZ        10000000//(80000000 / NEO_PIXEL_CLK_DIV)
#define NEO_PIXEL_T0H_NS            350  
#define NEO_PIXEL_T0L_NS            800
#define NEO_PIXEL_T1H_NS            700
#define NEO_PIXEL_T1L_NS            600
#define NEO_PIXEL_BITS_PER_LED      24
#define NEO_PIXEL_RESET_US          60

typedef struct {
    gpio_num_t gpio;
    rmt_channel_t channel;
    uint32_t led_buffer[NEO_PIXEL_NUM_LEDS];//RGB holder
    rmt_item32_t rmt_items[NEO_PIXEL_NUM_LEDS * NEO_PIXEL_BITS_PER_LED];
    uint8_t brightness;
    
} neopixel_t;

esp_err_t neopixel_init(neopixel_t *strip, gpio_num_t gpio);
void neopixel_set_pixel(neopixel_t *strip, uint16_t index,uint8_t r, uint8_t g, uint8_t b);
void neopixel_clear(neopixel_t *strip);
void neopixel_set_brightness(neopixel_t *strip, uint8_t brightness);
esp_err_t neopixel_show(neopixel_t *strip);

//here is some common scenaria to use
//use a good vtask delay to make it look good,or you can just use a for loop with a delay in it, but vtask delay is more efficient and less power consuming,so you can do other things while waiting for the next update!
void singal_to_other_side(neopixel_t *strip);
void full_red(neopixel_t *strip);
void full_blue(neopixel_t *strip);
void stop_light(neopixel_t *strip, uint8_t r, uint8_t g, uint8_t b, uint16_t delay_ms);
//this is preferred for stoplight
// while (1) {
//     stop_light(&strip, 255, 69, 0, 30);
//         vTaskDelay(100); // pause half a second    

// }   
#endif