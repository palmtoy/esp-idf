#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "light_sleep_example.h"

static const char* LIGHT_SLEEP_TAG = "LIGHT_SLEEP";

static void light_sleep_task(void *args)
{
    while (true)
    {
        ESP_LOGI(LIGHT_SLEEP_TAG, "Entering light sleep ...");
        /* Get timestamp before entering sleep */
        int64_t t_before_us = esp_timer_get_time();
        /* Enter sleep mode */
        esp_light_sleep_start();
        /* Get timestamp after waking up from sleep */
        int64_t t_after_us = esp_timer_get_time();

        /* Determine wake up reason */
        const char *wakeup_reason;
        switch (esp_sleep_get_wakeup_cause())
        {
        case ESP_SLEEP_WAKEUP_GPIO:
            wakeup_reason = "pin";
            break;
        default:
            wakeup_reason = "other";
            break;
        }
        ESP_LOGI(LIGHT_SLEEP_TAG, "Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms",
            wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
        /* Waiting for the gpio inactive, or the chip will continously trigger wakeup*/
        example_wait_gpio_inactive();
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    /* Enable wakeup from light sleep by gpio */
    example_register_gpio_wakeup();
    xTaskCreate(light_sleep_task, "light_sleep_task", 1024 * 4, NULL, tskIDLE_PRIORITY + 6, NULL);
}
