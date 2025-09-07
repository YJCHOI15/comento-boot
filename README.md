# RTOS 기반 PMIC 모니터링 및 차량 진단 제어기 펌웨어 개발

**STM32F413ZHTX** MCU를 기반으로, RTOS 환경에서 **전력 관리 반도체**(PMIC)의 상태를 실시간으로 감시하고 **차량 고장 진단 코드**(DTC)를 관리하는 임베디드 펌웨어 프로젝트입니다.

본 프로젝트는 자동차 소프트웨어 개발 표준인 **V-Model 프로세스**를 따라 체계적으로 진행되었으며, **실시간 운영체제**(RTOS)를 활용하여 여러 주변 장치를 안정적으로 동시에 제어하는 것을 목표로 합니다. PMIC에서 감지된 Fault 정보와 ADC로 측정한 전압 이상 상태를 DTC로 변환하여 EEPROM에 기록하고, 외부 진단기의 CAN 통신 요청에 응답하여 진단 정보를 제공하는 차량용 제어기(ECU)의 핵심 기능을 구현했습니다.

---

## 주요 기능
1. **RTOS 기반 멀티태스킹**: 5개의 독립적인 태스크(I2C, SPI, CAN, ADC, UART)를 구현하고, Message Queue, Semaphore, Mutex를 활용하여 태스크 간 안전한 통신 및 자원 공유를 보장합니다.

2. **실시간 PMIC Fault 감지**: I2C 통신을 통해 주기적으로 PMIC(MP5475GU)의 내부 레지스터를 읽어 저전압(UV), 과전류(OC) 등의 이상 상태를 실시간으로 감지합니다.

3. **ADC 전압 모니터링**: 시스템의 주요 전원 라인을 ADC로 직접 측정하여, PMIC의 디지털 Fault만으로는 감지하기 어려운 미세한 전압 변동 및 이상 상태를 감지합니다.

4. **DTC 관리 시스템**: 감지된 모든 고장 신호를 표준 OBD-II DTC 형식으로 변환하고, SPI 통신을 통해 비휘발성 메모리인 EEPROM(25LC256)에 안정적으로 저장 및 관리합니다.

5. **UDS 기반 CAN 진단 통신**: 자동차 진단 표준인 UDS(ISO 14229) 프로토콜을 기반으로 외부 진단기와의 통신 규약을 정의하고, DTC 정보 조회 및 삭제 요청을 처리합니다.

6. **비동기 드라이버**: I2C/SPI 통신에는 DMA와 Semaphore를, CAN 수신에는 Interrupt와 Message Queue를 적용하여 CPU의 부하를 최소화하고 시스템의 전반적인 반응성과 효율성을 극대화했습니다.

---

## System Architecture
### 하드웨어 구성도
MCU를 중심으로 각 주변 장치가 다음과 같이 연결됩니다.

<img width="844" height="722" alt="image" src="https://github.com/user-attachments/assets/0111e07c-1abb-452c-acd2-89be6b1436e3" />


**MCU ↔ PMIC (MP5475GU)**: I2C 통신을 통해 PMIC의 Fault 상태 레지스터를 읽고 제어합니다.

**MCU ↔ EEPROM (25LC256)**: SPI 통신을 통해 생성된 DTC를 저장하거나 읽어옵니다.

**MCU ↔ CAN Transceiver (TJA1051)**: CAN 프로토콜을 사용하여 외부 진단기와 통신합니다. 트랜시버는 MCU의 논리 신호를 CAN 버스의 물리적 차동 신호로 변환합니다.

---

### 소프트웨어 아키텍처
소프트웨어는 Application Layer와 Driver Layer의 2계층 구조로 설계하여 코드의 모듈성, 재사용성, 유지보수성을 향상시켰습니다.

**Driver Layer**: 각 하드웨어(PMIC, EEPROM 등)를 직접 제어하는 저수준(Low-Level) 드라이버 함수들을 포함합니다. HAL 라이브러리를 한 번 더 감싸 직관적인 API를 제공합니다.

**Application Layer**: RTOS 태스크들을 통해 시스템의 전체적인 동작 시나리오와 핵심 로직을 구현합니다.

**RTOS 태스크 설계**:

