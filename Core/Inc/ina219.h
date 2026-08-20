/*
 * ina219.h
 *
 *  Created on: Aug 13, 2026
 *      Author: cdh
 */
#ifndef INA219_H
#define INA219_H

#include "stm32f4xx_hal.h"

#define INA219_ADDR			(0x40 << 1)
#define INA219_CONF_REG			0x00
#define INA219_SHUNT_REG		0x01
#define INA219_BUS_REG			0x02
#define INA219_POWER_REG		0x03
#define INA219_CURRENT_REG		0x04
#define INA219_CAL_REG			0x05

#define INA219_CAL_VALUE		4096
#define INA219_CURRENT_LSB		0.0001f


typedef struct{
	I2C_HandleTypeDef *hi2c;
	uint8_t addr;
} INA219_t;

HAL_StatusTypeDef INA219_SetCalibration(INA219_t *dev);

HAL_StatusTypeDef INA219_Init(INA219_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr);
HAL_StatusTypeDef INA219_ReadBusVoltage(INA219_t *dev, float *volt);
HAL_StatusTypeDef INA219_ReadCurrent(INA219_t *dev, float *current);

#endif
