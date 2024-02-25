#ifndef _ESP32_BUTTON_CTRL_H_
#define _ESP32_BUTTON_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "httpClient.h"
#include "utils.h"
#include "myMPU6050.h"

#define BTN_TRIGGER_STATUS        0    // 按钮按下, 引脚电路接通, 引脚被拉低
#define BTN_RELEASE_STATUS        1    // 按钮弹起, 引脚电路断开, 引脚恢复拉高状态
#define BTN_GPIO_NUM              7
#define GESTURE_TESTING_NUM       10   // 连续动作判断次数
#define CAPTURE_THRESHOLD_NUM     3    // 连续动作判断阈值
#define CAPTURE_INTERVAL_TIME     100  // 动作判断时间间隔, unit: ms
#define GESTURE_DEGREE_THRESHOLD  5    // 连续动作幅度(角度值: X°)判断阈值
#define GESTURE_ACT_DURATION      3    // unit: s

gpio_num_t LED_GPIO_BLUE   = GPIO_NUM_2;
gpio_num_t BTN_GPIO_UP     = GPIO_NUM_32;
gpio_num_t BTN_GPIO_DOWN   = GPIO_NUM_33;
gpio_num_t BTN_GPIO_LEFT   = GPIO_NUM_25;
gpio_num_t BTN_GPIO_RIGHT  = GPIO_NUM_26;
gpio_num_t BTN_GPIO_MIDDLE = GPIO_NUM_27;
gpio_num_t BTN_GPIO_SET    = GPIO_NUM_14;
gpio_num_t BTN_GPIO_RESET  = GPIO_NUM_12;

struct S_BTN_STATUS {
  gpio_num_t gpio;
  u_int8_t status;
  char query[CAR_CMD_LENGTH];
} G_BTN_STATUS_LIST[BTN_GPIO_NUM] = {
  { BTN_GPIO_UP, BTN_RELEASE_STATUS, "car_forward" },
  { BTN_GPIO_DOWN, BTN_RELEASE_STATUS, "car_backward" },
  { BTN_GPIO_LEFT, BTN_RELEASE_STATUS, "car_left" },
  { BTN_GPIO_RIGHT, BTN_RELEASE_STATUS, "car_right" },
  { BTN_GPIO_MIDDLE, BTN_RELEASE_STATUS, "turn_right" },
  { BTN_GPIO_SET, BTN_RELEASE_STATUS, "turn_left" },
  { BTN_GPIO_RESET, BTN_RELEASE_STATUS, "switch_mode" }
};

struct S_BTN_Q_MSG {
  char query[CAR_CMD_LENGTH];
};

static const char* BTN_CTRL_TAG = "BTN_CTRL";

static const char* G_QUERY_TURN_LEFT = "turn_left";
static const char* G_QUERY_TURN_RIGHT = "turn_right";

static const char* G_QUERY_CAR_STOP = "car_stop";
static bool G_B_GESTURE_MODE = false;
static const char* G_QUERY_SWITCH_MODE = "switch_mode";

void _sendHttpQueryToQueue(const char* strQuery) {
  struct S_BTN_Q_MSG* pQMsg = (struct S_BTN_Q_MSG*)malloc(sizeof(struct S_BTN_Q_MSG));
  sprintf(pQMsg->query, "%s", strQuery);
  ESP_LOGI(BTN_CTRL_TAG, "QQQ ~ pQMsg = %d, pQMsg->query = %s", (int)pQMsg, pQMsg->query);
  xQueueSend(G_X_QUEUE_OBJ, (void *)&pQMsg, (TickType_t)0);
}

void _doGestureAction(const char* strQuery) {
  gpio_set_level(LED_GPIO_BLUE, 1);
  _sendHttpQueryToQueue(strQuery);
  vTaskDelay(GESTURE_ACT_DURATION * 1000 / portTICK_PERIOD_MS); // 延时, unit:ms
  gpio_set_level(LED_GPIO_BLUE, 0);
  _sendHttpQueryToQueue(G_QUERY_CAR_STOP);
}

static void _captureGesture(void *pvParameters) {
  while (true) {
    if (!G_B_GESTURE_MODE) {
      break;
    }
    u8_t captureLeftNum = 0;
    u8_t captureRightNum = 0;
    float rollAngle = 0.0;
    for(int i = 0; i < GESTURE_TESTING_NUM; i++) {
      float tmpRoll = getRoll();
      ESP_LOGI(BTN_CTRL_TAG, "tmpRoll = %3.1f, rollAngle: %3.1f", tmpRoll, rollAngle);
      if (tmpRoll > 0 && (tmpRoll - rollAngle > GESTURE_DEGREE_THRESHOLD)) {
        rollAngle = tmpRoll;
        captureLeftNum ++;
      } else if (tmpRoll < 0 && (rollAngle - tmpRoll > GESTURE_DEGREE_THRESHOLD)) {
        rollAngle = tmpRoll;
        captureRightNum ++;
      }
      vTaskDelay(CAPTURE_INTERVAL_TIME / portTICK_PERIOD_MS); // 延时, unit:ms
    }
    ESP_LOGI(BTN_CTRL_TAG, "captureLeftNum = %d, captureRightNum = %d\n", captureLeftNum, captureRightNum);
    if (captureLeftNum >= CAPTURE_THRESHOLD_NUM) {
      _doGestureAction(G_QUERY_TURN_LEFT);
    } else if (captureRightNum >= CAPTURE_THRESHOLD_NUM) {
      _doGestureAction(G_QUERY_TURN_RIGHT);
    }
  }
  vTaskDelete(NULL);
}

