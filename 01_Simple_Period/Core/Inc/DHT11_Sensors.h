/*
 * DHT11_Sensors.h
 *
 *  Created on: Dec 25, 2025
 *      Author: Asus ROG
 */

#ifndef INC_DHT11_SENSORS_H_
#define INC_DHT11_SENSORS_H_
#include "main.h"
typedef struct {
	float Temperature;
	float Humidity;
} DHT11_Data_t;

// Hàm khởi tạo bộ đếm thời gian (DWT)
void DHT11_Init_Timer(void);

// Hàm đọc dữ liệu (cần truyền vào Port và Pin đang nối)
DHT11_Data_t DHT11_Read(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
// Hàm khởi tạo bộ đếm thời gian (DWT)
void DHT11_Init_Timer(void);
// TRuyen du lieu
void send_DHT11_Data(UART_HandleTypeDef *huart, DHT11_Data_t value_send);
#endif /* INC_DHT11_SENSORS_H_ */
