#include "tasks.h"

extern osMessageQueueId_t DTCEventQueueHandle;

/**
 * I2CTask는 1ms 주기로 PMIC의 Fault 상태를 확인하여,
 * 새롭게 발생한 Fault가 있으면 SPITask로 DTC 이벤트 큐를 전송한다.
 */
void StartI2CTask(void *argument) {

  uint8_t current_faults[5];                 // 0x05 ~ 0x09 레지스터(5바이트) 값을 저장
  static uint8_t previous_faults[5] = {0};   // 이전 Fault 상태를 저장하기 위한 static 변수
  static bool is_i2c_com_failed = false;      // I2C 통신 실패 상태 추적 플래그

  for(;;)
  {
    osDelay(1);

    if (PMIC_Read_Faults_DMA(current_faults, 5) == HAL_OK) {

      MP5475_Reg07_Status* pCurReg07 = (MP5475_Reg07_Status*)&current_faults[2];
      MP5475_Reg08_Status* pCurReg08 = (MP5475_Reg08_Status*)&current_faults[3];
      MP5475_Reg09_Status* pCurReg09 = (MP5475_Reg09_Status*)&current_faults[4];

      MP5475_Reg07_Status* pPrevReg07 = (MP5475_Reg07_Status*)&previous_faults[2];
      MP5475_Reg08_Status* pPrevReg08 = (MP5475_Reg08_Status*)&previous_faults[3];
      MP5475_Reg09_Status* pPrevReg09 = (MP5475_Reg09_Status*)&previous_faults[4];

      DTC_Message_t msg;

      /* 0x07 레지스터: UV Fault 상태 확인 */
      if (pCurReg07->bits.bucka_uv && !pPrevReg07->bits.bucka_uv) {
        msg.dtc_code = DTC_C1221_BUCK_A_UV; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }
      if (pCurReg07->bits.buckb_uv && !pPrevReg07->bits.buckb_uv) {
        msg.dtc_code = DTC_C1222_BUCK_B_UV; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }
      if (pCurReg07->bits.buckc_uv && !pPrevReg07->bits.buckc_uv) {
        msg.dtc_code = DTC_C1242_BUCK_C_UV; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }
      if (pCurReg07->bits.buckd_uv && !pPrevReg07->bits.buckd_uv) {
        msg.dtc_code = DTC_C0577_BUCK_D_UV; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }

      /* 0x08 레지스터: OC Fault 상태 확인 */
      if (pCurReg08->bits.bucka_oc && !pPrevReg08->bits.bucka_oc) {
        msg.dtc_code = DTC_C1232_BUCK_A_OC; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }
      if (pCurReg08->bits.buckb_oc && !pPrevReg08->bits.buckb_oc) {
        msg.dtc_code = DTC_C1233_BUCK_B_OC; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }
      if (pCurReg08->bits.buckc_oc && !pPrevReg08->bits.buckc_oc) {
        msg.dtc_code = DTC_C1217_BUCK_C_OC; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }
      if (pCurReg08->bits.buckd_oc && !pPrevReg08->bits.buckd_oc) {
        msg.dtc_code = DTC_C0121_BUCK_D_OC; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }

      /* 0x09 레지스터: 시스템 Fault 상태 확인 */
      if (pCurReg09->bits.pmic_high_temp_shutdown && !pPrevReg09->bits.pmic_high_temp_shutdown) {
        msg.dtc_code = DTC_U0121_SYSTEM_FAIL; 
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }

      memcpy(previous_faults, current_faults, 5);

    } else {
      // 이전에 통신 실패 상태가 아니었을 때만 DTC를 한 번 전송하여 중복 방지
      if (!is_i2c_com_failed) {
        is_i2c_com_failed = true; // 통신 실패 상태로 설정
        DTC_Message_t msg;
        // PMIC와 통신이 두절된 것은 브레이크 시스템 전체와의 통신 두절로 간주
        msg.dtc_code = DTC_U0121_SYSTEM_FAIL;
        osMessageQueuePut(DTCEventQueueHandle, &msg, 0, 0);
      }
    }
  }
}

void StartSPITask(void *argument)
{
  DTC_Message_t received_msg;
  osStatus_t status;

  for(;;)
  {
    // DTCEventQueueHandle에 메시지가 도착할 때까지 무한정 대기 (휴면 상태)
    status = osMessageQueueGet(DTCEventQueueHandle, &received_msg, NULL, osWaitForever);

    if (status == osOK)
    {
      // 메시지 수신 성공!
      // TODO: 뮤텍스를 사용하여 EEPROM 접근 제어
      // TODO: EEPROM 드라이버 함수를 호출하여 received_msg.dtc_code를 저장
      // 예: EEPROM_Write_DTC(received_msg.dtc_code);
    }
  }
}

void StartCANTask(void *argument)
{
  for(;;)
  {
    osDelay(1000);
  }
}

void StartUARTTask(void *argument)
{
  for(;;)
  {
    osDelay(1000);
  }
}

void StartADCTask(void *argument)
{
    for(;;)
    {
        osDelay(1000);
    }
}

void StartDefaultTask(void *argument)
{
  for(;;)
  {
    osDelay(1000);
  }
}
