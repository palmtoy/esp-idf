#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Most development boards have "boot" button attached to GPIO9
#define BOOT_BUTTON_NUM GPIO_NUM_9
// Use boot button as gpio input
// "Boot" button is active high
#define GPIO_WAKEUP_LEVEL 1
#define GPIO_WAKEUP_NUM BOOT_BUTTON_NUM // input, pulled up, interrupt from rising edge
#define GPIO_INPUT_PIN_SEL (1ULL << GPIO_WAKEUP_NUM)

static const char* WAKEUP_TAG = "GPIO_WAKEUP";

void example_wait_gpio_inactive(void)
{
    printf("Waiting for GPIO%d to go high...\n", GPIO_WAKEUP_NUM);
    while (gpio_get_level(GPIO_WAKEUP_NUM) == GPIO_WAKEUP_LEVEL) {
        ESP_LOGI(WAKEUP_TAG, "I'm sleeping...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

esp_err_t example_register_gpio_wakeup(void)
{
    /* Initialize GPIO */
    gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_NEGEDGE,     // 仅下降沿产生中断
      .pin_bit_mask = GPIO_INPUT_PIN_SEL, // bit mask of the pins, use GPIO* here
      .mode = GPIO_MODE_INPUT,            // set as input mode
      .pull_up_en = GPIO_PULLUP_ENABLE   // enable pull-up mode
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), WAKEUP_TAG, "Initialize GPIO%d failed", GPIO_WAKEUP_NUM);

    /* Enable wake up from GPIO */
    ESP_RETURN_ON_ERROR(gpio_wakeup_enable(GPIO_WAKEUP_NUM, GPIO_WAKEUP_LEVEL == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL),
                        WAKEUP_TAG, "Enable gpio wakeup failed");
    ESP_RETURN_ON_ERROR(esp_sleep_enable_gpio_wakeup(), WAKEUP_TAG, "Configure gpio as wakeup source failed");

    /* Make sure the GPIO is inactive and it won't trigger wakeup immediately */
    example_wait_gpio_inactive();
    ESP_LOGI(WAKEUP_TAG, "gpio wakeup source is ready");

    return ESP_OK;
}