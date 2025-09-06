#include "pmic_mp5475gu.h"

extern I2C_HandleTypeDef hi2c1;

osSemaphoreId_t i2c_dma_semaphore;

/*
 * PMIC 드라이버 초기화
 * RTOS 세마포어 생성
 */
void PMIC_Init(void) {
    // 바이너리 세마포어 생성
    i2c_dma_semaphore = osSemaphoreNew(1, 0, NULL);
    if (i2c_dma_semaphore == NULL) {
        // 세마포어 생성 실패 처리
        Error_Handler();
    }
}

/**
 * DMA를 사용하여 PMIC의 Fault 관련 레지스터들을 연속으로 read
 */
HAL_StatusTypeDef PMIC_Read_Faults_DMA(uint8_t* pData, uint16_t Size) {
    // HAL_I2C_Mem_Read_DMA 함수를 호출하여 Non-Blocking I2C 읽기 시작
    // FSM_STATE_REG(0x05) 레지스터부터 Size 바이트만큼 read
    if (HAL_I2C_Mem_Read_DMA(&hi2c1, MP5475_I2C_SLAVE_ADDR, FSM_STATE_REG, I2C_MEMADD_SIZE_8BIT, pData, Size) != HAL_OK) {
        return HAL_ERROR;
    }

    // DMA 전송이 완료될 때까지 세마포어 대기 (Blocked)
    if (osSemaphoreAcquire(i2c_dma_semaphore, 100) == osOK) {
        return HAL_OK;
    } else {
        return HAL_TIMEOUT;
    }
}