#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "MPU6050.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "utils.h"

// static const char* MY_MPU6050_TAG = "MY_MPU6050";
float G_DEG_CONVERTOR = 180 / M_PI;  // ( 180 / π )
static int64_t G_MPU_INIT_TIME = 5000; // 5s
static bool G_B_MPU_INITED = false;

gpio_num_t I2C_PIN_SDA = GPIO_NUM_21;
gpio_num_t I2C_PIN_CLK = GPIO_NUM_22;

MPU6050* G_P_MPU_OBJ = NULL;
Quaternion G_DMP_QUAT;             // [w, x, y, z]         quaternion container
VectorFloat G_VECT_GRAVITY;        // [x, y, z]            gravity vector
float G_MPU_YPR[3];                // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
static uint8_t G_YAW_INDEX = 0;    // for G_MPU_YPR
static uint8_t G_PITCH_INDEX = 1;  // for G_MPU_YPR
static uint8_t G_ROLL_INDEX = 2;   // for G_MPU_YPR
uint16_t G_PACKET_SIZE = 42;       // expected DMP packet size (default is 42 bytes)
uint16_t G_FIFO_COUNT;             // count of all bytes currently in FIFO
uint8_t G_FIFO_BUFFER[64];         // FIFO storage buffer
uint8_t G_MPU_INT_STATUS;          // holds actual interrupt status byte from MPU

void _funcTaskInitI2C(void *pvParameters) {
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = I2C_PIN_SDA;
  conf.scl_io_num = I2C_PIN_CLK;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 400000;
  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
  vTaskDelete(NULL);
}

void _calcYPR() {
  G_MPU_INT_STATUS = G_P_MPU_OBJ->getIntStatus();
  // get current FIFO count
  G_FIFO_COUNT = G_P_MPU_OBJ->getFIFOCount();

  if ((G_MPU_INT_STATUS & 0x10) || G_FIFO_COUNT == 1024) {
    G_P_MPU_OBJ->resetFIFO(); // reset so we can continue cleanly
  } else if (G_MPU_INT_STATUS & 0x02) {
    // otherwise, check for DMP data ready interrupt frequently)
    // wait for correct available data length, should be a VERY short wait
    while (G_FIFO_COUNT < G_PACKET_SIZE) {
      G_FIFO_COUNT = G_P_MPU_OBJ->getFIFOCount();
    }
    // read a packet from FIFO
    G_P_MPU_OBJ->getFIFOBytes(G_FIFO_BUFFER, G_PACKET_SIZE);
    G_P_MPU_OBJ->dmpGetQuaternion(&G_DMP_QUAT, G_FIFO_BUFFER);
    G_P_MPU_OBJ->dmpGetGravity(&G_VECT_GRAVITY, &G_DMP_QUAT);
    G_P_MPU_OBJ->dmpGetYawPitchRoll(G_MPU_YPR, &G_DMP_QUAT, &G_VECT_GRAVITY);
    // ESP_LOGI(MY_MPU6050_TAG, "YAW: %3.1f, PITCH: %3.1f, ROLL: %3.1f",
    //   G_MPU_YPR[0] * G_DEG_CONVERTOR, G_MPU_YPR[1] * G_DEG_CONVERTOR, G_MPU_YPR[2] * G_DEG_CONVERTOR);
  }
}

float _getYPRByIndex(uint8_t idx) {
  if (!G_B_MPU_INITED) {
    return 0.0;
  }
  _calcYPR();
  return G_MPU_YPR[idx] * G_DEG_CONVERTOR;
}

float getYaw() {
  return _getYPRByIndex(G_YAW_INDEX);
}

float getPitch() {
  return _getYPRByIndex(G_PITCH_INDEX);
}

float getRoll() {
  return _getYPRByIndex(G_ROLL_INDEX);
}

void _funcInitMPU6050(void* pvParameters){
  G_P_MPU_OBJ = new MPU6050();
  G_P_MPU_OBJ->initialize();
  G_P_MPU_OBJ->dmpInitialize();

  // This need to be setup individually
  G_P_MPU_OBJ->setXGyroOffset(220);
  G_P_MPU_OBJ->setYGyroOffset(76);
  G_P_MPU_OBJ->setZGyroOffset(-85);
  G_P_MPU_OBJ->setZAccelOffset(1788);

  G_P_MPU_OBJ->setDMPEnabled(true);

  int64_t startTime = esp_timer_get_time();
  while (true) {
    _calcYPR();
    // Best result is to match with DMP refresh rate
    // Its last value in components/MPU6050/MPU6050_6Axis_MotionApps20.h file line 310
    // Now its 0x13, which means DMP is refreshed with 10Hz rate
    vTaskDelay(100 / portTICK_PERIOD_MS);
    int64_t endTime = esp_timer_get_time();
    if (endTime - startTime > G_MPU_INIT_TIME) {
      G_B_MPU_INITED = true;
      break;
    }
  }

  vTaskDelete(NULL);
}

void initMPU6050Loop() {
  xTaskCreate(&_funcTaskInitI2C, "_funcTaskInitI2C", G_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + TASK_LOOP_PRIORITY, NULL);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  xTaskCreate(&_funcInitMPU6050, "_funcInitMPU6050", G_TASK_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + TASK_LOOP_PRIORITY, NULL);
}

#ifdef __cplusplus
}
#endif
