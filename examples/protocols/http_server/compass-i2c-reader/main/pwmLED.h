#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define LEDC_LS_TIMER_0        LEDC_TIMER_0
#define LEDC_LS_TIMER_1        LEDC_TIMER_1
#define LEDC_LS_TIMER_2        LEDC_TIMER_2
#define LEDC_LS_TIMER_3        LEDC_TIMER_3
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_LS_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_LS_CH2_CHANNEL    LEDC_CHANNEL_2
#define LEDC_LS_CH3_CHANNEL    LEDC_CHANNEL_3
#define LEDC_FADE_TIME         (1800)
#define LEDC_PAUSE_TIME        (200)
#define LEDC_TASK_STACK_DEPTH  (1024 * 4) // 4 KiB
#define LEDC_MAX_DUTY          (8192)
#define LEDC_MIDDLE_DUTY       (1024)
#define LEDC_MIN_DUTY          (0)
#define LEDC_PRIORITY          (2)
#define LED_GPIO_RED           (3)
#define LED_GPIO_GREEN         (4)
#define LED_GPIO_BLUE          (5)
#define LED_GPIO_WARM          (18)
#define LEDC_MAX_CH_NUM        (4)
#define MAX_RDM_NUM            (70)
#define STEP_RDM_NUM           (7)
#define RDM_SCENE_3            (3)
#define RDM_SCENE_4            (4)
#define RDM_SCENE_5            (5)
#define RDM_SCENE_6            (6)
#define G_PWM_FREQ_HZ          (5000)

TaskHandle_t G_PWMLED_HANDLE = NULL;

bool G_RUNNING = false;
bool G_INIT_PWM = false;

