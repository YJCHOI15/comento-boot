#include "dtc_manager.h"

extern SPI_HandleTypeDef hspi1;

volatile uint8_t dtc_write_done = 0;

// CMD + ADDrH + ADDrL + Data(1Page)
static uint8_t tx_buf[sizeof(DTC_Table_t) + 3];

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        dtc_write_done = 1;
        HAL_GPIO_WritePin(EEPROM_CS_GPIO_PORT, GPIO_CS_PIN,  GPIO_PIN_SET);
    }
}

// Send WREN CMD (Polling)
static HAL_StatusTypeDef EEPROM_WriteEnable(void) {
    uint8_t cmd = EEPROM_CMD_WREN;

    HAL_GPIO_WritePin(EEPROM_CS_GPIO_PORT, GPIO_CS_PIN, GPIO_PIN_RESET);  // EEPROM CS LOW
    
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY /* no timeout */);
    
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_PORT, GPIO_CS_PIN, GPIO_PIN_SET);    // EEPROM CS HIGH
   
    return ret;
}

// Request DTC SPI DMA store
HAL_StatusTypeDef DTC_WriteToEEPROM_DMA(uint16_t address, DTC_Table_t *dtc) {
    if (EEPROM_WriteEnable() != HAL_OK) return HAL_ERROR;

    tx_buf[0] = EEPROM_CMD_WRITE;       // CMD
    tx_buf[1] = (address >> 8) & 0xFF;  // AddrH
    tx_buf[2] = (address >> 0) & 0xFF;  // AddrL
    
    memcpy(&tx_buf[3], dtc, sizeof(DTC_Table_t));

    dtc_write_done = 0;

    //Start sending
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_PORT, GPIO_CS_PIN, GPIO_PIN_RESET);  // CS LOW
    return HAL_SPI_Transmit_DMA(&hspi1, tx_buf, sizeof(DTC_Table_t) + 3);
}