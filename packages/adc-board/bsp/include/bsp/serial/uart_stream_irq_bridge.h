#pragma once

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void BspUartStream_HandleUartIrq(UART_HandleTypeDef* huart);

#ifdef __cplusplus
}
#endif
