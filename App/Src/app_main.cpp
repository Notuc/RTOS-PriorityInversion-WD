#include "app_main.hpp"
#include "CommsTask.hpp"
#include "DisplayTask.hpp"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "SensorReading.hpp"
#include "SensorTask.hpp"
#include "StorageTask.hpp"
#include "cmsis_os.h"
#include "event_groups.h"
#include "queue.h"

QueueHandle_t xDisplayQueue = nullptr;
QueueHandle_t xCommsQueue = nullptr;
QueueHandle_t xStorageQueue = nullptr;

// Static task instances — no heap allocation
static SensorTask sensorTask;
static DisplayTask displayTask;
static CommsTask commsTask;
static StorageTask storageTask;

void AppInit(void) {
  // Create queues
  xDisplayQueue = xQueueCreate(1, sizeof(SensorReading_t));
  xCommsQueue = xQueueCreate(8, sizeof(SensorReading_t));
  xStorageQueue = xQueueCreate(16, sizeof(SensorReading_t));

  configASSERT(xDisplayQueue);
  configASSERT(xCommsQueue);
  configASSERT(xStorageQueue);
  // Tasks create themselves in their constructors
}
