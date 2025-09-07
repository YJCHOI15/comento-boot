#include "yj_adc.h"

extern ADC_HandleTypeDef hadc1;

float ADC_GetVoltage(void)
{
    uint32_t raw_value = 0;
    float voltage = 0.0f;

    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
        // 변환이 완료될 때까지 대기 (Blocking 방식, 10ms 타임아웃)
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {

            raw_value = HAL_ADC_GetValue(&hadc1);
            voltage = (raw_value / ADC_RESOLUTION) * ADC_VREF;
        }
    }
    HAL_ADC_Stop(&hadc1);

    return voltage;
}