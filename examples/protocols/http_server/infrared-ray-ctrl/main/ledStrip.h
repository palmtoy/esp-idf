#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#define BLINK_LED_GPIO GPIO_NUM_8 // esp32c3-mini on board RGB LED
#define G_MAX_HUE 360
#define G_MAX_SATURATION 255
#define G_MAX_BRIGHTNESS 255

static const char* LED_STRIP_TAG = "LED_STRIP";
static led_strip_handle_t G_LED_STRIP;

static void do_set_led_hsv(uint16_t hue, uint8_t saturation, uint8_t value) {
  led_strip_set_pixel_hsv(G_LED_STRIP, 0, hue, saturation, value);
  led_strip_refresh(G_LED_STRIP);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

static void led_fade_in_out() {
  uint16_t hue = rand() % G_MAX_HUE;
  ESP_LOGI(LED_STRIP_TAG, "BlinkLed ~ hue = %d", hue);
  for (uint8_t brightness = 0; brightness < G_MAX_BRIGHTNESS; brightness++) {
    do_set_led_hsv(hue, G_MAX_SATURATION, brightness);
  }
  for (uint8_t brightness = G_MAX_BRIGHTNESS; brightness > 0; brightness--) {
    do_set_led_hsv(hue, G_MAX_SATURATION, brightness);
  }
  led_strip_clear(G_LED_STRIP);
}

static void init_led_strip() {
  ESP_LOGI(LED_STRIP_TAG, "Configured to blink addressable LED ( GPIO%d ).", BLINK_LED_GPIO);
  /* LED strip initialization with the GPIO and pixels number*/
  led_strip_config_t strip_config = {
      .strip_gpio_num = BLINK_LED_GPIO,
      .max_leds = 1, // at least one LED on board
      .led_pixel_format = LED_PIXEL_FORMAT_GRB,
      .led_model = LED_MODEL_WS2812,
      .flags = {0},
  };
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10 * 1000 * 1000, // 10MHz
      .mem_block_symbols = 0,
      .flags = {0},
  };
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &G_LED_STRIP));
  /* Set all LED off to clear all pixels */
  led_strip_clear(G_LED_STRIP);
  srand((unsigned)time(NULL));
}

#ifdef __cplusplus
}
#endif
