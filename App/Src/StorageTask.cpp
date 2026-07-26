#include "StorageTask.hpp"
#include "app_main.hpp"
#include "event_groups.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "task.h"
#include "watchdog_bits.hpp"
#include <stdio.h>
#include <string.h>

// Task run() at the bottom

extern SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart2;

// CS pin
#define W25Q32_CS_PORT Chip_Select_GPIO_Port
#define W25Q32_CS_PIN Chip_Select_Pin

// W25Q32 command bytes from datasheet
#define CMD_WRITE_ENABLE 0x06
#define CMD_PAGE_PROGRAM 0x02
#define CMD_READ_DATA 0x03
#define CMD_SECTOR_ERASE 0x20
#define CMD_READ_STATUS 0x05
#define STATUS_BUSY_BIT 0x01

// CS control
void StorageTask::csLow() {
  HAL_GPIO_WritePin(W25Q32_CS_PORT, W25Q32_CS_PIN, GPIO_PIN_RESET);
}

void StorageTask::csHigh() {
  HAL_GPIO_WritePin(W25Q32_CS_PORT, W25Q32_CS_PIN, GPIO_PIN_SET);
}

// W25Q32  operations
// NOR flash chips require an explicit write. By default it ignores any write or
// erase command. Enable command before any operation that modifies flash
// contents 0x06

void StorageTask::w25q32_WriteEnable() {
  uint8_t cmd = CMD_WRITE_ENABLE;
  csLow();
  HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
  csHigh();
}

// Setting the Busy pit into the status register
// write takes up to 3ms, sectore erase 400ms. During this time the chip is busy
// and ignores commands. The status register is the only thing it responds to
// while busy. polls that status register in a loop until the bit clear

void StorageTask::w25q32_WaitBusy() {
  uint8_t cmd = CMD_READ_STATUS;
  uint8_t status = 0;
  do {
    csLow();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, &status, 1, HAL_MAX_DELAY);
    csHigh();
    if (status & STATUS_BUSY_BIT) {
      vTaskDelay(pdMS_TO_TICKS(1)); // yield while chip is busy
    }
  } while (status & STATUS_BUSY_BIT);
}

// Cannot erase pages only sectore for nor W25Q32 chips
// For a NOR flash you can only write by changing bit from 1 to 0;
// So erase command rests an entier sector (16 pages, 4096bytes)
// back to all 0xFF every bit becomes 1 again, ready for new writes.
//
void StorageTask::w25q32_EraseSector(uint32_t pageAddr) {
  // Convert page address to byte address
  uint32_t byteAddr = pageAddr * 256;

  uint8_t cmd[4] = {CMD_SECTOR_ERASE, (uint8_t)(byteAddr >> 16),
                    (uint8_t)(byteAddr >> 8), (uint8_t)(byteAddr)};

  w25q32_WriteEnable();
  csLow();
  HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);
  csHigh();
  w25q32_WaitBusy(); // sector erase takes up to 400ms
}

// Has to be Enable first, then writes
// pulls cs low, send page cmd,24bit addres, send data bytes, pull cs high,
// and waits for write to complete

void StorageTask::w25q32_WritePage(uint32_t pageAddr, const uint8_t *data,
                                   uint16_t len) {
  uint32_t byteAddr = pageAddr * 256;

  uint8_t cmd[4] = {CMD_PAGE_PROGRAM, (uint8_t)(byteAddr >> 16),
                    (uint8_t)(byteAddr >> 8), (uint8_t)(byteAddr)};

  w25q32_WriteEnable();
  csLow();
  HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);
  HAL_SPI_Transmit(&hspi2, (uint8_t *)data, len, HAL_MAX_DELAY);
  csHigh();
  w25q32_WaitBusy(); // page write takes up to 3ms
}
// Reads are non-destruve so its much easier.
// Complete in the time its takes to clock bytes out.
// the chip whill stream data continoulsy as long as the cs stays
// low and the clock keeps toggling.
void StorageTask::w25q32_ReadPage(uint32_t pageAddr, uint8_t *data,
                                  uint16_t len) {
  uint32_t byteAddr = pageAddr * 256;

  uint8_t cmd[4] = {CMD_READ_DATA, (uint8_t)(byteAddr >> 16),
                    (uint8_t)(byteAddr >> 8), (uint8_t)(byteAddr)};

  csLow();
  HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);
  HAL_SPI_Receive(&hspi2, data, len, HAL_MAX_DELAY);
  csHigh();
  // Read has no busy wait — it completes immediately
}

