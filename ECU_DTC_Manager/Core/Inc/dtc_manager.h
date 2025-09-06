#ifndef DTC_MANAGER_H
#define DTC_MANAGER_H

#include <stdint.h>

typedef enum {
    /* BUCK A: 좌측 전륜 휠 스피드 센서 (Left Front Wheel Speed Sensor) 전원 */
    DTC_C1221_BUCK_A_UV = 0x1221, // C1221: Left Front Wheel Speed Sensor Input Signal is 0
    DTC_C1232_BUCK_A_OC = 0x1232, // C1232: Left Front Wheel Speed Circuit Open or Shorted

    /* BUCK B: 우측 전륜 휠 스피드 센서 (Right Front Wheel Speed Sensor) 전원 */
    DTC_C1222_BUCK_B_UV = 0x1222, // C1222: Right Front Wheel Speed Sensor Input Signal is 0
    DTC_C1233_BUCK_B_OC = 0x1233, // C1233: Right Front Wheel Speed Circuit Open or Shorted

    /* BUCK C: ABS 펌프 모터 (Pump Motor) 전원 */
    DTC_C1242_BUCK_C_UV = 0x1242, // C1242: Pump Motor Circuit Open
    DTC_C1217_BUCK_C_OC = 0x1217, // C1217: Pump Motor Shorted to Ground

    /* BUCK D: 솔레노이드 밸브 릴레이 (Solenoid Valve Relay) 전원 */
    DTC_C0577_BUCK_D_UV = 0x0577, // C0577: Left Front Solenoid Circuit Low
    DTC_C0121_BUCK_D_OC = 0x0121, // C0121: Valve Relay Circuit Malfunction

    /* ADC 감지: ECU 메인 전원 (ECU Main Power Supply) 관련 Faults */
    DTC_C1236_ADC_VOLTAGE_LOW  = 0x1236, // C1236: Low System Supply Voltage
    DTC_C1237_ADC_VOLTAGE_HIGH = 0x1237, // C1237: High System Supply Voltage

    /* 시스템 및 통신 (System & Network) 관련 Faults (U-Codes) */
    DTC_U0121_SYSTEM_FAIL = 0x0121, // U0121: Lost Communication With ABS Module (PMIC 과열 등 심각한 오류)

} DTC_Code_t;

#endif