// 定时器回调函数
void timerCallback(void *arg) {
  for (int i = 0; i < BTN_GPIO_NUM; i++) {
    struct S_BTN_STATUS* pBtnStatus = G_BTN_STATUS_LIST + i;
    // 获取 BTN_GPIO_* 的电平状态
    // 0 ( BTN_TRIGGER_STATUS ): 按钮按下, 引脚电路接通, 引脚被拉低
    // 1 ( BTN_RELEASE_STATUS ): 按钮弹起, 引脚电路断开, 引脚恢复拉高状态
    uint16_t btnStatus = gpio_get_level((gpio_num_t)pBtnStatus->gpio);
    if (btnStatus ^ pBtnStatus->status) {
      pBtnStatus->status = btnStatus;
      if (BTN_TRIGGER_STATUS == btnStatus) { // 按钮被按下
        if (strcmp(pBtnStatus->query, G_QUERY_SWITCH_MODE) == 0) {
          G_B_GESTURE_MODE = !G_B_GESTURE_MODE;
          if (G_B_GESTURE_MODE) {
            ESP_LOGI(BTN_CTRL_TAG, "Gesture mode is OPEN ...\n");
            xTaskCreate(&_captureGesture, "_captureGesture", G_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + TASK_LOOP_PRIORITY, NULL);
          } else {
            ESP_LOGI(BTN_CTRL_TAG, "Gesture mode is CLOSED.\n");
          }
        } else {
          if (!G_B_GESTURE_MODE) {
            gpio_set_level(LED_GPIO_BLUE, 1);
            _sendHttpQueryToQueue(pBtnStatus->query);
          }
        }
      } else {
        if (strcmp(pBtnStatus->query, G_QUERY_SWITCH_MODE) != 0 && !G_B_GESTURE_MODE) {
          gpio_set_level(LED_GPIO_BLUE, 0);
          _sendHttpQueryToQueue(G_QUERY_CAR_STOP);
        }
      }
    }
  }
}

static void _recMsgFromQueue(void *pvParameters) {
  if(G_X_QUEUE_OBJ != NULL) {
    while (true) {
      struct S_BTN_Q_MSG* pQMsg = NULL;
      if(xQueueReceive(G_X_QUEUE_OBJ, &pQMsg, portMAX_DELAY)) {
        if (pQMsg != NULL && sizeof(pQMsg->query) > 0) {
          ESP_LOGI(BTN_CTRL_TAG, "RRR ~ pQMsg = %d, pQMsg->query = %s", (int)pQMsg, pQMsg->query);
          sendHttpRequest(pQMsg->query);
          free(pQMsg);
        }
      }
    }
  }
  vTaskDelete(NULL);
}

static void _doInitLoop(void *pvParameters) {
  // 复位/设置 LED_GPIO_BLUE 为输出模式
  gpio_reset_pin(LED_GPIO_BLUE);
  gpio_set_direction(LED_GPIO_BLUE, GPIO_MODE_OUTPUT);
  // 复位/设置 BTN_GPIO_* 为输入/上拉模式
  for (int i = 0; i < BTN_GPIO_NUM; i++) {
    struct S_BTN_STATUS* pBtnStatus = G_BTN_STATUS_LIST + i;
    gpio_reset_pin(pBtnStatus->gpio);
    gpio_set_direction(pBtnStatus->gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pBtnStatus->gpio, GPIO_PULLUP_ONLY);
  }
  // 初始化定时器
  esp_timer_init();
  // 定时器的参数
  esp_timer_create_args_t timerObj = {};
  timerObj.name = "BtnTimer";         // 定时器的名称
  timerObj.arg = NULL;                // 传递给回调函数的参数
  timerObj.callback = &timerCallback; // 回调函数
  // 定时器的句柄
  esp_timer_handle_t timerHandler;
  // 创建定时器
  ESP_ERROR_CHECK(esp_timer_create(&timerObj, &timerHandler));
  // 启动定时器, period: 100ms
  ESP_ERROR_CHECK(esp_timer_start_periodic(timerHandler, 100 * 1000));
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 延时, unit:ms
  }
  vTaskDelete(NULL);
}

void initBtnStatusLoop() {
  G_X_QUEUE_OBJ = xQueueCreate(G_X_QUEUE_LENGTH, sizeof(struct S_BTN_Q_MSG*));
  if(G_X_QUEUE_OBJ == 0) {
      ESP_LOGE(BTN_CTRL_TAG, "Create the queue FAILED.");
      abort();
      return;
  }
  xTaskCreate(&_doInitLoop, "_doInitLoop", G_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + TASK_LOOP_PRIORITY, NULL);
  xTaskCreate(&_recMsgFromQueue, "_recMsgFromQueue", G_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + TASK_LOOP_PRIORITY, NULL);
}

#ifdef __cplusplus
}
#endif

#endif /* _ESP32_BUTTON_CTRL_H_ */
