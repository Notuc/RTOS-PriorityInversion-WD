#include "app_main.hpp"
#include "CommsTask.hpp"
#include "DisplayTask.hpp"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "SensorReading.hpp"
#include "SensorTask.hpp"
#include "StorageTask.hpp"
#include "WatchdogTask.hpp";
#include "cmsis_os.h"
#include "event_groups.h"
#include "queue.h"

QueueHandle_t xDisplayQueue = nullptr;
QueueHandle_t xCommsQueue = nullptr;
QueueHandle_t xStorageQueue = nullptr;
EventGroupHandle_t xWDGroup = nullptr;

// Static task instances — no heap allocation
static SensorTask sensorTask;
static DisplayTask displayTask;
static CommsTask commsTask;
static StorageTask storageTask;
static WatchdogTask watchdogTask;

void AppInit(void) {
  // Create queues
  xDisplayQueue = xQueueCreate(1, sizeof(SensorReading_t));
  xCommsQueue = xQueueCreate(8, sizeof(SensorReading_t));
  xStorageQueue = xQueueCreate(16, sizeof(SensorReading_t));
  xWDGroup = xEventGroupCreate();

  configASSERT(xDisplayQueue);
  configASSERT(xCommsQueue);
  configASSERT(xStorageQueue);
  configASSERT(xWDGroup);
  // Tasks create themselves in their constructors
}
