#pragma once
#include "FreeRTOS.h"
#include "SensorReading.hpp"
#include "event_groups.h"
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
extern QueueHandle_t xCommsQueue;
extern QueueHandle_t xStorageQueue;
extern EventGroupHandle_t xWDGroup;
