#include "main.h"
#include "cmsis_os.h"
#include "tasks.h"
#include "pmic_mp5475gu.h"

void StartI2CTask(void *argument)
{
  // 0x05 ~ 0x09 레지스터(5바이트) 값을 저장
  uint8_t fault_registers[5];

  for(;;)
  {
    osDelay(1);

    // PMIC 드라이버 함수를 호출하여 Fault 레지스터들을 read
    if (PMIC_Read_Faults_DMA(fault_registers, 5) == HAL_OK) {
      // TODO: 읽기 성공 시 데이터 처리 로직 구현
      // 예: fault_registers[2] (0x07 레지스터) 값을 파싱하여 UV/OV 상태 확인
      // MP5475_Reg07_Status* pReg07 = (MP5475_Reg07_Status*)&fault_registers[2];
      // if (pReg07->bits.bucka_ov == 1) { ... }
    } else {
      // TODO: I2C 통신 타임아웃 또는 에러 발생 시 처리 로직 구현
    }
  }
}

void StartSPITask(void *argument)
{
  for(;;)
  {
    osDelay(1000);
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
