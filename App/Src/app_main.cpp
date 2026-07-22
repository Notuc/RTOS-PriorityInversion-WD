#include "app_main.hpp"
#include "CommsTask.hpp"
#include "DisplayTask.hpp"
#include "FreeRTOS.h"
#include "SensorReading.hpp"
#include "SensorTask.hpp"
#include "cmsis_os.h"
#include "event_groups.h"
#include "queue.h"

QueueHandle_t xDisplayQueue = nullptr;
QueueHandle_t xCommsQueue = nullptr;

// Static task instances — no heap allocation
static SensorTask sensorTask;
static DisplayTask displayTask;
static CommsTask commsTask;

void AppInit(void) {
  // Create queues
  xDisplayQueue = xQueueCreate(1, sizeof(SensorReading_t));
  xCommsQueue = xQueueCreate(8, sizeof(SensorReading_t));

  configASSERT(xDisplayQueue);
  // Tasks create themselves in their constructors
}
