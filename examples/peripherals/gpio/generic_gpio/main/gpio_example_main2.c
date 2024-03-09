#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO0:  input, pulled up, interrupt from rising edge and falling edge.
 * Press and then release the button IO0 on board.
 */

#define GPIO_INPUT_IO_0 0
#define GPIO_INPUT_PIN_SEL 1ULL << GPIO_INPUT_IO_0
#define ESP_INTR_FLAG_DEFAULT 0

#define BLINK_GPIO_BLUE 2  // ESP32S on board Blue LED
#define BLINK_INTERVAL 1000

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg) {
  uint32_t io_num;
  for (;;) {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
      printf("GPIO[%ld] intr, val: %d\n", io_num, gpio_get_level(io_num));
      gpio_set_level(BLINK_GPIO_BLUE, 1); // on
      vTaskDelay(BLINK_INTERVAL / portTICK_PERIOD_MS);
      gpio_set_level(BLINK_GPIO_BLUE, 0); // off
    }
  }
}

void app_main(void) {
  gpio_reset_pin(BLINK_GPIO_BLUE);
  gpio_set_direction(BLINK_GPIO_BLUE, GPIO_MODE_OUTPUT);

  gpio_config_t io_conf = {}; // zero-initialize the config structure.
  io_conf.intr_type = GPIO_INTR_POSEDGE;  // 仅上升沿产生中断
  // io_conf.intr_type = GPIO_INTR_ANYEDGE;     // 上升沿 & 下降沿都产生中断
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL; // bit mask of the pins, use GPIO0 here
  io_conf.mode = GPIO_MODE_INPUT;            // set as input mode
  io_conf.pull_up_en = 1;                    // enable pull-up mode
  gpio_config(&io_conf);

  // create a queue to handle gpio event from isr
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  // start gpio task
  xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

  // install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  // hook isr handler for specific gpio pin
  gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void *)GPIO_INPUT_IO_0);

  printf("Minimum free heap size: %ld bytes\n", esp_get_minimum_free_heap_size());
}
