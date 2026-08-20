/*
 * ina219.c
 *
 *  Created on: Aug 13, 2026
 *      Author: cdh
 */
#include "ina219.h"

static HAL_StatusTypeDef ina219_write_reg(INA219_t *dev, uint8_t reg, uint16_t value){
	uint8_t buffer[2];
	buffer[0] = (value >> 8) & 0xFF;
	buffer[1] = value & 0xFF;
	return HAL_I2C_Mem_Write(dev->hi2c, dev->addr, reg, I2C_MEMADD_SIZE_8BIT, buffer, 2, 100);
}

static HAL_StatusTypeDef ina219_read_reg(INA219_t *dev, uint8_t reg, uint16_t *value){
	uint8_t buffer[2];
	HAL_StatusTypeDef res = HAL_I2C_Mem_Read(dev->hi2c, dev->addr, reg, I2C_MEMADD_SIZE_8BIT, buffer, 2, 100);
	if(res != HAL_OK){
		return res;
	}
	*value = (buffer[0] << 8) | buffer[1];
	return res;
}

HAL_StatusTypeDef INA219_Init(INA219_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr){
	dev->hi2c = hi2c;
	dev->addr = addr;
	HAL_StatusTypeDef CalRes = INA219_SetCalibration(dev);
	return CalRes;
}

HAL_StatusTypeDef INA219_ReadBusVoltage(INA219_t *dev, float *volt){
	uint16_t raw;
	HAL_StatusTypeDef ResFlag = ina219_read_reg(dev, INA219_BUS_REG, &raw);
	if(ResFlag != HAL_OK){
		return ResFlag;
	}
	*volt = (raw >> 3) * 0.004;
	return ResFlag;
}

HAL_StatusTypeDef INA219_SetCalibration(INA219_t *dev){
	HAL_StatusTypeDef WriteRes = ina219_write_reg(dev, INA219_CAL_REG, INA219_CAL_VALUE);
	return WriteRes;
}

HAL_StatusTypeDef INA219_ReadCurrent(INA219_t *dev, float *current){
	uint16_t raw;
	HAL_StatusTypeDef ReadRes = ina219_read_reg(dev, INA219_CURRENT_REG, &raw);
	if(ReadRes != HAL_OK){
		return ReadRes;
	}
	*current = ((int16_t)raw * INA219_CURRENT_LSB);
	return ReadRes;
}
