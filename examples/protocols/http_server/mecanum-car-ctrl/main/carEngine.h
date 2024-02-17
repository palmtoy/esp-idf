// Edit the macros at the top enable/disable the submodules which are used
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#define G_MAX_MOTOR_NUM       4
#define G_MOTOR_LEFT_TOP      0
#define G_MOTOR_LEFT_BOTTOM   1
#define G_MOTOR_RIGHT_TOP     2
#define G_MOTOR_RIGHT_BOTTOM  3
#define GPIO_PWM0A_OUT_L      16
#define GPIO_PWM0B_OUT_L      17
#define GPIO_PWM1A_OUT_L      18
#define GPIO_PWM1B_OUT_L      19
#define GPIO_PWM0A_OUT_R      25
#define GPIO_PWM0B_OUT_R      26
#define GPIO_PWM1A_OUT_R      32
#define GPIO_PWM1B_OUT_R      33
#define G_MAX_SPEED           100.0
#define G_DEFAULT_SPEED       40.0
#define G_TURN_SPEED          50.0
#define G_MIN_SPEED           21.0
#define G_SPEED_STEP          10.0
#define G_RUN_FORWARD         1
#define G_STOP                0
#define G_RUN_BACKWARD        -1

static const char* DC_MOTOR_TAG = "DC-MOTOR";

static int G_INIT_SPEED = G_DEFAULT_SPEED - 2 * G_SPEED_STEP; // 20
static float G_MOTOR_SPEED_LIST[G_MAX_MOTOR_NUM] = { 0.0, 0.0, 0.0, 0.0 };
static int G_MOTOR_DIRECTION_LIST[G_MAX_MOTOR_NUM] = { 0, 0, 0, 0 };
static char* G_MOTOR_INDEX_LIST[G_MAX_MOTOR_NUM] = { "Left-Top", "Left-Bottom", "Right-Top", "Right-Bottom" };

typedef void (*tSetLEDStatus)(bool);

void motorInit() {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT_L);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT_L);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, GPIO_PWM1A_OUT_L);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1B, GPIO_PWM1B_OUT_L);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, GPIO_PWM0A_OUT_R);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0B, GPIO_PWM0B_OUT_R);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, GPIO_PWM1A_OUT_R);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1B, GPIO_PWM1B_OUT_R);
    mcpwm_config_t mcpwmConfig = {
        .frequency = 1000,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &mcpwmConfig);
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &mcpwmConfig);
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &mcpwmConfig);
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &mcpwmConfig);
    for (int i = G_MOTOR_LEFT_TOP; i <= G_MOTOR_RIGHT_BOTTOM; i++) {
        G_MOTOR_SPEED_LIST[i] = G_INIT_SPEED;
    }
}

mcpwm_unit_t _getUnitByMotorIdx(int motorIdx) {
    return ( motorIdx <= G_MOTOR_LEFT_BOTTOM ) ? MCPWM_UNIT_0 : MCPWM_UNIT_1;
}

mcpwm_timer_t _getTimerByMotorIdx(int motorIdx) {
    return ( motorIdx % 2 == 0 ) ? MCPWM_TIMER_0 : MCPWM_TIMER_1;
}

void _motorForwardWithDuty(int motorIdx, float mDuty) {
    G_MOTOR_SPEED_LIST[motorIdx] = mDuty;
    G_MOTOR_DIRECTION_LIST[motorIdx] = G_RUN_FORWARD;
    ESP_LOGI(DC_MOTOR_TAG, "Motor-%s forward with duty( %f ).", G_MOTOR_INDEX_LIST[motorIdx], mDuty);
    mcpwm_unit_t tmpUnit = _getUnitByMotorIdx(motorIdx);
    mcpwm_timer_t tmpTimer = _getTimerByMotorIdx(motorIdx);
    mcpwm_set_signal_low(tmpUnit, tmpTimer, MCPWM_GEN_A);
    mcpwm_set_duty(tmpUnit, tmpTimer, MCPWM_GEN_B, mDuty);
    mcpwm_set_duty_type(tmpUnit, tmpTimer, MCPWM_GEN_B, MCPWM_DUTY_MODE_0);
}

void _motorBackwardWithDuty(int motorIdx, float mDuty) {
    G_MOTOR_SPEED_LIST[motorIdx] = mDuty;
    G_MOTOR_DIRECTION_LIST[motorIdx] = G_RUN_BACKWARD;
    ESP_LOGI(DC_MOTOR_TAG, "Motor-%s backward with duty( %f ).", G_MOTOR_INDEX_LIST[motorIdx], mDuty);
    mcpwm_unit_t tmpUnit = _getUnitByMotorIdx(motorIdx);
    mcpwm_timer_t tmpTimer = _getTimerByMotorIdx(motorIdx);
    mcpwm_set_duty(tmpUnit, tmpTimer, MCPWM_GEN_A, mDuty);
    mcpwm_set_signal_low(tmpUnit, tmpTimer, MCPWM_GEN_B);
    mcpwm_set_duty_type(tmpUnit, tmpTimer, MCPWM_GEN_A, MCPWM_DUTY_MODE_0);
}

void _motorStop(int motorIdx) {
    G_MOTOR_SPEED_LIST[motorIdx] = G_INIT_SPEED; // 30
    G_MOTOR_DIRECTION_LIST[motorIdx] = G_STOP;
    mcpwm_unit_t tmpUnit = _getUnitByMotorIdx(motorIdx);
    mcpwm_timer_t tmpTimer = _getTimerByMotorIdx(motorIdx);
    mcpwm_set_signal_low(tmpUnit, tmpTimer, MCPWM_GEN_A);
    mcpwm_set_signal_low(tmpUnit, tmpTimer, MCPWM_GEN_B);
}

