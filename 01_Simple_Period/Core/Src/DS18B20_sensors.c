/*
 * DS18B20_Sensors.c
 * Created on: Jan 10, 2026
 * Author: Kien Nguyen
 */

#include "DS18B20_Sensors.h"

// Gọi biến htim2 đã khai báo từ main.c
extern TIM_HandleTypeDef htim2;

// --- Tầng 1: Xử lý Delay Micro giây (Sử dụng TIM2) ---
void delay_us(uint32_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

// --- Tầng 2: Giao thức 1-Wire (Low-level) ---

uint8_t DS18B20_Reset(void) {
    uint8_t presence = 0;

    if (!HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) return 0; // Bus hỏng hoặc thiếu trở

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(480);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    delay_us(80);

    if (!HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) presence = 1;
    delay_us(400);
    return presence;
}

void DS18B20_WriteBit(uint8_t bit) {
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    if (bit) {
        delay_us(10); // Ghi bit 1: kéo thấp 10us rồi nhả
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        delay_us(50);
    } else {
        delay_us(60); // Ghi bit 0: kéo thấp hẳn 60us
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        delay_us(10);
    }
}

uint8_t DS18B20_ReadBit(void) {
    uint8_t bit = 0;
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET); // Thả để cảm biến điều khiển bus
    delay_us(10);
    if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) bit = 1;
    delay_us(50);
    return bit;
}

void DS18B20_WriteByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        DS18B20_WriteBit((data >> i) & 1);
    }
}

uint8_t DS18B20_ReadByte(void) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        if (DS18B20_ReadBit()) value |= (1 << i);
    }
    return value;
}

// --- Tầng 3: Ứng dụng DS18B20 ---

uint8_t DS18B20_Init(void) {
    // Chỉ thực hiện Reset để kiểm tra sự có mặt của cảm biến
    return DS18B20_Reset();
}

// Hàm gửi lệnh yêu cầu bắt đầu đo nhiệt độ (Non-blocking)
void DS18B20_StartMeasure(void) {
    if (DS18B20_Reset()) {
        DS18B20_WriteByte(DS18B20_CMD_SKIPROM);
        DS18B20_WriteByte(DS18B20_CMD_CONVERTTEMP); // Lệnh 0x44
    }
}

// Hàm đọc giá trị từ bộ nhớ đệm (Nên gọi sau StartMeasure ít nhất 750ms)
float DS18B20_GetTempResult(void) {
    uint8_t data[2];
    int16_t temp_raw;

    if (!DS18B20_Reset()) return -999.0;

    DS18B20_WriteByte(DS18B20_CMD_SKIPROM);
    DS18B20_WriteByte(DS18B20_CMD_RSCRATCHPAD); // Lệnh 0xBE

    data[0] = DS18B20_ReadByte(); // LSB (Byte thấp)
    data[1] = DS18B20_ReadByte(); // MSB (Byte cao)

    temp_raw = (data[1] << 8) | data[0]; // Hợp nhất 2 byte dữ liệu
    return (float)temp_raw / 16.0; // Giá trị 12-bit có độ phân giải 0.0625 (1/16)
}