// Config persistence
// Called at the start of run()
// Reads page 0 into a FlashConfig_t and checks the magic number.
// Then writeHead_ goes back to where logging stopped before the last reboot
void StorageTask::loadConfig() {
  FlashConfig_t cfg{};
  w25q32_ReadPage(CONFIG_PAGE, (uint8_t *)&cfg, sizeof(cfg));

  if (cfg.magic == CONFIG_MAGIC) {
    writeHead_ = cfg.writeHead;
    // Sanity check — corrupt value could send us out of bounds
    if (writeHead_ < 1 || writeHead_ >= MAX_PAGES) {
      writeHead_ = 1;
    }
  } else {
    // Uninitialised flash — start fresh
    writeHead_ = 1;
    persistConfig();
  }
}
// Erasing the Config Sector, becasue of overwritting an already writing page
// Erasing the whole sectore 0-15, then rewriting page 0. Also cost 400ms
void StorageTask::persistConfig() {
  FlashConfig_t cfg{};
  cfg.magic = CONFIG_MAGIC;
  cfg.writeHead = writeHead_;

  // Config sector must be erased before writing
  w25q32_EraseSector(CONFIG_PAGE);
  w25q32_WritePage(CONFIG_PAGE, (uint8_t *)&cfg, sizeof(cfg));
}

// Core write operation

bool StorageTask::writePage(const SensorReading_t &r) {
  // Erase sector if we're entering a new one
  if (writeHead_ % PAGES_PER_SECTOR == 0) {
    // Every 16 pages the storage task calls w25q32_EraseSector
    // which takes up to 400ms inside w25q32_WaitBusy. Beacause of this
    // the storage task misses the watchdog check-in.
    // so now were erasing the next sector ahead of time, right after
    // writing the first page of current sector so by the time we need it
    // the erase is already done.
    // This huart2 implementation is to check to see witch page triggers the
    // erase.
    uint8_t msg[64];
    snprintf((char *)msg, sizeof(msg),
             "[Storage] erasing sector %lu (page %lu)\r\n",
             writeHead_ / PAGES_PER_SECTOR, writeHead_);
    HAL_UART_Transmit(&huart2, msg, strlen((char *)msg), HAL_MAX_DELAY);
    w25q32_EraseSector(writeHead_);
  }

  // Write the reading — pad to 256 bytes
  uint8_t pageBuf[256] = {};
  memcpy(pageBuf, &r, sizeof(SensorReading_t));
  w25q32_WritePage(writeHead_, pageBuf, sizeof(pageBuf));

  // Log the write over UART so you can verify in the terminal
  uint8_t buf[64];
  snprintf((char *)buf, sizeof(buf),
           "[Storage] page=%lu T=%.1f P=%.1f H=%.1f\r\n", writeHead_,
           r.temperature, r.pressure, r.humidity);
  HAL_UART_Transmit(&huart2, buf, strlen((char *)buf), HAL_MAX_DELAY);

  // Advance and wrap — never touch page 0
  writeHead_++;
  if (writeHead_ >= MAX_PAGES) {
    writeHead_ = 1;
  }

  return true;
}

// Connectivity Check
// Checking SPI, 0x9F is a read only command, its asks the chip to identify
// itself. Three response bytes EF 40 16. Theses are set at the manufacture and
// never change.

bool StorageTask::checkSPI() {
  uint8_t cmd = 0x9F;
  uint8_t id[3] = {0};
  uint8_t buf[64];

  csLow();
  HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
  HAL_SPI_Receive(&hspi2, id, 3, HAL_MAX_DELAY);
  csHigh();

  snprintf((char *)buf, sizeof(buf), "[SPI] JEDEC ID: %02X %02X %02X\r\n",
           id[0], id[1], id[2]);
  HAL_UART_Transmit(&huart2, buf, strlen((char *)buf), HAL_MAX_DELAY);

  // W25Q32 always returns EF 40 16
  bool ok = (id[0] == 0xEF && id[1] == 0x40 && id[2] == 0x16);

  uint8_t result[32];
  snprintf((char *)result, sizeof(result), "[SPI] %s\r\n",
           ok ? "OK" : "FAIL — check wiring");
  HAL_UART_Transmit(&huart2, result, strlen((char *)result), HAL_MAX_DELAY);

  return ok;
}

// Task entry point
void StorageTask::run() {
  // Initialise mutex
  spiMutex_ = xSemaphoreCreateMutex();
  configASSERT(spiMutex_);

  // Verify SPI before doing anything else
  if (!checkSPI()) {
    // SPI is broken — trap here so you know immediately
    // rather than getting mysterious flash errors later
    uint8_t msg[] = "[Storage] halting — SPI failed\r\n";
    HAL_UART_Transmit(&huart2, msg, strlen((char *)msg), HAL_MAX_DELAY);
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  // Restore write position from flash
  loadConfig();

  for (;;) {
    SensorReading_t r{};

    // Block until sensor task posts a reading
    if (xQueueReceive(xStorageQueue, &r, portMAX_DELAY) == pdPASS) {

      // Take SPI mutex — blocks if another SPI device is using the bus
      xSemaphoreTake(spiMutex_, portMAX_DELAY);
      bool ok = writePage(r);
      xSemaphoreGive(spiMutex_);

      if (ok) {
        // Persist write head every write so reboot resumes correctly
        xSemaphoreTake(spiMutex_, portMAX_DELAY);
        persistConfig();
        xEventGroupSetBits(xWDGroup, WD_BIT_STORAGE);
        xSemaphoreGive(spiMutex_);
      }
    }
  }
}
