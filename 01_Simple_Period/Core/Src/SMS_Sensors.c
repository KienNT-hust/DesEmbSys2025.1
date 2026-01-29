#include "SMS_Sensors.h"
#include <stdio.h>
#include <string.h>

static char msg[64];

SMS_Data_t read_SMS_V1(ADC_HandleTypeDef *hadc) {
    SMS_Data_t sms_data;
    uint32_t sum_adc = 0;
    int samples = 20; // Số lần lấy mẫu

    // --- BƯỚC 1: ĐỌC TRUNG BÌNH (SỬA LỖI) ---
    // Lỗi cũ: Để Start bên ngoài vòng lặp chỉ chạy được nếu bật Continuous Mode.
    // Cách sửa: Đưa Start/Stop vào trong vòng lặp để đảm bảo chạy đúng mọi chế độ.

    for (int i = 0; i < samples; i++) {
        HAL_ADC_Start(hadc); // Kích hoạt ADC

        if (HAL_ADC_PollForConversion(hadc, 10) == HAL_OK) {
            sum_adc += HAL_ADC_GetValue(hadc);
        }

        HAL_ADC_Stop(hadc); // Dừng để reset trạng thái
        // HAL_Delay(1); // Nếu cần delay nhỏ
    }

    // Tính trung bình
    sms_data.adc_raw = sum_adc / samples;

    // --- BƯỚC 2: CALIBRATION (QUAN TRỌNG) ---
    // Kiên chú ý đọc giá trị RAW in ra màn hình để sửa 2 số này nhé
    uint16_t dry_val = 3800; // Giá trị khi đất KHÔ (Đo ngoài không khí)
    uint16_t wet_val = 1200; // Giá trị khi đất ƯỚT (Nhúng vào nước)

    // --- BƯỚC 3: XỬ LÝ SỐ LIỆU ---
    // Kẹp dòng (Clamp)
    if (sms_data.adc_raw > dry_val) sms_data.adc_raw = dry_val;
    if (sms_data.adc_raw < wet_val) sms_data.adc_raw = wet_val;

    // Tính phần trăm: (Càng ẩm -> ADC càng thấp)
    sms_data.moist_pct = 100 - ((sms_data.adc_raw - wet_val) * 100 / (dry_val - wet_val));

    return sms_data;
}

void send_SMS_V1(UART_HandleTypeDef *huart, SMS_Data_t value_send) {
    // Mình thêm hiển thị giá trị RAW để bạn biết đường sửa dry_val/wet_val
    sprintf(msg, "RAW: %d - Do am: %d %%\r\n", value_send.adc_raw, value_send.moist_pct);
    HAL_UART_Transmit(huart, (uint8_t*)msg, strlen(msg), 100);
}
