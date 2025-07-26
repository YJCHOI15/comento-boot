#ifndef PMIC_DRIVER_H
#define PMIC_DRIVER_H

// I2C slave address (7bit + write bit)
#define PMIC_I2C_ADDR       (0x60 << 1)

#include "main.h"

// mp5475gu register
typedef enum {
    PMIC_REG_FAULT_STATUS1 = 0x07,  // UV/OV condition
    PMIC_REG_FAULT_STATUS2 = 0x08,  // OC condition
} PMIC_Register_t;

// FAULT_STATUS1 (0x07)
typedef union {
    uint8_t all;
    struct {
        uint8_t BUCKD_OV    : 1;    // LSB
        uint8_t BUCKC_OV    : 1;
        uint8_t BUCKB_OV    : 1;
        uint8_t BUCKA_OV    : 1;
        uint8_t BUCKD_UV    : 1;
        uint8_t BUCKC_UV    : 1;
        uint8_t BUCKB_UV    : 1;
        uint8_t BUCKA_UV    : 1;    // MSB
    } bits;
} PMIC_FaultStatus1_t;

// FAULT_STATUS2 (0x08)
typedef union {
    uint8_t all;
    struct {
        uint8_t BUCKD_OC_WARN    : 1;    // LSB
        uint8_t BUCKC_OC_WARN    : 1;  
        uint8_t BUCKB_OC_WARN    : 1;  
        uint8_t BUCKA_OC_WARN    : 1;   
        uint8_t BUCKD_OC         : 1;
        uint8_t BUCKC_OC         : 1;
        uint8_t BUCKB_OC         : 1;
        uint8_t BUCKA_OC         : 1;   // MSB
    } bits;
} PMIC_FaultStatus2_t;

HAL_StatusTypeDef PMIC_ReadFaultStatus_DMA(void);

extern volatile uint8_t pmic_dma_done;
extern PMIC_FaultStatus1_t pmic_uv_status;
extern PMIC_FaultStatus2_t pmic_oc_status;

#endif