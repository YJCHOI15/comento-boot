#ifndef TASKS_H_
#define TASKS_H_

#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "cmsis_os.h"
#include "pmic_mp5475gu.h"
#include "dtc_manager.h"
#include "eeprom_25lc256.h"
#include "yj_can.h"
#include "can_uds_protocol.h"
#include "yj_adc.h"

typedef enum {
    SAVE_DTC_REQUEST,       // 새로운 DTC 저장 요청 (I2CTask, ADCTask -> SPITask)
    READ_ALL_DTCS_REQUEST,  // 모든 DTC 읽기 요청   (CANTask -> SPITask)
    CLEAR_ALL_DTCS_REQUEST  // 모든 DTC 삭제 요청   (CANTask -> SPITask)
} DTC_RequestType_t;

// -> SPITASK 요청 메시지 큐에서 사용
typedef struct {
    DTC_RequestType_t type;       // 요청 종류
    DTC_Code_t        dtc_code;   // SAVE_DTC_REQUEST일 때만 사용
} DTC_RequestMessage_t;

// SPITask -> CANTask 응답 메시지 큐에서 사용
typedef struct {
    uint8_t dtc_count;            // 저장된 DTC 개수
    DTC_Code_t dtc_list[16];      // 저장된 DTC 목록 (최대 16개)
} DTC_ResponseMessage_t;


extern osMessageQueueId_t DTC_RequestQueueHandle;  // SPITask로 들어오는 모든 요청 큐
extern osMessageQueueId_t DTC_ResponseQueueHandle; // SPITask가 CANTask로 보내는 응답 큐
extern osMutexId_t EepromMutexHandle;          // EEPROM 접근 제어를 위한 뮤텍스

extern osMessageQueueId_t CanQueueHandle;

void StartDefaultTask(void *argument);
void StartI2CTask(void *argument);
void StartSPITask(void *argument);
void StartCANTask(void *argument);
void StartADCTask(void *argument); 
void StartUARTTask(void *argument);


#endif /* TASKS_H_ */