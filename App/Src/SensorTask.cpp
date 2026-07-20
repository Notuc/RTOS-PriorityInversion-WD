#include "SensorTask.hpp"
#include "SensorReading.hpp"
#include "app_main.hpp" // queue handles
#include "cmsis_os.h"
#include "queue.h"
#include "stm32f4xx_hal.h"
#include "task.h"

static constexpr uint32_t SAMPLE_RATE_MS = 1000;

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
  for (;;) {
    SensorReading_t reading{};
    readBME280(reading);
    postToQueues(reading);
    vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE_MS));
  }
}

void SensorTask::readBME280(SensorReading_t &out) {
  out.temperature = 25.0f;
  out.pressure = 1013.0f;
  out.humidity = 50.0f;
  out.timestamp_ms = xTaskGetTickCount();
};

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
