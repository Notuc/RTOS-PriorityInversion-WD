#pragma once
#include <stdint.h>

struct SensorReading_t {
  float temperature; // Degress C
  float pressure;    // hPa
  float humidity;    // % RH
  uint32_t timestamp_ms;
};
