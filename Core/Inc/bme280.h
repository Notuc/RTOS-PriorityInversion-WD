/*
 * bme280.h
 *
 * Minimal HAL-based BME280 driver (I2C mode)
 * Compensation formulas per Bosch BME280 datasheet rev 1.6 (BST-BME280-DS002-15)
 */

#ifndef BME280_H
#define BME280_H

#include "main.h"   /* pulls in HAL headers + hi2c1 type */
#include <stdint.h>

#define BME280_I2C_ADDR_PRIMARY    (0x77 << 1)  /* SDO -> VDDIO */
#define BME280_I2C_ADDR_SECONDARY  (0x76 << 1)  /* SDO -> GND   */

/* Register map */
#define BME280_REG_CHIP_ID     0xD0
#define BME280_REG_RESET       0xE0
#define BME280_REG_CTRL_HUM    0xF2
#define BME280_REG_STATUS      0xF3
#define BME280_REG_CTRL_MEAS   0xF4
#define BME280_REG_CONFIG      0xF5
#define BME280_REG_DATA_START  0xF7   /* press_msb .. hum_lsb, 8 bytes burst */
#define BME280_REG_CALIB00     0x88   /* 26 bytes: dig_T1..dig_P9 */
#define BME280_REG_CALIB26     0xE1   /* 7 bytes: dig_H2..dig_H6  */

#define BME280_CHIP_ID_VALUE   0x60

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;

    int32_t  t_fine;

    I2C_HandleTypeDef *hi2c;
    uint16_t addr;      /* already left-shifted, e.g. BME280_I2C_ADDR_PRIMARY */
} BME280_HandleTypeDef;

/* Returns HAL_OK on success. Call after I2C peripheral is initialized. */
HAL_StatusTypeDef BME280_Init(BME280_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c, uint16_t addr);

/* Reads raw registers, applies compensation, returns real-world units. */
HAL_StatusTypeDef BME280_ReadAll(BME280_HandleTypeDef *dev,
                                  float *temperature_degC,
                                  float *pressure_hPa,
                                  float *humidity_rh);

#endif /* BME280_H */