| 태스크      | 주기  | 주요 역할 및 동작 방식                                                    | 동기화 메커니즘             |
| -------- | --- | ---------------------------------------------------------------- | -------------------- |
| I2CTask  | 1ms | PMIC의 Fault 상태를 주기적으로 감시하고, 새로운 Fault 발생 시 SPITask에 DTC 저장을 요청함  | Message Queue (송신)   |
| SPITask  | 이벤트 | I2CTask나 CANTask로부터의 요청이 있을 때만 동작하여 EEPROM에 DTC를 읽고 씀            | Message Queue (수신)   |
| CANTask  | 이벤트 | 외부 진단기로부터 CAN 메시지 수신 시 동작하며, DTC 조회/삭제 요청을 SPITask에 전달하고 결과를 응답함 | Message Queue (수/송신) |
| ADCTask  | 5ms | PMIC 특정 채널의 아날로그 전압을 직접 측정하고, 임계값 초과 시 SPITask에 DTC 저장을 요청함      | Message Queue (송신)   |
| UARTTask | 5ms | 시스템의 현재 상태나 ADC 측정값 등 주요 디버깅 정보를 UART를 통해 터미널로 출력함               | (해당 없음)              |


**자원 보호**: 여러 태스크가 동시에 EEPROM에 접근하는 것을 방지하기 위해 Mutex를 사용하여 EEPROM 자원을 보호합니다.

---

## Development Process

본 프로젝트는 요구사항의 누락 및 오해석을 방지하고 시스템의 신뢰성을 확보하기 위해 V-Model 개발 프로세스를 기반으로 진행되었습니다. 각 단계별 산출물은 [/docs](https://github.com/YJCHOI15/ecu-dtc-manager/tree/main/docs) 폴더에서 확인할 수 있습니다.

<img width="1338" height="581" alt="image" src="https://github.com/user-attachments/assets/6000bedd-4fe0-4009-82c9-81b5e2b2c648" />

### 요구사항 분석 및 시스템 설계 (V-Model 좌측):

**1주차**: 통신 프로토콜(SPI, I2C, CAN) 및 데이터 처리 방식(Polling, Interrupt, DMA)을 비교 분석하고 시스템에 가장 적합한 기술을 선정했습니다. (**1주차 보고서.pdf 참조**)

<img width="1685" height="785" alt="image" src="https://github.com/user-attachments/assets/25a3d0f3-6ad5-4965-8cb8-b6638b26fc24" />

**2주차 계획**: V-모델에 기반하여 2주차 개발 목표와 상세 계획을 수립했습니다. (**V-Model 2주차 개발 계획.pdf 참조**)

### 상세 설계 및 구현 (V-Model 중앙):

**2주차**: 시스템 아키텍처(H/W, S/W)를 확정하고, RTOS 태스크와 드라이버를 상세 설계한 후 C언어로 핵심 로직을 구현했습니다. (2주차 보고서.pdf 참조)

<img width="1686" height="786" alt="image" src="https://github.com/user-attachments/assets/0cc63b47-1e1f-4c53-8b7e-d5953e804530" />

### 단위/통합/시스템 테스트 (V-Model 우측):

향후 실제 하드웨어를 기반으로 각 모듈 및 전체 시스템의 기능과 안정성을 검증하는 단계를 진행할 예정입니다. (개선 및 보완 사항 참조)

---

## 향후 개선 및 보완 사항
1. **CAN 전송 프로토콜(ISO 15765-2) 구현**: 현재 단일 프레임 제약으로 최대 2개의 DTC만 전송 가능합니다. 8바이트가 넘는 긴 데이터를 여러 프레임으로 나누어 보내는 전송 프로토콜을 구현하여 저장된 모든 DTC 목록을 한 번에 전송하는 완전한 진단 기능을 구현할 수 있습니다.

2. **동적 Fault 복구 로직 추가**: 현재는 Fault 발생 시 기록에 중점을 둡니다. 향후 특정 전원 채널에서 Fault 발생 시 해당 채널을 비활성화했다가 일정 시간 후 자동으로 재활성화하는 등 능동적인 에러 처리 로직을 추가하여 시스템의 강건성(Robustness)을 높일 수 있습니다.

3. **실제 하드웨어 기반 V-Model 검증 수행**: V-모델의 우측 단계인 통합 및 시스템 테스트를 실제 하드웨어 환경에서 수행하여 펌웨어의 최종 신뢰성을 확보할 계획입니다.
