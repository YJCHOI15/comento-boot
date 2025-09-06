#include "tasks.h"

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

    if (PMIC_Read_Faults(current_faults, 5) == HAL_OK) {

      MP5475_Reg07_Status* pCurReg07 = (MP5475_Reg07_Status*)&current_faults[2];
      MP5475_Reg08_Status* pCurReg08 = (MP5475_Reg08_Status*)&current_faults[3];
      MP5475_Reg09_Status* pCurReg09 = (MP5475_Reg09_Status*)&current_faults[4];

      MP5475_Reg07_Status* pPrevReg07 = (MP5475_Reg07_Status*)&previous_faults[2];
      MP5475_Reg08_Status* pPrevReg08 = (MP5475_Reg08_Status*)&previous_faults[3];
      MP5475_Reg09_Status* pPrevReg09 = (MP5475_Reg09_Status*)&previous_faults[4];

      DTC_RequestMessage_t msg;
      msg.type = SAVE_DTC_REQUEST;

      /* 0x07 레지스터: UV Fault 상태 확인 */
      if (pCurReg07->bits.bucka_uv && !pPrevReg07->bits.bucka_uv) {
        msg.dtc_code = DTC_C1221_BUCK_A_UV; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }
      if (pCurReg07->bits.buckb_uv && !pPrevReg07->bits.buckb_uv) {
        msg.dtc_code = DTC_C1222_BUCK_B_UV; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }
      if (pCurReg07->bits.buckc_uv && !pPrevReg07->bits.buckc_uv) {
        msg.dtc_code = DTC_C1242_BUCK_C_UV; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }
      if (pCurReg07->bits.buckd_uv && !pPrevReg07->bits.buckd_uv) {
        msg.dtc_code = DTC_C0577_BUCK_D_UV; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }

      /* 0x08 레지스터: OC Fault 상태 확인 */
      if (pCurReg08->bits.bucka_oc && !pPrevReg08->bits.bucka_oc) {
        msg.dtc_code = DTC_C1232_BUCK_A_OC; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }
      if (pCurReg08->bits.buckb_oc && !pPrevReg08->bits.buckb_oc) {
        msg.dtc_code = DTC_C1233_BUCK_B_OC; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }
      if (pCurReg08->bits.buckc_oc && !pPrevReg08->bits.buckc_oc) {
        msg.dtc_code = DTC_C1217_BUCK_C_OC; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }
      if (pCurReg08->bits.buckd_oc && !pPrevReg08->bits.buckd_oc) {
        msg.dtc_code = DTC_C0121_BUCK_D_OC; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }

      /* 0x09 레지스터: 시스템 Fault 상태 확인 */
      if (pCurReg09->bits.pmic_high_temp_shutdown && !pPrevReg09->bits.pmic_high_temp_shutdown) {
        msg.dtc_code = DTC_U0121_SYSTEM_FAIL; 
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }

      memcpy(previous_faults, current_faults, 5);

    } else {
      // 이전에 통신 실패 상태가 아니었을 때만 DTC를 한 번 전송하여 중복 방지
      if (!is_i2c_com_failed) {
        is_i2c_com_failed = true; // 통신 실패 상태로 설정
        DTC_RequestMessage_t msg;
        msg.type = SAVE_DTC_REQUEST;
        // PMIC와 통신이 두절된 것은 브레이크 시스템 전체와의 통신 두절로 간주
        msg.dtc_code = DTC_U0121_SYSTEM_FAIL;
        osMessageQueuePut(DTC_RequestQueueHandle, &msg, 0, 0);
      }
    }
  }
}

// EEPROM에 DTC를 저장할 시작 주소 및 최대 개수 정의
#define DTC_STORAGE_START_ADDRESS 0x0100   // 사용자 정의 (실제 저장공간 0x0000 ~ 0x7FFF)
#define MAX_DTC_COUNT             16
#define DTC_ENTRY_SIZE            sizeof(DTC_Code_t) // DTC 하나는 2바이트

/*
 * SPITask는 DTC 관련 요청을 받아 EEPROM에 읽고 쓰는 역할을 전담한다.
 */
void StartSPITask(void *argument) {
  DTC_RequestMessage_t request_msg;
  osStatus_t status;

  for(;;) {
    // DTC_RequestQueueHandle에 메시지가 도착할 때까지 Blocked
    status = osMessageQueueGet(DTC_RequestQueueHandle, &request_msg, NULL, osWaitForever);

    if (status == osOK) {
      // EEPROM 접근을 위해 뮤텍스를 점유
      if (osMutexAcquire(EepromMutexHandle, 100) == osOK) {
        switch (request_msg.type) {
          case SAVE_DTC_REQUEST:
          {
            // 1. 현재 저장된 DTC 목록을 EEPROM에서 읽어온다. 
            DTC_Code_t stored_dtcs[MAX_DTC_COUNT] = {0};
            EEPROM_Read_DTCs(DTC_STORAGE_START_ADDRESS, (uint8_t*)stored_dtcs, sizeof(stored_dtcs));

            // 2. 이미 저장된 DTC인지, 빈 공간이 있는지 확인한다.
            bool already_exists = false;
            int empty_slot = -1;
            for (int i = 0; i < MAX_DTC_COUNT; i++) {
              if (stored_dtcs[i] == request_msg.dtc_code) {
                already_exists = true;
                break;
              }
              if ((stored_dtcs[i] == 0x0000 || stored_dtcs[i] == 0xFFFF) && empty_slot == -1) {
                empty_slot = i;
              }
            }

            // 3. 중복되지 않았고, 빈 공간이 있으면 새로운 DTC를 추가하고 EEPROM에 쓴다.
            if (!already_exists && empty_slot != -1) {
              uint16_t write_address = DTC_STORAGE_START_ADDRESS + (empty_slot * DTC_ENTRY_SIZE);
              EEPROM_Write_DTC(write_address, (uint8_t*)&request_msg.dtc_code, DTC_ENTRY_SIZE);
            }
            break;
          }

          case READ_ALL_DTCS_REQUEST:
          {
            DTC_ResponseMessage_t response_msg = {0};
            EEPROM_Read_DTCs(DTC_STORAGE_START_ADDRESS, (uint8_t*)response_msg.dtc_list, sizeof(response_msg.dtc_list));

            // 유효한 DTC 개수 카운트
            for (int i = 0; i < MAX_DTC_COUNT; i++) {
              if (response_msg.dtc_list[i] != 0x0000 && response_msg.dtc_list[i] != 0xFFFF) {
                response_msg.dtc_count++;
              }
            }
            // CANTask로 응답 전송
            osMessageQueuePut(DTC_ResponseQueueHandle, &response_msg, 0, 10);
            break;
          }

          case CLEAR_ALL_DTCS_REQUEST:
          {
            uint8_t clear_buffer[MAX_DTC_COUNT * DTC_ENTRY_SIZE];
            memset(clear_buffer, 0xFF, sizeof(clear_buffer)); // EEPROM은 보통 0xFF로 지움
            EEPROM_Write_DTC(DTC_STORAGE_START_ADDRESS, clear_buffer, sizeof(clear_buffer));
            break;
          }
        }
        // 작업이 끝났으므로 뮤텍스를 반드시 해제한다.
        osMutexRelease(EepromMutexHandle);
      }
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
