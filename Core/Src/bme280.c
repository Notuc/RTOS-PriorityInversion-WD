/*
 * bme280.c
 *
 * Minimal HAL-based BME280 driver (I2C mode)
 * Compensation formulas per Bosch BME280 datasheet rev 1.6 (BST-BME280-DS002-15)
 */

#include "bme280.h"

/* ---- internal: pack calibration bytes into the struct (Table 16) ---- */
static void BME280_ParseCalib(BME280_HandleTypeDef *dev, uint8_t *c1, uint8_t *c2)
{
    /* c1 = 26 bytes from 0x88..0xA1, c2 = 7 bytes from 0xE1..0xE7 */
    dev->dig_T1 = (uint16_t)(c1[0] | (c1[1] << 8));
    dev->dig_T2 = (int16_t)(c1[2] | (c1[3] << 8));
    dev->dig_T3 = (int16_t)(c1[4] | (c1[5] << 8));

    dev->dig_P1 = (uint16_t)(c1[6]  | (c1[7]  << 8));
    dev->dig_P2 = (int16_t)(c1[8]  | (c1[9]  << 8));
    dev->dig_P3 = (int16_t)(c1[10] | (c1[11] << 8));
    dev->dig_P4 = (int16_t)(c1[12] | (c1[13] << 8));
    dev->dig_P5 = (int16_t)(c1[14] | (c1[15] << 8));
    dev->dig_P6 = (int16_t)(c1[16] | (c1[17] << 8));
    dev->dig_P7 = (int16_t)(c1[18] | (c1[19] << 8));
    dev->dig_P8 = (int16_t)(c1[20] | (c1[21] << 8));
    dev->dig_P9 = (int16_t)(c1[22] | (c1[23] << 8));

    dev->dig_H1 = c1[25]; /* 0xA1 */

    dev->dig_H2 = (int16_t)(c2[0] | (c2[1] << 8));   /* 0xE1/0xE2 */
    dev->dig_H3 = c2[2];                              /* 0xE3 */
    /* dig_H4[11:4] = c2[3], dig_H4[3:0] = low nibble of c2[4] */
    dev->dig_H4 = (int16_t)((c2[3] << 4) | (c2[4] & 0x0F));
    /* dig_H5[3:0] = high nibble of c2[4], dig_H5[11:4] = c2[5] */
    dev->dig_H5 = (int16_t)((c2[5] << 4) | (c2[4] >> 4));
    dev->dig_H6 = (int8_t)c2[6];                       /* 0xE7 */
}

HAL_StatusTypeDef BME280_Init(BME280_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c, uint16_t addr)
{
    HAL_StatusTypeDef status;
    uint8_t chip_id = 0;
    uint8_t data;
    uint8_t calib1[26];
    uint8_t calib2[7];

    dev->hi2c = hi2c;
    dev->addr = addr;

    /* 1. Confirm we can actually talk to the sensor */
    status = HAL_I2C_Mem_Read(dev->hi2c, dev->addr, BME280_REG_CHIP_ID, 1, &chip_id, 1, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;
    if (chip_id != BME280_CHIP_ID_VALUE) return HAL_ERROR;

    /* 2. Read calibration data once */
    status = HAL_I2C_Mem_Read(dev->hi2c, dev->addr, BME280_REG_CALIB00, 1, calib1, 26, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;
    status = HAL_I2C_Mem_Read(dev->hi2c, dev->addr, BME280_REG_CALIB26, 1, calib2, 7, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;
    BME280_ParseCalib(dev, calib1, calib2);

    /* 3. ctrl_hum MUST be written before ctrl_meas for it to take effect */
    data = 0x01; /* osrs_h x1 */
    status = HAL_I2C_Mem_Write(dev->hi2c, dev->addr, BME280_REG_CTRL_HUM, 1, &data, 1, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;

    /* 4. ctrl_meas: osrs_t x1, osrs_p x1, mode = normal (11) */
    data = (0b001 << 5) | (0b001 << 2) | 0b11;
    status = HAL_I2C_Mem_Write(dev->hi2c, dev->addr, BME280_REG_CTRL_MEAS, 1, &data, 1, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;

    return HAL_OK;
}

HAL_StatusTypeDef BME280_ReadAll(BME280_HandleTypeDef *dev,
                                  float *temperature_degC,
                                  float *pressure_hPa,
                                  float *humidity_rh)
{
    uint8_t raw[8];
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(dev->hi2c, dev->addr, BME280_REG_DATA_START, 1, raw, 8, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8)  |  raw[7];

    /* ---- temperature (must run first: sets t_fine) ---- */
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)dev->dig_T1 << 1))) * ((int32_t)dev->dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dev->dig_T1)) * ((adc_T >> 4) - ((int32_t)dev->dig_T1))) >> 12) *
            ((int32_t)dev->dig_T3)) >> 14;
    dev->t_fine = var1 + var2;
    T = (dev->t_fine * 5 + 128) >> 8;
    *temperature_degC = (float)T / 100.0f;

    /* ---- pressure (64-bit path) ---- */
    int64_t p_var1, p_var2, p;
    p_var1 = ((int64_t)dev->t_fine) - 128000;
    p_var2 = p_var1 * p_var1 * (int64_t)dev->dig_P6;
    p_var2 = p_var2 + ((p_var1 * (int64_t)dev->dig_P5) << 17);
    p_var2 = p_var2 + (((int64_t)dev->dig_P4) << 35);
    p_var1 = ((p_var1 * p_var1 * (int64_t)dev->dig_P3) >> 8) + ((p_var1 * (int64_t)dev->dig_P2) << 12);
    p_var1 = (((((int64_t)1) << 47) + p_var1)) * ((int64_t)dev->dig_P1) >> 33;
    if (p_var1 == 0)
    {
        *pressure_hPa = 0.0f; /* avoid divide-by-zero */
    }
    else
    {
        p = 1048576 - adc_P;
        p = (((p << 31) - p_var2) * 3125) / p_var1;
        p_var1 = (((int64_t)dev->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
        p_var2 = (((int64_t)dev->dig_P8) * p) >> 19;
        p = ((p + p_var1 + p_var2) >> 8) + (((int64_t)dev->dig_P7) << 4);
        *pressure_hPa = ((float)p / 256.0f) / 100.0f; /* Q24.8 Pa -> hPa */
    }

    /* ---- humidity ---- */
    int32_t v_x1_u32r;
    v_x1_u32r = (dev->t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dev->dig_H4) << 20) - (((int32_t)dev->dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)dev->dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)dev->dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)dev->dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dev->dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    uint32_t H = (uint32_t)(v_x1_u32r >> 12);
    *humidity_rh = (float)H / 1024.0f;

    return HAL_OK;
}
