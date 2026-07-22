#pragma once
#include "FreeRTOS.h"
#include "SensorReading.hpp"
#include "Task.hpp"
#include "semphr.h"

class SensorTask : public Task {
public:
  SensorTask();

  // ISR callback calls
  void onDMAComplete();

  // Singleton — ISR callback needs to reach this instance
  static SensorTask &getInstance();

protected:
  void run() override;

private:
  SemaphoreHandle_t i2cMutex_;

  bool readBME280(SensorReading_t &out);
  void postToQueues(const SensorReading_t &reading);
};
