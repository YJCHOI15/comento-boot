#ifndef TASKS_H_
#define TASKS_H_

#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "cmsis_os.h"
#include "pmic_mp5475gu.h"
#include "dtc_manager.h"

typedef struct {
    DTC_Code_t dtc_code; // 발생한 DTC의 코드 번호
} DTC_Message_t;

void StartDefaultTask(void *argument);
void StartI2CTask(void *argument);
void StartSPITask(void *argument);
void StartCANTask(void *argument);
void StartUARTTask(void *argument);
void StartADCTask(void *argument); 


#endif /* TASKS_H_ */