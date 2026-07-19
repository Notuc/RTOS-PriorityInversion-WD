#include "DisplayTask.hpp"
#include "FreeRTOS.h"
#include "app_main.hpp"
#include <task.h>

#define WD_BIT_DISPLAY (1 << 1)
#define REFRESH_RATE_MS 250 // 4 Hz

void DisplayTask::run() {
  for (;;) {
    SensorReading_t r{};
    // Peek — item stays in queue, no data lost
    if (xQueuePeek(xDisplayQueue, &r, pdMS_TO_TICKS(1000)) == pdPASS) {
      renderOLED(r);
    }
    vTaskDelay(pdMS_TO_TICKS(REFRESH_RATE_MS));
  }
}
