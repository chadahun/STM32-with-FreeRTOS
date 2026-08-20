/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "semphr.h"
#include "ina219.h"
#include "ssd1306.h"
#include "queue.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
	float voltage;
	float current;
} SensorData_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static INA219_t ina219;
static SSD1306_t oled;

static TaskHandle_t xBlinkHandle = NULL;
static TaskHandle_t xHeartbeatHandle = NULL;
static TaskHandle_t xButtonHandle = NULL;
static TaskHandle_t xStatsHandle = NULL;
static TaskHandle_t xI2CHandle = NULL;
static TaskHandle_t xSensorHandle = NULL;
static TaskHandle_t xServoHandle = NULL;
static TaskHandle_t xDisplayHandle = NULL;
static TaskHandle_t xControlHandle = NULL;

static SemaphoreHandle_t xBtnSem = NULL;
static SemaphoreHandle_t xI2CMutex = NULL;

static QueueHandle_t xSensorQueue = NULL;

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;

static float g_peak_current = 0, g_volt_at_peak = 0;
static volatile uint8_t g_servo_blocked = 0;

#define CURRENT_LIMIT_A 0.4f
#define OVER_COUNT_LIMIT 10
#define CURRENT_AVG_LIMIT 0.15f
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void vBlinkTask(void *pvParameters);
static void vHeartbeatTask(void *pvParameters);
static void vButtonTask(void *pvParameters);
static void vStatsTask(void *pvParameters);
static void vI2CScanTask(void *pvParameters);
static void vSensorTask(void *pvParameters);
static void vServoTask(void *pvParameters);
static void vDisplayTask(void *pvParameters);
static void vControlTask(void *pvParameters);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	BaseType_t xResult;
	INA219_Init(&ina219, &hi2c1, INA219_ADDR);
	SSD1306_Init(&oled, &hi2c1, SSD1306_ADDR);

	SSD1306_Clear(&oled);
	SSD1306_SetCursor(&oled, 0, 0);
	SSD1306_WriteString(&oled, "HELLO");
	SSD1306_UpdateScreen(&oled);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
	xI2CMutex = xSemaphoreCreateMutex();
	configASSERT(xI2CMutex != NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
	xBtnSem = xSemaphoreCreateBinary();
	configASSERT(xBtnSem != NULL);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	xSensorQueue = xQueueCreate(5, sizeof(SensorData_t));
	configASSERT(xSensorQueue != NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  xResult = xTaskCreate(
		  vBlinkTask,
		  "BLINK",
		  128,
		  NULL,
		  1,
		  &xBlinkHandle
  );
  configASSERT(xResult == pdPASS);

  xResult = xTaskCreate(
		  vHeartbeatTask,
		  "HEARTBEAT",
		  128,
		  NULL,
		  2,
		  &xHeartbeatHandle
  );
  configASSERT(xResult == pdPASS);

  xResult = xTaskCreate(
		  vButtonTask,
		  "BUTTON",
		  128,
		  NULL,
		  3,
		  &xButtonHandle
  );
  configASSERT(xResult == pdPASS);

  xResult = xTaskCreate(
		  vStatsTask,
		  "STAT",
		  256,
		  NULL,
		  1,
		  &xStatsHandle
  );
  configASSERT(xResult == pdPASS);

//  xResult = xTaskCreate(
//		  vI2CScanTask,
//		  "I2C",
//		  256,
//		  NULL,
//		  1,
//		  &xI2CHandle
//  );
//  configASSERT(xResult == pdPASS);

  xResult = xTaskCreate(
		  vSensorTask,
		  "SENSOR",
		  256,
		  NULL,
		  2,
		  &xSensorHandle
  );
  configASSERT(xResult == pdPASS);

  xResult = xTaskCreate(
		  vServoTask,
		  "SERVO",
		  128,
		  NULL,
		  2,
		  &xServoHandle
  );
  configASSERT(xResult == pdPASS);

  xResult = xTaskCreate(
		  vDisplayTask,
		  "DISPLAY",
		  256,
		  NULL,
		  1,
		  &xDisplayHandle
  );
  configASSERT(xResult == pdPASS);

  xResult = xTaskCreate(
		  vControlTask,
		  "CONTROL",
		  256,
		  NULL,
		  4,
		  &xControlHandle
  );
  configASSERT(xResult == pdPASS);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void vBlinkTask(void *pvParameters){
	for(;;){
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

static void vHeartbeatTask(void *pvParameters){
	char tx_buffer[20] = {0};
	strcpy(tx_buffer, "OK\r\n");
	for(;;){
		HAL_UART_Transmit(&huart2, (uint8_t *)tx_buffer, strlen(tx_buffer), 100);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

static void vButtonTask(void *pvParameters){
	char msg[] = "\r\nBUTTON\r\n";
	for(;;){
		if(xSemaphoreTake(xBtnSem, portMAX_DELAY) == pdTRUE){
			HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
			HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
			g_servo_blocked = 0;
		}
	}
}

static void vStatsTask(void *pvParameters){
	char msg[64] = {0};
	for(;;){
		UBaseType_t uxResult1 = uxTaskGetStackHighWaterMark(xBlinkHandle);
		UBaseType_t uxResult2 = uxTaskGetStackHighWaterMark(xButtonHandle);
		UBaseType_t uxResult3 = uxTaskGetStackHighWaterMark(xHeartbeatHandle);
		size_t res = xPortGetFreeHeapSize();
		snprintf(msg, 64, "BLINK: %lu | BTN: %lu | HB: %lu | HEAP: %u\r\n", (unsigned long)uxResult1, (unsigned long)uxResult2, (unsigned long)uxResult3, res);
		HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

//static void vI2CScanTask(void *pvParameters){
//	char msg[32];
//
//	vTaskDelay(pdMS_TO_TICKS(500));
//	for(;;){
//		for(uint8_t addr = 1; addr < 128; addr++){
//				HAL_StatusTypeDef I2CFlag = HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10);
//				if(I2CFlag == HAL_OK){
//					snprintf(msg, sizeof(msg), "Found: 0x%02X\r\n", addr);
//					HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
//				}
//			}
//		vTaskDelay(pdMS_TO_TICKS(5000));
//	}
//
//}

static void vSensorTask(void *pvParameters){
	char msg[96];
	float volt, current;
	HAL_StatusTypeDef VolRes = HAL_OK, CurRes = HAL_OK;
	for(;;){
		g_peak_current = 0, g_volt_at_peak = 0;
		int fail_count = 0;
		for(int i = 0; i < 20; i++){
			if(xSemaphoreTake(xI2CMutex, portMAX_DELAY) == pdTRUE){
				VolRes = INA219_ReadBusVoltage(&ina219, &volt);
				CurRes = INA219_ReadCurrent(&ina219, &current);
				xSemaphoreGive(xI2CMutex);
			}
			if(VolRes == HAL_OK && CurRes == HAL_OK){
				SensorData_t data;
				data.voltage = volt;
				data.current = current;
				xQueueSend(xSensorQueue, &data, 0);
				if(current > g_peak_current){
					g_peak_current = current;
					g_volt_at_peak = volt;
				}
			}else{
				fail_count++;
			}
			vTaskDelay(pdMS_TO_TICKS(50));
		} // peak loop end
		int mv = (int)(g_volt_at_peak * 1000);
		int ua = (int)(g_peak_current * 1000000);
		if(fail_count == 20){
			snprintf(msg, sizeof(msg), "ERROR");
		}else{
			snprintf(msg, sizeof(msg), "V: %d.%03d(V) | I: %d.%01d(mA) | FAIL: %d\r\n", mv / 1000, mv % 1000, ua / 1000, ua % 1000, fail_count);
		}

		HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
		HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 1000);
	} // Task loop end
}

static void vServoTask(void *pvParameters){
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	for(;;){
		if(g_servo_blocked == 0){
			TIM2->CCR1 = 1000;
			vTaskDelay(pdMS_TO_TICKS(1000));
			TIM2->CCR1 = 2000;
		}else{
			TIM2->CCR1 = 1500;
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

static void vDisplayTask(void *pvParameters){
	char line1[24];
	char line2[24];
	for(;;){
		int mv = (int)(g_volt_at_peak * 1000);
		int ua = (int)(g_peak_current * 1000000);
		snprintf(line1, sizeof(line1), "V: %d.%03d(V)", mv / 1000, mv % 1000);
		snprintf(line2, sizeof(line2), "I: %d.%01d(mA)", ua / 1000, ua % 1000);
		SSD1306_Clear(&oled);
		SSD1306_SetCursor(&oled, 0, 0);
		SSD1306_WriteString(&oled, line1);
		SSD1306_SetCursor(&oled, 0, 16);
		SSD1306_WriteString(&oled, line2);
		if(xSemaphoreTake(xI2CMutex, portMAX_DELAY) == pdTRUE){
			SSD1306_UpdateScreen(&oled);
			xSemaphoreGive(xI2CMutex);
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

static void vControlTask(void *pvParameters){
	int count = 0;
	float sum = 0;
	SensorData_t data;
	char msg[64];
	for(;;){
		if(xQueueReceive(xSensorQueue, &data, portMAX_DELAY) == pdPASS){
			sum += data.current;
			count++;
			if(count >= 20){
				float avg = sum / count;
				int avg_ua = (int)(avg * 1000000);
				snprintf(msg, sizeof(msg), "CONTROL AVG: %d.%03d\r\n", avg_ua / 1000, avg_ua % 1000);
				if(avg > CURRENT_AVG_LIMIT && g_servo_blocked != 1){
					HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
					g_servo_blocked = 1;
					snprintf(msg, sizeof(msg), "********* SERVO BLOCKED *********\r\n");
				}
				HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
				sum = 0, count = 0;
			}
		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == GPIO_PIN_13){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xBtnSem, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}


/* USER CODE END Application */

