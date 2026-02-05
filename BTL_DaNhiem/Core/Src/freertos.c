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
#include "ds18b20.h"
#include <stdio.h>
#include <string.h>
#include "i2c-lcd.h"

extern UART_HandleTypeDef huart1;
extern ADC_HandleTypeDef hadc1;

extern volatile uint32_t delayTaskTemp;
extern volatile uint32_t delayTaskSoil;
extern volatile uint32_t delayTaskUart;
extern volatile uint32_t delayTaskLCD;
extern volatile float global_temp;
extern volatile uint16_t global_soil;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId Task_TempHandle;
osThreadId Task_SoilHandle;
osThreadId Task_UartHandle;
osThreadId Task_LCDHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartTaskTemp(void const * argument);
void StartTaskSoil(void const * argument);
void StartTaskUart(void const * argument);
void StartTaskLCD(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Task_Temp */
  osThreadDef(Task_Temp, StartTaskTemp, osPriorityHigh, 0, 512);
  Task_TempHandle = osThreadCreate(osThread(Task_Temp), NULL);

  /* definition and creation of Task_Soil */
  osThreadDef(Task_Soil, StartTaskSoil, osPriorityNormal, 0, 256);
  Task_SoilHandle = osThreadCreate(osThread(Task_Soil), NULL);

  /* definition and creation of Task_Uart */
  osThreadDef(Task_Uart, StartTaskUart, osPriorityBelowNormal, 0, 256);
  Task_UartHandle = osThreadCreate(osThread(Task_Uart), NULL);

  /* definition and creation of Task_LCD */
  osThreadDef(Task_LCD, StartTaskLCD, osPriorityBelowNormal, 0, 256);
  Task_LCDHandle = osThreadCreate(osThread(Task_LCD), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartTaskTemp */
/**
  * @brief  Function implementing the Task_Temp thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskTemp */
void StartTaskTemp(void const * argument)
{
  /* USER CODE BEGIN StartTaskTemp */
	char msg[64];
	  sprintf(msg, "StartTask_Temp\r\n");
	  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	for(;;)
		  {
		    float temp_val = DS18B20_ReadTemp();
		    if (temp_val > -90.0)
		    {
		        global_temp = temp_val;
		    }
		    else
		    {
		        DS18B20_Init(GPIOA, GPIO_PIN_1);
		    }
		    sprintf(msg, "Task_Temp: Loop\r\n");
		    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
		    osDelay(delayTaskTemp);
		  }
  /* USER CODE END StartTaskTemp */
}

/* USER CODE BEGIN Header_StartTaskSoil */
/**
* @brief Function implementing the Task_Soil thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskSoil */
void StartTaskSoil(void const * argument)
{
  /* USER CODE BEGIN StartTaskSoil */
	char msg[64];
	  sprintf(msg, "StartTask_Soil\r\n");
	  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	  /* Infinite loop */
	  for(;;)
	  {
	    HAL_ADC_Start(&hadc1);
	    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
	    {
	        uint32_t raw = HAL_ADC_GetValue(&hadc1);
	        // Quy đổi ra % (ADC 12-bit: 0-4095)
	        // Công thức: 100 - (raw * 100 / 4095)
	        int soil_percent = 100 - (raw * 100 / 4095);
	        if(soil_percent > 100) soil_percent = 100;
	        if(soil_percent < 0) soil_percent = 0;
	        global_soil = (uint16_t)soil_percent;
	    }
	    HAL_ADC_Stop(&hadc1);
	    sprintf(msg, "Task_Soil: Loop\r\n");
	        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	    osDelay(delayTaskSoil);
	  }
  /* USER CODE END StartTaskSoil */
}

/* USER CODE BEGIN Header_StartTaskUart */
/**
* @brief Function implementing the Task_Uart thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskUart */
void StartTaskUart(void const * argument)
{
  /* USER CODE BEGIN StartTaskUart */
	char msg[100];
	sprintf(msg, "StartTask_UART\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	  int t_int, t_dec;

	  for(;;)
	  {
	    t_int = (int)global_temp;
	    t_dec = (int)((global_temp - t_int) * 10);
	    if(t_dec < 0) t_dec = -t_dec;
	    sprintf(msg, "Temp: %d.%d | Soil: %d\r\n", t_int, t_dec, global_soil);
	    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	    sprintf(msg, "Task_UART: Loop\r\n");
	    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	    osDelay(delayTaskUart);
	  }
  /* USER CODE END StartTaskUart */
}

/* USER CODE BEGIN Header_StartTaskLCD */
/**
* @brief Function implementing the Task_LCD thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskLCD */
void StartTaskLCD(void const * argument)
{
  /* USER CODE BEGIN StartTaskLCD */
  /* Infinite loop */
  for(;;)
  {
	    char lcd_buffer[50];
	    char msg[64];
	    int t_int, t_dec;
	    sprintf(msg, "StartTask_LCD\r\n");
	    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	    /* Infinite loop */
	    for(;;)
	    {
	      t_int = (int)global_temp;
	      t_dec = (int)((global_temp - t_int) * 10);
	      if(t_dec < 0) t_dec = -t_dec;
	      lcd_put_cur(0, 0);
	      sprintf(lcd_buffer, "Temp: %d.%d C   ", t_int, t_dec);
	      lcd_send_string(lcd_buffer);
	      lcd_put_cur(1, 0);
	      sprintf(lcd_buffer, "Soil: %d %%     ", global_soil);
	      lcd_send_string(lcd_buffer);
	      sprintf(msg, "Task_LCD: Loop\r\n");
	      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	      osDelay(delayTaskLCD);
	    }
}
  /* USER CODE END StartTaskLCD */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

