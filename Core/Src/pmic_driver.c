/*
* 목적: PMIC UV/OC 레지스터 (0x07, 0x08) 상태 확인을 위한 DMA 기반 I2C read 구현
* 사용 API: HAL_I2C_Mem_Read_DMA()
* 주의사항: 2단계 읽기 → 콜백으로 연결, busy 상태 고려 필요
* 구현 파일: pmic_driver.c/h, main.c (현재는 Task가 main.c에 있음)
* 다음 단계: 상태 해석 후 SPI EEPROM에 DTC 저장 연동
*/

#include "pmic_driver.h"

extern I2C_HandleTypeDef hi2c1;

PMIC_FaultStatus1_t pmic_uv_status;
PMIC_FaultStatus2_t pmic_oc_status;
volatile uint8_t pmic_dma_done = 0;

HAL_StatusTypeDef PMIC_ReadFaultStatus_DMA(void) {
    HAL_StatusTypeDef ret;

    // Initialize DMA completion flag
    pmic_dma_done = 0;

    // Read 0x07 register (UV/OV status)
    ret = HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR,
                                PMIC_REG_FAULT_STATUS1,
                                I2C_MEMADD_SIZE_8BIT,
                                &pmic_uv_status.all,
                                1);

    // DMA is asynchronous and does not gaurantee sequential execution
    // -> After UV status is read, OC status is read in the callback
    return ret;
}

// DMA receive complete callback
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR,
                                PMIC_REG_FAULT_STATUS2,
                                I2C_MEMADD_SIZE_8BIT,
                                &pmic_oc_status.all,
                                1);

        pmic_dma_done = 1;
    }
}