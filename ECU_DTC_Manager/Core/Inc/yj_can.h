#ifndef YJ_CAN_H
#define YJ_CAN_H

#include "main.h"

typedef struct {
    CAN_RxHeaderTypeDef header;
    uint8_t             data[8];
} CAN_Message_t;

HAL_StatusTypeDef CAN_Init(void);
HAL_StatusTypeDef CAN_SendMessage(uint32_t id, uint8_t* pData, uint8_t len);

#endif