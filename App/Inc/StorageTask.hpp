#pragma once
#include "FreeRTOS.h"
#include "SensorReading.hpp"
#include "Task.hpp"
#include "semphr.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

class StorageTask : public Task {
public:
  StorageTask() : Task("Storage", 512, osPriorityNormal) {}

  // Deleted copy and move — FreeRTOS handles must not be copied
  StorageTask(const StorageTask &) = delete;
  StorageTask &operator=(const StorageTask &) = delete;
  StorageTask(StorageTask &&) = delete;
  StorageTask &operator=(StorageTask &&) = delete;

  virtual ~StorageTask() = default;

  // SPI mutex accessor
  SemaphoreHandle_t getSPIMutex() const { return spiMutex_; }

protected:
  void run() override;

private:
  SemaphoreHandle_t spiMutex_ = nullptr;

  // track the last known write position
  uint32_t writeHead_ = 1; // page 0 reserved for config

  // Config struct stored in flash page 0
  struct __attribute__((packed)) FlashConfig_t {
    uint32_t magic;       // 0xC0FFEE01 if valid
    uint32_t writeHead;   // last known write position
    uint8_t padding[248]; // pad to 256 bytes (one full page)
  };

  static constexpr uint32_t CONFIG_MAGIC = 0xC0FFEE01;
  static constexpr uint32_t CONFIG_PAGE = 0;
  static constexpr uint32_t MAX_PAGES = 16384; // W25Q32 = 4MB
  static constexpr uint32_t PAGES_PER_SECTOR = 16;
  static constexpr uint32_t WD_BIT_STORAGE = (1 << 2);

  // Core operations
  void loadConfig();
  void persistConfig();
  bool writePage(const SensorReading_t &r);

  // W25Q32 low level driver calls
  void w25q32_WriteEnable();
  void w25q32_WaitBusy();
  void w25q32_EraseSector(uint32_t pageAddr);
  void w25q32_WritePage(uint32_t pageAddr, const uint8_t *data, uint16_t len);
  void w25q32_ReadPage(uint32_t pageAddr, uint8_t *data, uint16_t len);

  // CS pin helpers — controlled via software NSS
  void csLow();
  void csHigh();

  // Check if spi is working
  bool checkSPI();
};
