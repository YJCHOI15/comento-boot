#include "tasks.h"

// EEPROM에 DTC를 저장할 시작 주소 및 최대 개수 정의
#define DTC_STORAGE_START_ADDRESS 0x0100   // 사용자 정의 (실제 저장공간 0x0000 ~ 0x7FFF)
#define MAX_DTC_COUNT             16
#define DTC_ENTRY_SIZE            sizeof(DTC_Code_t) // DTC 하나는 2바이트

static void Process_CAN_Response(CAN_Message_t* rx_msg);

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

/* 
 * CAN 메시지 수신을 기다리다가, 진단 요청이 오면 처리한다. 
 */
void StartCANTask(void *argument)
{
  CAN_Message_t rx_msg;
  osStatus_t status;

  // CAN 드라이버 초기화 및 인터럽트 활성화
  CAN_Init();

  for(;;)
  {
    // CanQueueHandle에 메시지가 도착할 때까지 Blocked
    status = osMessageQueueGet(CanQueueHandle, &rx_msg, NULL, osWaitForever);
    if (status == osOK) {
      // 수신된 메시지 ID가 진단 요청 ID(0x7DF)일 경우에만 처리
      if (rx_msg.header.StdId == CAN_ID_DIAG_REQUEST) {
        Process_CAN_Response(&rx_msg);
      }
    }
  }
}

/**
 * 수신된 UDS 진단 요청 메시지를 파싱하고 처리한다.
 * rx_msg: 수신된 CAN 메시지 데이터
 */
static void Process_CAN_Response(CAN_Message_t* rx_msg)
{
  uint8_t sid = rx_msg->data[1]; // Service ID

  switch (sid) {

    case SID_READ_DTC_INFO: // 0x19 - DTC 정보 읽기 요청
    {
      // 1. SPITask에게 "모든 DTC를 읽어달라"고 요청
      DTC_RequestMessage_t request_to_spi;
      request_to_spi.type = READ_ALL_DTCS_REQUEST;
      osMessageQueuePut(DTC_RequestQueueHandle, &request_to_spi, 0, 10);

      // 2. SPITask로부터 응답이 올 때까지 잠시 Blocked
      DTC_ResponseMessage_t response_from_spi;
      if (osMessageQueueGet(DTC_ResponseQueueHandle, &response_from_spi, NULL, 100) == osOK) {
        // 3. UDS 프로토콜에 맞춰 응답 메시지 포맷팅
        uint8_t tx_data[8] = {0};
        uint8_t valid_dtc_count = 0;

        // 유효한 DTC만 필터링하여 응답 데이터 구성
        for (int i = 0; i < MAX_DTC_COUNT && valid_dtc_count < 2; i++) {
            if (response_from_spi.dtc_list[i] != 0x0000 && response_from_spi.dtc_list[i] != 0xFFFF) {
                if (valid_dtc_count == 0) { // 첫 번째 DTC
                    tx_data[3] = (response_from_spi.dtc_list[i] >> 8) & 0xFF; // DTC High Byte
                    tx_data[4] = response_from_spi.dtc_list[i] & 0xFF;        // DTC Low Byte
                    tx_data[5] = 0x09; // DTC Status (exmple)
                } else { // 두 번째 DTC
                    tx_data[6] = (response_from_spi.dtc_list[i] >> 8) & 0xFF;
                    tx_data[7] = response_from_spi.dtc_list[i] & 0xFF;
                }
                valid_dtc_count++;
            }
        }

        // PCI 및 SID, Sub-function 설정
        tx_data[0] = 1 + (valid_dtc_count * 3); // PCI: SID + SubFunc + DTCs
        tx_data[1] = SID_READ_DTC_INFO | SID_POSITIVE_RESPONSE_MASK; // 0x59
        tx_data[2] = SUB_FUNC_DTC_BY_STATUS_MASK; // 0x02

        // 4. 진단기로 최종 응답 전송
        CAN_SendMessage(CAN_ID_DIAG_RESPONSE, tx_data, tx_data[0] + 1);
      }
      break;
    }

    case SID_CLEAR_DIAG_INFO: // 0x14 - DTC 삭제 요청
    {
      // 1. SPITask에게 "모든 DTC를 삭제해달라"고 요청
      DTC_RequestMessage_t request_to_spi;
      request_to_spi.type = CLEAR_ALL_DTCS_REQUEST;
      osMessageQueuePut(DTC_RequestQueueHandle, &request_to_spi, 0, 10);

      // 2. UDS 프로토콜에 맞춰 긍정 응답 메시지 포맷팅
      uint8_t tx_data[2];
      tx_data[0] = 0x01; // PCI
      tx_data[1] = SID_CLEAR_DIAG_INFO | SID_POSITIVE_RESPONSE_MASK; // 0x54

      // 3. 진단기로 최종 응답 전송
      CAN_SendMessage(CAN_ID_DIAG_RESPONSE, tx_data, 2);
      break;
    }
  }
}

void StartADCTask(void *argument)
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

void StartDefaultTask(void *argument)
{
  for(;;)
  {
    osDelay(1000);
  }
}
