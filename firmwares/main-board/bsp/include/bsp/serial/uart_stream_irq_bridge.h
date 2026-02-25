#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

void BspUartStream_HandleUartIrq(UART_HandleTypeDef* huart);

#ifdef __cplusplus
}
#endif
