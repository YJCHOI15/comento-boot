#ifndef EEPROM_25LC256_H
#define EEPROM_25LC256_H

#include "main.h"
#include "cmsis_os.h"

/* EEPROM 명령어 정의 (25LC256 데이터시트 참조) */
#define EEPROM_CMD_READ    0x03  // 데이터 읽기
#define EEPROM_CMD_WRITE   0x02  // 데이터 쓰기
#define EEPROM_CMD_WREN    0x06  // 쓰기 활성화
#define EEPROM_CMD_RDSR    0x05  // 상태 레지스터 읽기

/* EEPROM CS 핀 정의 */
#define EEPROM_CS_PORT     GPIOB
#define EEPROM_CS_PIN      GPIO_PIN_0

extern osSemaphoreId_t spi_dma_semaphore;

void EEPROM_Init(void);
HAL_StatusTypeDef EEPROM_WriteBytes_DMA(uint16_t address, uint8_t* pData, uint16_t size);
HAL_StatusTypeDef EEPROM_ReadBytes_DMA(uint16_t address, uint8_t* pData, uint16_t size);


#endif /* BSP_EEPROM_25LC256_H_ */
