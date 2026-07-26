#include "WatchdogTask.hpp"
#include "app_main.hpp"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;

static constexpr uint32_t WD_TIMEOUT_MS = 6000;

void WatchdogTask::logMissedTasks(EventBits_t bits) {
  uint8_t buf[128];
  snprintf((char *)buf, sizeof(buf), "[WDT] TIMEOUT — missed bits: 0x%02lX\r\n",
           (uint32_t)(~bits & WD_ALL_BITS));
  HAL_UART_Transmit(&huart2, buf, strlen((char *)buf), HAL_MAX_DELAY);

  // Log which specific task missed
  if (!(bits & WD_BIT_SENSOR)) {
    uint8_t m[] = "[WDT] SensorTask hung\r\n";
    HAL_UART_Transmit(&huart2, m, strlen((char *)m), HAL_MAX_DELAY);
  }
  if (!(bits & WD_BIT_DISPLAY)) {
    uint8_t m[] = "[WDT] DisplayTask hung\r\n";
    HAL_UART_Transmit(&huart2, m, strlen((char *)m), HAL_MAX_DELAY);
  }
  if (!(bits & WD_BIT_STORAGE)) {
    uint8_t m[] = "[WDT] StorageTask hung\r\n";
    HAL_UART_Transmit(&huart2, m, strlen((char *)m), HAL_MAX_DELAY);
  }
  if (!(bits & WD_BIT_COMMS)) {
    uint8_t m[] = "[WDT] CommsTask hung\r\n";
    HAL_UART_Transmit(&huart2, m, strlen((char *)m), HAL_MAX_DELAY);
  }
}

void WatchdogTask::run() {
  uint8_t start[] = "[WDT] Watchdog started\r\n";
  HAL_UART_Transmit(&huart2, start, strlen((char *)start), HAL_MAX_DELAY);

  for (;;) {
    EventBits_t bits = xEventGroupWaitBits(xWDGroup, WD_ALL_BITS,
                                           pdTRUE, // clear bits on exit
                                           pdTRUE, // wait for ALL bits
                                           pdMS_TO_TICKS(WD_TIMEOUT_MS));

    if ((bits & WD_ALL_BITS) == WD_ALL_BITS) {
      // All tasks checked in — healthy
      uint8_t ok[] = "[WDT] all tasks OK\r\n";
      HAL_UART_Transmit(&huart2, ok, strlen((char *)ok), HAL_MAX_DELAY);
    } else {
      // At least one task missed its check-in
      logMissedTasks(bits);

      // Flush UART before reset so the log makes it out
      vTaskDelay(pdMS_TO_TICKS(50));
      NVIC_SystemReset();
    }
  }
}
