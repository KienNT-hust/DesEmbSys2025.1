#include "ds18b20.h"

static GPIO_TypeDef* DS_PORT;
static uint16_t DS_PIN;
void DWT_Delay_Init(void) {
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}
void DWT_Delay_us(volatile uint32_t microseconds) {
    uint32_t clk_cycle_start = DWT->CYCCNT;
    microseconds *= (HAL_RCC_GetHCLKFreq() / 1000000);
    while ((DWT->CYCCNT - clk_cycle_start) < microseconds);
}
static void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}
static void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}
void DS18B20_Init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
	DS_PORT = GPIOx;
	DS_PIN = GPIO_Pin;
	DWT_Delay_Init();
}
uint8_t DS18B20_Start(void) {
	uint8_t Response = 0;
	Set_Pin_Output(DS_PORT, DS_PIN);
	HAL_GPIO_WritePin(DS_PORT, DS_PIN, 0);
	DWT_Delay_us(480);
	Set_Pin_Input(DS_PORT, DS_PIN);
	DWT_Delay_us(80);
	if (!(HAL_GPIO_ReadPin(DS_PORT, DS_PIN))) Response = 1;
	else Response = 0;
	DWT_Delay_us(400);
	return Response;
}

void DS18B20_Write(uint8_t data) {
	Set_Pin_Output(DS_PORT, DS_PIN);
	for (int i = 0; i < 8; i++) {
		if ((data & (1 << i)) != 0) {
			Set_Pin_Output(DS_PORT, DS_PIN);
			HAL_GPIO_WritePin(DS_PORT, DS_PIN, 0);
			DWT_Delay_us(1);
			Set_Pin_Input(DS_PORT, DS_PIN);
			DWT_Delay_us(60);
		} else {
			Set_Pin_Output(DS_PORT, DS_PIN);
			HAL_GPIO_WritePin(DS_PORT, DS_PIN, 0);
			DWT_Delay_us(60);
			Set_Pin_Input(DS_PORT, DS_PIN);
		}
	}
}

uint8_t DS18B20_Read(void) {
	uint8_t value = 0;
	Set_Pin_Input(DS_PORT, DS_PIN);
	for (int i = 0; i < 8; i++) {
		Set_Pin_Output(DS_PORT, DS_PIN);
		HAL_GPIO_WritePin(DS_PORT, DS_PIN, 0);
		DWT_Delay_us(2);
		Set_Pin_Input(DS_PORT, DS_PIN);
		if (HAL_GPIO_ReadPin(DS_PORT, DS_PIN)) {
			value |= 1 << i;
		}
		DWT_Delay_us(60);
	}
	return value;
}
float DS18B20_ReadTemp(void) {
	uint8_t Temp_byte1, Temp_byte2;
	uint16_t TEMP;
	float Temperature = 0;

	if(DS18B20_Start()) {
		HAL_Delay(1);
		DS18B20_Write(0xCC);
		DS18B20_Write(0x44);
		HAL_Delay(750);

		DS18B20_Start();
		HAL_Delay(1);
		DS18B20_Write(0xCC);
		DS18B20_Write(0xBE);

		Temp_byte1 = DS18B20_Read();
		Temp_byte2 = DS18B20_Read();
		TEMP = (Temp_byte2 << 8) | Temp_byte1;
		Temperature = (float)TEMP / 16.0;
		return Temperature;
	}
	return -99.0;
}
