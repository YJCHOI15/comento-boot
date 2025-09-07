#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "yj_uart.h"

extern UART_HandleTypeDef huart4;

#define UART_BUFFER_SIZE 256
static char uart_buffer[UART_BUFFER_SIZE];

void UART_Printf(const char *format, ...) {
    va_list args;
    int len;

    // 가변 인자 처리 시작
    va_start(args, format);

    // 포맷 문자열과 가변 인자를 uart_buffer에 저장
    len = vsnprintf(uart_buffer, UART_BUFFER_SIZE, format, args);

    // 가변 인자 처리 종료
    va_end(args);

    if (len > 0) {
        // 포맷팅된 문자열을 Polling 방식으로 전송 (Blocking)
        HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, len, 100); // 100ms timeout
    }
}