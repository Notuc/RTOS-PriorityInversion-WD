#include "DisplayTask.hpp"
#include "FreeRTOS.h"
#include "SensorReading.hpp"
#include "app_main.hpp"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>
#include <task.h>

#define WD_BIT_DISPLAY (1 << 1)
#define REFRESH_RATE_MS 250 // 4 Hz

void DisplayTask::run() {
  ssd1306_Init();

  for (;;) {
    SensorReading_t r{};
    ssd1306_Fill(Black);

    // Peek — item stays in queue, no data lost
    if (xQueuePeek(xDisplayQueue, &r, pdMS_TO_TICKS(1000)) == pdPASS) {
      renderOLED(r);
    }
    vTaskDelay(pdMS_TO_TICKS(REFRESH_RATE_MS));
  }
}

void DisplayTask::renderOLED(const SensorReading_t &reading) {
  char buf[32];

  ssd1306_Fill(Black);

  // Line 1 — y=0, top of screen
  ssd1306_SetCursor(0, 0);
  snprintf(buf, sizeof(buf), "Temp: %.1f C", reading.temperature);
  ssd1306_WriteString(buf, Font_6x8, White);

  // Line 2 — y=20, middle
  ssd1306_SetCursor(0, 20);
  snprintf(buf, sizeof(buf), "Pres:%.1f hPa", reading.pressure);
  ssd1306_WriteString(buf, Font_6x8, White);

  // Line 3 — y=40, lower
  ssd1306_SetCursor(0, 40);
  snprintf(buf, sizeof(buf), "Humi: %.1f %%", reading.humidity);
  ssd1306_WriteString(buf, Font_6x8, White);

  // Line 4 — y=54, bottom — small status indicator
  ssd1306_SetCursor(0, 54);
  snprintf(buf, sizeof(buf), "T:%lums", reading.timestamp_ms);
  ssd1306_WriteString(buf, Font_6x8, White);

  ssd1306_UpdateScreen();
}
