#ifndef DTC_MANAGER_H
#define DTC_MANAGER_H

#include "main.h"
#include <string.h>

#define EEPROM_SPI_ADDR_BYTES   2        // Use 2bytes address
#define EEPROM_PAGE_SIZE        64       // 25LC256's Page Write unit
#define EEPROM_TOTAL_SIZE       32768    // 32KB = 256Kbit
#define EEPROM_BASE_ADDR        0x0000

#define EEPROM_CMD_WREN         0x06     // Write Enable
#define EEPROM_CMD_WRITE        0x02     // Write. cmd + address + data

#define EEPROM_CS_GPIO_PORT     GPIOB
#define GPIO_CS_PIN             GPIO_PIN_2

typedef struct {
    uint16_t DTC_Code;              // 고장 코드 (예: C1234)
    char Description[50];           // 설명 문자열
    uint8_t active;                 // 활성화 상태 플래그
} DTC_Table_t;

HAL_StatusTypeDef DTC_WriteToEEPROM_DMA(uint16_t address, DTC_Table_t *dtc);

extern volatile uint8_t dtc_write_done;

#endif