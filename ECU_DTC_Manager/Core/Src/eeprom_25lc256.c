#include <string.h>
#include "eeprom_25lc256.h"

extern SPI_HandleTypeDef hspi1;
osSemaphoreId_t spi_dma_semaphore;

static void EEPROM_CS_LOW(void) {
    HAL_GPIO_WritePin(EEPROM_CS_PORT, EEPROM_CS_PIN, GPIO_PIN_RESET);
}

static void EEPROM_CS_HIGH(void) {
    HAL_GPIO_WritePin(EEPROM_CS_PORT, EEPROM_CS_PIN, GPIO_PIN_SET);
}

/* EEPROM에 데이터를 쓰기 전에 
 * 쓰기 활성화 명령을 전송해야 한다. 
 */
static void EEPROM_WriteEnable(void) {
    uint8_t cmd = EEPROM_CMD_WREN;
    EEPROM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    EEPROM_CS_HIGH();
}

/* 
 * EEPROM에 데이터 저장 시간을 고려하여
 * EEPROM의 상태 레지스터를 계속 확인하고
 * 내부 쓰기 작업이 끝나면 다음 명령을 보내기 위함
 */
static void EEPROM_WaitForWriteInProgress(void) {
    uint8_t cmd = EEPROM_CMD_RDSR;
    uint8_t status;
    EEPROM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    do {
        HAL_SPI_Receive(&hspi1, &status, 1, 100);
    } while (status & 0x01); // 상태 레지스터의 WIP(Write-In-Progress) 비트가 0이 될 때까지 대기
    EEPROM_CS_HIGH();
}

void EEPROM_Init(void) {
    // 바이너리 세마포어 생성
    // DMA 콜백 함수에서 Release
    spi_dma_semaphore = osSemaphoreNew(1, 0, NULL);
    if (spi_dma_semaphore == NULL) {
        Error_Handler();
    }
}

HAL_StatusTypeDef EEPROM_Write_DTC(uint16_t address, uint8_t* pData, uint16_t size) {
    EEPROM_WaitForWriteInProgress();
    EEPROM_WriteEnable();

    uint8_t tx_buffer[size + 3];          // 명령어(1) + 데이터 저장할 주소(2) + 데이터(size) 저장
    tx_buffer[0] = EEPROM_CMD_WRITE;
    tx_buffer[1] = (address >> 8) & 0xFF; // 주소 상위 바이트
    tx_buffer[2] = address & 0xFF;        // 주소 하위 바이트
    memcpy(&tx_buffer[3], pData, size);

    EEPROM_CS_LOW();
    if (HAL_SPI_Transmit_DMA(&hspi1, tx_buffer, size + 3) != HAL_OK) {
        EEPROM_CS_HIGH();
        return HAL_ERROR;
    }

    // DMA 완료 신호를 받을 때까지 Blocked
    if (osSemaphoreAcquire(spi_dma_semaphore, 100) != osOK) {
        EEPROM_CS_HIGH();
        return HAL_TIMEOUT;
    }

    EEPROM_CS_HIGH();
    return HAL_OK;
}

HAL_StatusTypeDef EEPROM_Read_DTCs(uint16_t address, uint8_t* pData, uint16_t size) {
    EEPROM_WaitForWriteInProgress();

    uint8_t tx_buffer[3]; // 명령어(1) + 주소(2)
    tx_buffer[0] = EEPROM_CMD_READ;
    tx_buffer[1] = (address >> 8) & 0xFF;
    tx_buffer[2] = address & 0xFF;

    EEPROM_CS_LOW();
    if (HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buffer, pData, size + 3) != HAL_OK) {
        EEPROM_CS_HIGH();
        return HAL_ERROR;
    }

    // DMA 완료 신호를 받을 때까지 Blocked
    if (osSemaphoreAcquire(spi_dma_semaphore, 100) != osOK) {
        EEPROM_CS_HIGH();
        return HAL_TIMEOUT;
    }

    EEPROM_CS_HIGH();
    return HAL_OK;
}