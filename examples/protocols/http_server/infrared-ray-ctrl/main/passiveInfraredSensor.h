#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "httpClient.h"
#include "ledStrip.h"

#define GPIO_INPUT_INFRARED_RAY GPIO_NUM_10 // input, pulled up, interrupt from rising edge
#define GPIO_INPUT_PIN_SEL      (1ULL << GPIO_INPUT_INFRARED_RAY)
#define ESP_INTR_FLAG_DEFAULT   0
#define G_X_QUEUE_LENGTH        16

static const char* PIR_CTRL_TAG = "PIR_CTRL"; // PIR: passive infrared ( sensor )
static QueueHandle_t G_X_QUEUE_OBJ = NULL;
static time_t lastTriggerTimestamp = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(G_X_QUEUE_OBJ, &gpio_num, NULL); // ISR: interrupt service routine
}

static void gpio_task_q_recv(void *arg) {
  gpio_num_t io_num;
  for (;;) {
    if (xQueueReceive(G_X_QUEUE_OBJ, &io_num, portMAX_DELAY)) {
      int ioLv = gpio_get_level(io_num);
      printf("\n");
      ESP_LOGI(PIR_CTRL_TAG, "GPIO[%d] interrupt, level: %d", io_num, ioLv);
      struct timeval tv;
      if (gettimeofday(&tv, NULL) != 0) {
        ESP_LOGE(PIR_CTRL_TAG, "Failed to obtain time. Skip...");
        vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms
      } else {
        ESP_LOGI(PIR_CTRL_TAG, "TimeVal-sec = %lld, ms = %ld", tv.tv_sec, tv.tv_usec / 1000);
        time_t curMs = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        if (curMs - lastTriggerTimestamp <= 2 * 1000) { // 2000ms
          ESP_LOGW(PIR_CTRL_TAG, "Debounce. Skip...");
          vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms
        } else {
          if (ioLv > 0) {
            lastTriggerTimestamp = curMs;
            led_fade_in_out();
            char pStrQuery[] = "switch";
            sendHttpRequest(pStrQuery);
          } else {
            ESP_LOGW(PIR_CTRL_TAG, "GPIO[%d] falling edge. Skip...", io_num);
            vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms
          }
        }
      }
    }
  }
}

void initPIRSensorLoop() {
  gpio_config_t io_conf = {};                // zero-initialize the config structure.
  io_conf.intr_type = GPIO_INTR_POSEDGE;     // 仅上升沿产生中断
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL; // bit mask of the pins, use GPIO* here
  io_conf.mode = GPIO_MODE_INPUT;            // set as input mode
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;   // enable pull-up mode
  gpio_config(&io_conf);

  // create a queue to handle gpio event from isr
  G_X_QUEUE_OBJ = xQueueCreate(G_X_QUEUE_LENGTH, sizeof(uint32_t));
  // start gpio task
  xTaskCreate(gpio_task_q_recv, "GPIO_TASK_Q_RECV", G_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + TASK_LOOP_PRIORITY, NULL);

  // install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  // hook isr handler for specific gpio pin
  gpio_isr_handler_add(GPIO_INPUT_INFRARED_RAY, gpio_isr_handler, (void *)GPIO_INPUT_INFRARED_RAY);

  init_led_strip();
  ESP_LOGI(PIR_CTRL_TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
}

#ifdef __cplusplus
}
#endif
