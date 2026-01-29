/*
 * DS18B20_Sensors.h
 * Created on: Jan 10, 2026
 * Author: Kien Nguyen
 */

#ifndef INC_DS18B20_SENSORS_H_
#define INC_DS18B20_SENSORS_H_

#include "main.h"

// --- Định nghĩa mã lệnh DS18B20 ---
#define DS18B20_CMD_CONVERTTEMP       0x44
#define DS18B20_CMD_RSCRATCHPAD       0xBE
#define DS18B20_CMD_SKIPROM           0xCC

// --- Cấu hình chân kết nối (Kiên có thể sửa ở đây) ---
#define DS18B20_PORT  GPIOA
#define DS18B20_PIN   GPIO_PIN_1

// --- Khai báo các hàm ---

uint8_t DS18B20_Init(void);
float DS18B20_ReadTemp(void);
void DS18B20_StartMeasure(void);
float DS18B20_GetTempResult(void);
void delay_us(uint32_t us);
#endif /* INC_DS18B20_SENSORS_H_ */
