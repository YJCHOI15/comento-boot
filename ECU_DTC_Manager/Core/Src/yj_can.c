#include "yj_can.h"

extern CAN_HandleTypeDef hcan1;

HAL_StatusTypeDef CAN_Init(void)
{
    // CAN 수신 FIFO 0에 메시지가 도착하면 인터럽트가 발생하도록 설정
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return HAL_ERROR;
    }

    // CAN 컨트롤러 시작
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

/* ID와 uds 프로토콜 기반 데이터를 실제 CAN 버스에 전송한다. */
HAL_StatusTypeDef CAN_SendMessage(uint32_t id, uint8_t* pData, uint8_t len)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;

    tx_header.StdId = id;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = len;

    // 비어있는 CAN 송신 메일박스가 있을 때까지 최대 10ms 대기
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0) {
        return HAL_CAN_AddTxMessage(&hcan1, &tx_header, pData, &tx_mailbox);
    }
    return HAL_BUSY;
}