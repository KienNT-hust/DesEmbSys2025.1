/*
 * DHT11_Sensors.c
 * Created on: Dec 04, 2025
 * Author: KienNT
 */
#include "main.h"
#include "DHT11_Sensors.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint16_t timeout = 0;
// --- HÀM RIÊNG TƯ (Helper Functions) ---
void DHT11_Init_Timer(void) {
    // 1. Kích hoạt bộ đếm DWT
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    // 2. Reset bộ đếm về 0
    DWT->CYCCNT = 0;
    // 3. Bật bộ đếm
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void Set_Pin_Output(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

static void Set_Pin_Input(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

// --- HÀM CHÍNH ---
DHT11_Data_t DHT11_Read(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    DHT11_Data_t result = {0, 0};
    uint8_t data[5] = {0};

    // --- GỬI START ---
    Set_Pin_Output(GPIOx, GPIO_Pin);
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, 0);
    HAL_Delay(20);
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, 1);
    delay_us(30); // Dùng hàm từ util.h
    Set_Pin_Input(GPIOx, GPIO_Pin);

    // --- CHECK PHẢN HỒI ---
    if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == 1) {
        result.Humidity = -1.0;
        result.Temperature = -1.0;
        return result;
    }

    delay_us(80);
    if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == 0) {
        result.Humidity = -2.0;
        return result;
    }
    delay_us(80);

    // --- ĐỌC DATA ---
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
        	timeout = 0;
        	while (!(HAL_GPIO_ReadPin(GPIOx, GPIO_Pin))) {
        	    timeout++;
        	    if (timeout > 1000) return result; // Thoát nếu kẹt quá lâu
        	    delay_us(1);
        	}
        }
    }

    // --- TÍNH TOÁN ---
    if (data[4] == (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
        result.Humidity = (float)data[0] + (float)data[1] / 10.0;
        result.Temperature = (float)data[2] + (float)data[3] / 10.0;
    }
    return result;
}

void send_DHT11_Data(UART_HandleTypeDef *huart, DHT11_Data_t value_send) {
    char msg[64];
    int temp_int = (int)value_send.Temperature;
    int temp_dec = (int)((value_send.Temperature - temp_int) * 10);
    int hum_int = (int)value_send.Humidity;
    int hum_dec = (int)((value_send.Humidity - hum_int) * 10);

    sprintf(msg, "Temp: %d.%d C - Hum: %d.%d %%\r\n", temp_int, abs(temp_dec), hum_int, abs(hum_dec));
    HAL_UART_Transmit(huart, (uint8_t*)msg, strlen(msg), 100);
}
