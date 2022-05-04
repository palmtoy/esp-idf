/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO_R 3
#define BLINK_GPIO_G 4
#define BLINK_GPIO_B 5
#define BLINK_INTERVAL 1000

void app_main(void)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_reset_pin(BLINK_GPIO_R);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO_R, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BLINK_GPIO_G);
    gpio_set_direction(BLINK_GPIO_G, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BLINK_GPIO_B);
    gpio_set_direction(BLINK_GPIO_B, GPIO_MODE_OUTPUT);
    while(1) {
        printf("Turning on the red LED\n");
        /* Blink on (output high) */
        gpio_set_level(BLINK_GPIO_R, 1);
        vTaskDelay(BLINK_INTERVAL / portTICK_PERIOD_MS);
        /* Blink off (output low) */
        gpio_set_level(BLINK_GPIO_R, 0);

        printf("Turning on the green LED\n");
        gpio_set_level(BLINK_GPIO_G, 1);
        vTaskDelay(BLINK_INTERVAL / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO_G, 0);

        printf("Turning on the blue LED\n");
        gpio_set_level(BLINK_GPIO_B, 1);
        vTaskDelay(BLINK_INTERVAL / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO_B, 0);

        printf("Turning on all LEDs\n");
        gpio_set_level(BLINK_GPIO_R, 1);
        gpio_set_level(BLINK_GPIO_G, 1);
        gpio_set_level(BLINK_GPIO_B, 1);
        vTaskDelay(BLINK_INTERVAL / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO_R, 0);
        gpio_set_level(BLINK_GPIO_G, 0);
        gpio_set_level(BLINK_GPIO_B, 0);
    }
}
