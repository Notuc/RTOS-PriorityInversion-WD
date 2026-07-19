#pragma once
#include "FreeRTOS.h"
#include "SensorReading.hpp"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

void AppInit(void);

#ifdef __cplusplus
}
#endif

// Extern declarations
extern QueueHandle_t xDisplayQueue;
