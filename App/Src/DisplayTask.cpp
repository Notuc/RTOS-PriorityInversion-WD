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
  for (;;) {
    SensorReading_t r{};
    ssd1306_Init();
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

  // Clearing the Screen
  ssd1306_Fill(Black);

  // temperature at top
  ssd1306_SetCursor(0, 0);
  snprintf(buf, sizeof(buf), "Temp: %.1f C", reading.temperature);
  ssd1306_WriteString(buf, Font_7x10, White);

  // Pressure in the middle
  ssd1306_SetCursor(0, 22);
  snprintf(buf, sizeof(buf), "Pres: %.1f C", reading.pressure);
  ssd1306_WriteString(buf, Font_7x10, White);

  // Humidity at Bottom
  ssd1306_SetCursor(0, 44);
  snprintf(buf, sizeof(buf), "Pres: %.1f C", reading.pressure);
  ssd1306_WriteString(buf, Font_7x10, White);

  // Push to screen;
  ssd1306_UpdateScreen();
};
