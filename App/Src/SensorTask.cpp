#include "SensorTask.hpp"
#include "FreeRTOS.h"
#include "SensorReading.hpp"
#include "app_main.hpp" // queue handles
#include "cmsis_os.h"
#include "queue.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "task.h"
#include "watchdog_bits.hpp"

// Prevent name mangling issue from the c++ complier
extern "C" {
#include "bme280.h"
}

// Sensor Polling rate
// Hardware handles
static constexpr uint32_t SAMPLE_RATE_MS = 1000;
extern I2C_HandleTypeDef hi2c1;
BME280_HandleTypeDef bme280;

// Constructor
SensorTask::SensorTask() : Task("Sensor", 512, osPriorityHigh) {
  // mutex to protect the shared 12c bus against multi-task contention
  i2cMutex_ = xSemaphoreCreateMutex();
  // Crash early in debug if memory allocation for mutex fails
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
  vTaskDelay(pdMS_TO_TICKS(100));

  for (;;) {
    SensorReading_t reading{};
    if (readBME280(reading)) {
      postToQueues(reading);
      xEventGroupSetBits(xWDGroup, WD_BIT_SENSOR);
    }
    vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE_MS));
  }
}

bool SensorTask::readBME280(SensorReading_t &out) {
  HAL_StatusTypeDef status =
      BME280_ReadAll(&bme280, &out.temperature, &out.pressure, &out.humidity);

  if (status == HAL_OK) {
    out.timestamp_ms = xTaskGetTickCount();
    return true;
  }
  return false;
}
void SensorTask::postToQueues(const SensorReading_t &r) {
  xQueueOverwrite(xDisplayQueue, &r); // always latest
  xQueueSend(xCommsQueue, &r, HAL_MAX_DELAY);
  xQueueSend(xStorageQueue, &r, HAL_MAX_DELAY);
}
// Called when an I2C DMA reception finishes. Unblocks the task from ISR context
void SensorTask::onDMAComplete() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // Task notificaiton to wake up the sensor thread
  vTaskNotifyGiveFromISR(handle(), &xHigherPriorityTaskWoken);
  // Request a context switch if the notified task has a higher priority than
  // than the interrupted code
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// HAL ISR callback — must have C linkage
extern "C" void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  SensorTask::getInstance().onDMAComplete();
}
