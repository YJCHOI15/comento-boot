#include "pmic_driver.h"

extern I2C_HandleTypeDef hi2c1;

PMIC_FaultStatus1_t pmic_uv_status;
PMIC_FaultStatus2_t pmic_oc_status;
volatile uint8_t pmic_dma_done = 0;

HAL_StatusTypeDef PMIC_RequestFaultStatus_DMA(void) {
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