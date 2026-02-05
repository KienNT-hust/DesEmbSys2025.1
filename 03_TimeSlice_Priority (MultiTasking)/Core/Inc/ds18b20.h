#ifndef INC_DS18B20_H_
#define INC_DS18B20_H_

#include "stm32f1xx_hal.h"
void DS18B20_Init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
float DS18B20_ReadTemp(void);

#endif /* INC_DS18B20_H_ */
