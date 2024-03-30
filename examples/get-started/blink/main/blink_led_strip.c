#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#define BLINK_GPIO 8

static const char *TAG = "ESPC3 Mini";
static led_strip_handle_t led_strip;

static void do_set_led_hsv(uint16_t hue, uint8_t saturation, uint8_t value)
{
  led_strip_set_pixel_hsv(led_strip, 0, hue, saturation, value);
  led_strip_refresh(led_strip);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

static void blink_led()
{
  uint16_t hue = rand() % 360;
  ESP_LOGI(TAG, "BlinkLed ~ hue = %d", hue);
  uint8_t saturation = 255;
  uint8_t maxBrightness = 255;
  for (uint8_t brightness = 0; brightness < maxBrightness; brightness++)
  {
    do_set_led_hsv(hue, saturation, brightness);
  }
  for (uint8_t brightness = maxBrightness; brightness > 0; brightness--)
  {
    do_set_led_hsv(hue, saturation, brightness);
  }
}

static void configure_led()
{
  ESP_LOGI(TAG, "Configured to blink addressable LED ( GPIO%d ).", BLINK_GPIO);
  /* LED strip initialization with the GPIO and pixels number*/
  led_strip_config_t strip_config = {
      .strip_gpio_num = BLINK_GPIO,
      .max_leds = 1, // at least one LED on board
  };
  led_strip_rmt_config_t rmt_config = {
      .resolution_hz = 10 * 1000 * 1000, // 10MHz
      .flags.with_dma = false,
  };
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  /* Set all LED off to clear all pixels */
  led_strip_clear(led_strip);
}

void app_main()
{
  configure_led();
  while (true)
  {
    srand((unsigned)time(NULL));
    blink_led();
    led_strip_clear(led_strip);
  }
}