// Prepare and set configuration of timers that will be used by LED Controller
ledc_timer_config_t G_LEDC_TIMER_LIST[LEDC_MAX_CH_NUM] = {
    {
        .duty_resolution = LEDC_TIMER_13_BIT,  // resolution of PWM duty
        .freq_hz = G_PWM_FREQ_HZ,              // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,            // timer mode
        .timer_num = LEDC_LS_TIMER_0,          // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    },
    {
        .duty_resolution = LEDC_TIMER_13_BIT,  // resolution of PWM duty
        .freq_hz = G_PWM_FREQ_HZ,              // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,            // timer mode
        .timer_num = LEDC_LS_TIMER_1,          // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    },
    {
        .duty_resolution = LEDC_TIMER_13_BIT,  // resolution of PWM duty
        .freq_hz = G_PWM_FREQ_HZ,              // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,            // timer mode
        .timer_num = LEDC_LS_TIMER_2,          // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    },
    {
        .duty_resolution = LEDC_TIMER_13_BIT,  // resolution of PWM duty
        .freq_hz = G_PWM_FREQ_HZ,              // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,            // timer mode
        .timer_num = LEDC_LS_TIMER_3,          // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    },
};

ledc_channel_config_t G_LEDC_OBJ_LIST[LEDC_MAX_CH_NUM] = {
    {
        .channel    = LEDC_LS_CH0_CHANNEL,
        .duty       = 0,
        .gpio_num   = LED_GPIO_RED,
        .speed_mode = LEDC_LS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_LS_TIMER_0
    },
    {
        .channel    = LEDC_LS_CH1_CHANNEL,
        .duty       = 0,
        .gpio_num   = LED_GPIO_GREEN,
        .speed_mode = LEDC_LS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_LS_TIMER_1
    },
    {
        .channel    = LEDC_LS_CH2_CHANNEL,
        .duty       = 0,
        .gpio_num   = LED_GPIO_BLUE,
        .speed_mode = LEDC_LS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_LS_TIMER_2
    },
    {
        .channel    = LEDC_LS_CH3_CHANNEL,
        .duty       = 0,
        .gpio_num   = LED_GPIO_WARM,
        .speed_mode = LEDC_LS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_LS_TIMER_3
    },
};

void _dimPwmLED() {
    for (uint32_t ch = 0; ch < LEDC_MAX_CH_NUM; ch++) {
        ledc_set_duty(G_LEDC_OBJ_LIST[ch].speed_mode, G_LEDC_OBJ_LIST[ch].channel, LEDC_MIN_DUTY);
        ledc_update_duty(G_LEDC_OBJ_LIST[ch].speed_mode, G_LEDC_OBJ_LIST[ch].channel);
    }
}

void _turnOnWarmLED() {
    uint32_t ch = LEDC_MAX_CH_NUM - 1;
    ledc_set_duty(G_LEDC_OBJ_LIST[ch].speed_mode, G_LEDC_OBJ_LIST[ch].channel, LEDC_MIDDLE_DUTY);
    ledc_update_duty(G_LEDC_OBJ_LIST[ch].speed_mode, G_LEDC_OBJ_LIST[ch].channel);
}

void _initPwmLED() {
    if (G_INIT_PWM) {
        return;
    }
    // Set configuration of timerX for low speed channels, LED Controller with prepared configuration
    for (uint32_t ch = 0; ch < LEDC_MAX_CH_NUM; ch++) {
        ledc_timer_config(&G_LEDC_TIMER_LIST[ch]);
        ledc_channel_config(&G_LEDC_OBJ_LIST[ch]);
    }
    // Initialize fade service.
    ledc_fade_func_install(0);
    _dimPwmLED();
    G_INIT_PWM = true;
}

uint32_t genRandomScene() {
    srand(time(NULL));
    uint rdm = (uint)(rand() % MAX_RDM_NUM);
    if (rdm < MAX_RDM_NUM / STEP_RDM_NUM) {
        return 0;
    } else if (rdm < MAX_RDM_NUM * 2 / STEP_RDM_NUM) {
        return 1;
    } else if (rdm < MAX_RDM_NUM * 3 / STEP_RDM_NUM) {
        return 2;
    } else if (rdm < MAX_RDM_NUM * 4 / STEP_RDM_NUM) {
        return RDM_SCENE_3;
    } else if (rdm < MAX_RDM_NUM * 5 / STEP_RDM_NUM) {
        return RDM_SCENE_4;
    } else if (rdm < MAX_RDM_NUM * 6 / STEP_RDM_NUM) {
        return RDM_SCENE_5;
    } else {
        return RDM_SCENE_6;
    }
}

void _doFadeUp(uint32_t ch) {
    printf("A: Channel = %d, LEDC fade up to duty = %d ~ %ld\n", ch, LEDC_MAX_DUTY, time(NULL));
    ledc_set_fade_with_time(G_LEDC_OBJ_LIST[ch].speed_mode, G_LEDC_OBJ_LIST[ch].channel, LEDC_MAX_DUTY, LEDC_FADE_TIME);
    ledc_fade_start(G_LEDC_OBJ_LIST[ch].speed_mode, G_LEDC_OBJ_LIST[ch].channel, LEDC_FADE_NO_WAIT);
}

void _doFadeDown(uint32_t ch) {
    printf("Z: Channel = %d, LEDC fade down to duty = %d\n", ch, LEDC_MIN_DUTY);
    ledc_set_fade_with_time(G_LEDC_OBJ_LIST[ch].speed_mode, G_LEDC_OBJ_LIST[ch].channel, LEDC_MIN_DUTY, LEDC_FADE_TIME);
    ledc_fade_start(G_LEDC_OBJ_LIST[ch].speed_mode, G_LEDC_OBJ_LIST[ch].channel, LEDC_FADE_NO_WAIT);
}

void _doLEDFadeUpDown(uint32_t rdmScene) {
    uint32_t ch = rdmScene;
    switch (rdmScene) {
        case RDM_SCENE_3:
            _doFadeUp(0);
            _doFadeUp(1);
            vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);

            _doFadeDown(0);
            _doFadeDown(1);
            vTaskDelay((LEDC_FADE_TIME + LEDC_PAUSE_TIME) / portTICK_PERIOD_MS);
            break;
        case RDM_SCENE_4:
            _doFadeUp(0);
            _doFadeUp(2);
            vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);

            _doFadeDown(0);
            _doFadeDown(2);
            vTaskDelay((LEDC_FADE_TIME + LEDC_PAUSE_TIME) / portTICK_PERIOD_MS);
            break;
        case RDM_SCENE_5:
            _doFadeUp(1);
            _doFadeUp(2);
            vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);

            _doFadeDown(1);
            _doFadeDown(2);
            vTaskDelay((LEDC_FADE_TIME + LEDC_PAUSE_TIME) / portTICK_PERIOD_MS);
            break;
        case RDM_SCENE_6:
            _doFadeUp(0);
            _doFadeUp(1);
            _doFadeUp(2);
            vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);

            _doFadeDown(0);
            _doFadeDown(1);
            _doFadeDown(2);
            vTaskDelay((LEDC_FADE_TIME + LEDC_PAUSE_TIME) / portTICK_PERIOD_MS);
            break;
        default:
            _doFadeUp(ch);
            vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);

            _doFadeDown(ch);
            vTaskDelay((LEDC_FADE_TIME + LEDC_PAUSE_TIME) / portTICK_PERIOD_MS);
            break;
    }
}

void _doStartPwmLED(void* pvParameters) {
    uint32_t rdmScene = genRandomScene();
    do {
        if (!G_RUNNING) {
          break;
        }
        _doLEDFadeUpDown(rdmScene);
    } while (true);
    vTaskDelete(NULL);
}

void stopPwmLED() {
    _initPwmLED();
    G_RUNNING = false;
    if (G_PWMLED_HANDLE) {
        vTaskDelete(G_PWMLED_HANDLE);
        _dimPwmLED();
        G_PWMLED_HANDLE = NULL;
    }
}

void startPwmLED() {
    _initPwmLED();
    stopPwmLED();
    _turnOnWarmLED();
    // Create the task, storing the handle.
    BaseType_t taskRet = xTaskCreate(_doStartPwmLED, "_doStartPwmLED", LEDC_TASK_STACK_DEPTH, NULL, tskIDLE_PRIORITY + LEDC_PRIORITY, &G_PWMLED_HANDLE);
    if( taskRet == pdPASS ) {
        G_RUNNING = true;
    }
}

#ifdef __cplusplus
}
#endif
