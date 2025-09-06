#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>

/* 1. CAN Message Identifiers (CAN ID) */
#define CAN_ID_DIAG_REQUEST         0x7DF  // 기능적 요청 주소 (진단기 -> ECU)
#define CAN_ID_DIAG_RESPONSE        0x7E8  // 물리적 응답 주소 (ABS ECU -> 진단기)

/* 2. UDS Service Identifiers (SID) */
#define SID_READ_DTC_INFO           0x19   // ReadDTCInformation 서비스
#define SID_CLEAR_DIAG_INFO         0x14   // ClearDiagnosticInformation 서비스

#define SID_POSITIVE_RESPONSE_MASK  0x40   // SID에 더해져 긍정 응답임을 표시 (예: 0x19 -> 0x59)
#define SID_NEGATIVE_RESPONSE       0x7F   // 부정 응답 SID

/* 3. UDS Message Data Format */

/* [요청 1] 저장된 모든 DTC 정보 읽기 (Read DTC Information) */
#define SUB_FUNC_DTC_BY_STATUS_MASK 0x02
/*
 * [진단기 -> ECU]
 * - CAN ID: 0x7DF
 * - Data[0]: 0x02 (메시지 길이: 2바이트)
 * - Data[1]: 0x19 (SID: DTC 정보 읽기)
 * - Data[2]: 0x02 (Sub-function: 상태 마스크로 DTC 리포트)
 *
 * [ECU -> 진단기] (긍정 응답)
 * - CAN ID: 0x7E8
 * - Data[0]: PCI (전체 메시지 길이)
 * - Data[1]: 0x59 (긍정 응답 SID)
 * - Data[2]: 0x02 (요청받은 Sub-function)
 * - Data[3...]: DTC 목록 (DTC 2바이트 + 상태 1바이트)
 */

/* [요청 2] 저장된 모든 DTC 정보 삭제 (Clear Diagnostic Information) */
#define GROUP_OF_DTC_ALL            0xFFFFFF
/*
 * [진단기 -> ECU]
 * - CAN ID: 0x7DF
 * - Data[0]: 0x04 (메시지 길이: 4바이트)
 * - Data[1]: 0x14 (SID: 진단 정보 삭제)
 * - Data[2-4]: 0xFFFFFF (GroupOfDTC: 모든 DTC 그룹)
 *
 * [ECU -> 진단기] (긍정 응답)
 * - CAN ID: 0x7E8
 * - Data[0]: 0x01 (메시지 길이: 1바이트)
 * - Data[1]: 0x54 (긍정 응답 SID)
 */

#endif