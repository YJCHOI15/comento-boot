#ifndef PMIC_MP5475GU_H
#define PMIC_MP5475GU_H

#include <stdint.h>

#define MP5475_I2C_SLAVE_ADDR   0x60

typedef enum {
    FSM_STATE_REG                   = 0x05, // FSM 상태 및 Power Good 상태
    BUCK_PG_STATUS_REG              = 0x06, // Buckx의 Power Good 상태
    BUCK_UV_OV_STATUS_REG           = 0x07, // 각 Buck의 UV, OV 상태
    BUCK_OC_STATUS_REG              = 0x08, // 각 Buck의 OC, OC WARNING 상태
    SYSTEM_FAULT_STATUS_REG         = 0x09, // 시스템 전체 Fault 상태
} MP5475_StatusRegisters;

typedef union {
    uint8_t raw; 
    struct {
        // LSB부터 순서대로 정의
        uint8_t buckd_ov : 1; // Buck D OV 상태
        uint8_t buckc_ov : 1; // Buck C OV 상태
        uint8_t buckb_ov : 1; // Buck B OV 상태
        uint8_t bucka_ov : 1; // Buck A OV 상태
        uint8_t buckd_uv : 1; // Buck D UV 상태
        uint8_t buckc_uv : 1; // Buck C UV 상태
        uint8_t buckb_uv : 1; // Buck B UV 상태
        uint8_t bucka_uv : 1; // Buck A UV 상태
    } bits;
} MP5475_Reg07_Status;

typedef union {
    uint8_t raw; 
    struct {
        // LSB부터 순서대로 정의
        uint8_t buckd_oc_warning : 1; // Buck A OC WARNING 상태
        uint8_t buckc_oc_warning : 1; // Buck B OC WARNING 상태
        uint8_t buckb_oc_warning : 1; // Buck C OC WARNING 상태
        uint8_t bucka_oc_warning : 1; // Buck D OC WARNING 상태
        uint8_t buckd_oc : 1; // Buck A OCP 발생 상태 
        uint8_t buckc_oc : 1; // Buck B OCP 발생 상태 
        uint8_t buckb_oc : 1; // Buck C OCP 발생 상태
        uint8_t bucka_oc : 1; // Buck D OCP 발생 상태
    } bits;
} MP5475_Reg08_Status;

typedef union {
    uint8_t raw;
    struct {
        // LSB부터 순서대로 정의
        uint8_t pmic_high_temp_shutdown : 1; // PMIC 과열 셧다운 상태
        uint8_t pmic_high_temp_warning  : 1; // PMIC 과열 경고 상태
        uint8_t vdrv_ov                 : 1; // VDRV OV 상태
        uint8_t vbulk_ov                : 1; // VINx OV 상태
        uint8_t vr_fault                : 1; // Buck 출력 종합 Fault 상태
        uint8_t ldo1v1_fault            : 1; // 1.1V LDO Fault 상태
        uint8_t ldo1v8_fault            : 1; // 1.8V LDO Fault 상태
        uint8_t reserved                : 1;
    } bits;
} MP5475_Reg09_Status;

#endif