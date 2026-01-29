/*
 * SMS_Sensors.h
 *
 *  Created on: Dec 23, 2025
 *      Author: Asus ROG
 */

#ifndef INC_SMS_SENSORS_H_
#define INC_SMS_SENSORS_H_

#include <main.h>


typedef struct {
    uint16_t adc_raw;   // Giá trị thô (0-4095)
    uint8_t  moist_pct; // Phần trăm (0-100%)
} SMS_Data_t;

SMS_Data_t read_SMS_V1(ADC_HandleTypeDef *hadc);
void send_SMS_V1(UART_HandleTypeDef *huart, SMS_Data_t value_send);
#endif /* INC_SMS_SENSORS_H_ */