void mCarStop() {
    for (int i = G_MOTOR_LEFT_TOP; i <= G_MOTOR_RIGHT_BOTTOM; i++) {
        _motorStop(i);
    }
}

void mCarSpeedUp(tSetLEDStatus pSetLEDStatus) {
    bool bTurnOnLed = false;
    for (int i = G_MOTOR_LEFT_TOP; i <= G_MOTOR_RIGHT_BOTTOM; i++) {
        float fSpeed = G_MOTOR_SPEED_LIST[i] + G_SPEED_STEP;
        if (fSpeed > G_MAX_SPEED) {
            fSpeed = G_MAX_SPEED;
        }
        if (G_MOTOR_DIRECTION_LIST[i] == G_RUN_FORWARD) {
            _motorForwardWithDuty(i, fSpeed);
            bTurnOnLed = true;
        } else if (G_MOTOR_DIRECTION_LIST[i] == G_RUN_BACKWARD) {
            _motorBackwardWithDuty(i, fSpeed);
            bTurnOnLed = true;
        }
    }
    pSetLEDStatus(bTurnOnLed);
}

void mCarSlowDown(tSetLEDStatus pSetLEDStatus) {
    for (int i = G_MOTOR_LEFT_TOP; i <= G_MOTOR_RIGHT_BOTTOM; i++) {
        float fSpeed = G_MOTOR_SPEED_LIST[i] - G_SPEED_STEP;
        if (fSpeed < G_MIN_SPEED) {
            mCarStop();
            pSetLEDStatus(false);
        } else {
            if (G_MOTOR_DIRECTION_LIST[i] == G_RUN_FORWARD) {
                _motorForwardWithDuty(i, fSpeed);
            } else if (G_MOTOR_DIRECTION_LIST[i] == G_RUN_BACKWARD) {
                _motorBackwardWithDuty(i, fSpeed);
            }
            pSetLEDStatus(true);
        }
    }
}

void mCarForward() {
    _motorForwardWithDuty(G_MOTOR_LEFT_TOP, G_DEFAULT_SPEED);
    _motorForwardWithDuty(G_MOTOR_LEFT_BOTTOM, G_DEFAULT_SPEED);
    _motorForwardWithDuty(G_MOTOR_RIGHT_TOP, G_DEFAULT_SPEED);
    _motorForwardWithDuty(G_MOTOR_RIGHT_BOTTOM, G_DEFAULT_SPEED);
}

void mCarBackward() {
    _motorBackwardWithDuty(G_MOTOR_LEFT_TOP, G_DEFAULT_SPEED);
    _motorBackwardWithDuty(G_MOTOR_LEFT_BOTTOM, G_DEFAULT_SPEED);
    _motorBackwardWithDuty(G_MOTOR_RIGHT_TOP, G_DEFAULT_SPEED);
    _motorBackwardWithDuty(G_MOTOR_RIGHT_BOTTOM, G_DEFAULT_SPEED);
}

void mCarLeftFront() {
    _motorForwardWithDuty(G_MOTOR_LEFT_BOTTOM, G_TURN_SPEED);
    _motorForwardWithDuty(G_MOTOR_RIGHT_TOP, G_TURN_SPEED);
    _motorStop(G_MOTOR_LEFT_TOP);
    _motorStop(G_MOTOR_RIGHT_BOTTOM);
}

void mCarRightFront() {
    _motorForwardWithDuty(G_MOTOR_LEFT_TOP, G_TURN_SPEED);
    _motorForwardWithDuty(G_MOTOR_RIGHT_BOTTOM, G_TURN_SPEED);
    _motorStop(G_MOTOR_LEFT_BOTTOM);
    _motorStop(G_MOTOR_RIGHT_TOP);
}

void mCarGoLeft() {
    _motorBackwardWithDuty(G_MOTOR_LEFT_TOP, G_TURN_SPEED);
    _motorForwardWithDuty(G_MOTOR_LEFT_BOTTOM, G_TURN_SPEED);
    _motorForwardWithDuty(G_MOTOR_RIGHT_TOP, G_TURN_SPEED);
    _motorBackwardWithDuty(G_MOTOR_RIGHT_BOTTOM, G_TURN_SPEED);
}

void mCarGoRight() {
    _motorForwardWithDuty(G_MOTOR_LEFT_TOP, G_TURN_SPEED);
    _motorBackwardWithDuty(G_MOTOR_LEFT_BOTTOM, G_TURN_SPEED);
    _motorBackwardWithDuty(G_MOTOR_RIGHT_TOP, G_TURN_SPEED);
    _motorForwardWithDuty(G_MOTOR_RIGHT_BOTTOM, G_TURN_SPEED);
}

void mCarTurnLeft() {
    _motorBackwardWithDuty(G_MOTOR_LEFT_TOP, G_DEFAULT_SPEED);
    _motorBackwardWithDuty(G_MOTOR_LEFT_BOTTOM, G_DEFAULT_SPEED);
    _motorForwardWithDuty(G_MOTOR_RIGHT_TOP, G_DEFAULT_SPEED);
    _motorForwardWithDuty(G_MOTOR_RIGHT_BOTTOM, G_DEFAULT_SPEED);
}

void mCarTurnRight() {
    _motorForwardWithDuty(G_MOTOR_LEFT_TOP, G_DEFAULT_SPEED);
    _motorForwardWithDuty(G_MOTOR_LEFT_BOTTOM, G_DEFAULT_SPEED);
    _motorBackwardWithDuty(G_MOTOR_RIGHT_TOP, G_DEFAULT_SPEED);
    _motorBackwardWithDuty(G_MOTOR_RIGHT_BOTTOM, G_DEFAULT_SPEED);
}

#ifdef __cplusplus
}
#endif
