#pragma once

#include "SensorReading.hpp"
#include "Task.hpp"
#include "stm32f4xx_hal.h"
#include <stdint.h>

class CommsTask : public Task {
public:
  CommsTask() : Task("Comms", 512, osPriorityAboveNormal) {};

  CommsTask(const CommsTask &) = delete;
  CommsTask &operator=(const CommsTask &) = delete;
  CommsTask(CommsTask &&) = delete;
  CommsTask &operator=(CommsTask &&) = delete;

  // frame structure
  struct DataFrame_t {
    uint8_t header; // always 0xAA
    uint8_t length; // payload length in bytes
    float temperature;
    float pressure;
    float humidity;
    uint32_t timestamp_ms;
    uint8_t checksum; // XOR of all payload bytes
  } __attribute__((packed));

protected:
  void run() override;

private:
  uint8_t serialise(const SensorReading_t &r, uint8_t *out);
  void transmit(const SensorReading_t &r);
};
