#include "SensorTask.hpp"
#include "FreeRTOS.h"
#include "SensorReading.hpp"
#include "app_main.hpp" // queue handles
#include "cmsis_os.h"
#include "queue.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "task.h"

extern "C" {
#include "bme280.h"
}
static constexpr uint32_t SAMPLE_RATE_MS = 1000;
extern I2C_HandleTypeDef hi2c1;
BME280_HandleTypeDef bme280;

SensorTask::SensorTask() : Task("Sensor", 512, osPriorityHigh) {
  i2cMutex_ = xSemaphoreCreateMutex();
  configASSERT(i2cMutex_);
}

SensorTask &SensorTask::getInstance() {
  // Defined as static in app_main
  // this just provides external access
  extern SensorTask sensorTask;
  return sensorTask;
}

void SensorTask::run() {
  BME280_Init(&bme280, &hi2c1, BME280_I2C_ADDR_PRIMARY);
  vTaskDelay(pdMS_TO_TICKS(100)); // let sensor stabilise after init
                                  //
  for (;;) {
    SensorReading_t reading{};
    readBME280(reading);
    postToQueues(reading);
    vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE_MS));
  }
}
void SensorTask::readBME280(SensorReading_t &out) {
  HAL_StatusTypeDef status =
      BME280_ReadAll(&bme280, &out.temperature, &out.pressure, &out.humidity);

  if (status == HAL_OK) {
    out.timestamp_ms = xTaskGetTickCount();
  } else {
    // Sentinel values so failure is on screen
    out.temperature = -1.0f;
    out.pressure = -1.0f;
    out.humidity = -1.0f;
    out.timestamp_ms = 0;
  }
}
void SensorTask::postToQueues(const SensorReading_t &r) {
  xQueueOverwrite(xDisplayQueue, &r); // always latest
}

void SensorTask::onDMAComplete() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(handle(), &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// HAL ISR callback — must have C linkage
extern "C" void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  SensorTask::getInstance().onDMAComplete();
}
