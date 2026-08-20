/*
 * ssd1306.h
 *
 *  Created on: Aug 16, 2026
 *      Author: cdh
 */

#ifndef INC_SSD1306_H_
#define INC_SSD1306_H_

#include "stm32f4xx_hal.h"

#define SSD1306_ADDR		(0x3c << 1)
#define SSD1306_CTRL_CMD	0x00
#define SSD1306_CTRL_DATA	0x40

#define SSD1306_WIDTH		128
#define SSD1306_HEIGHT		64
#define SSD1306_SIZE		(SSD1306_WIDTH * SSD1306_HEIGHT / 8)

typedef struct{
	I2C_HandleTypeDef *hi2c;
	uint8_t addr;
	uint8_t buffer[SSD1306_SIZE];
	uint8_t cursor_x;
	uint8_t cursor_y;
} SSD1306_t;

HAL_StatusTypeDef SSD1306_Init(SSD1306_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr);
HAL_StatusTypeDef SSD1306_UpdateScreen(SSD1306_t *dev);
void SSD1306_Clear(SSD1306_t *dev);
void SSD1306_SetCursor(SSD1306_t *dev, uint8_t x, uint8_t y);
void SSD1306_WriteChar(SSD1306_t *dev, char ch);
void SSD1306_WriteString(SSD1306_t *dev, const char *str);

#endif /* INC_SSD1306_H_ */
