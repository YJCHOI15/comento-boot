#ifndef YJ_ADC_H
#define YJ_ADC_H

#include "main.h"

#define ADC_VREF                3.3f    // STM32 MCU의 ADC 기준 측정 전압
#define ADC_RESOLUTION          4095.0f // 12비트 ADC의 최대 값 (2^12 - 1)

float ADC_GetVoltage(void);

#endif /* BSP_ADC_H_ */