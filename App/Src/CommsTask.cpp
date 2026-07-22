#include "CommsTask.hpp"
#include "app_main.hpp"
#include "main.h" // huart2 handle
#include "stm32f4xx_hal.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

void CommsTask::run() {
  for (;;) {
    SensorReading_t r{};
    xQueueReceive(xCommsQueue, &r, portMAX_DELAY);
    transmit(r);
  }
}

void CommsTask::transmit(const SensorReading_t &r) {
  uint8_t frame[32];
  uint8_t len = serialise(r, frame);
  HAL_UART_Transmit(&huart1, frame, len, HAL_MAX_DELAY);
}

uint8_t CommsTask::serialise(const SensorReading_t &r, uint8_t *out) {
  DataFrame_t frame;
  frame.header = 0xAA;
  frame.length = sizeof(DataFrame_t) - 2; // exclude header and checksum
  frame.temperature = r.temperature;
  frame.pressure = r.pressure;
  frame.humidity = r.humidity;
  frame.timestamp_ms = r.timestamp_ms;

  // XOR checksum over payload bytes
  uint8_t *payload = (uint8_t *)&frame + 2;
  frame.checksum = 0;
  for (uint8_t i = 0; i < 16; i++) {
    frame.checksum ^= payload[i];
  }

  memcpy(out, &frame, sizeof(frame));
  return sizeof(frame);
}
