/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SEN21_Pin GPIO_PIN_3
#define SEN21_GPIO_Port GPIOF
#define SEN22_Pin GPIO_PIN_4
#define SEN22_GPIO_Port GPIOF
#define SEN20_Pin GPIO_PIN_5
#define SEN20_GPIO_Port GPIOF
#define SEN17_Pin GPIO_PIN_6
#define SEN17_GPIO_Port GPIOF
#define SEN18_Pin GPIO_PIN_7
#define SEN18_GPIO_Port GPIOF
#define SEN19_Pin GPIO_PIN_8
#define SEN19_GPIO_Port GPIOF
#define SEN13_Pin GPIO_PIN_9
#define SEN13_GPIO_Port GPIOF
#define SEN14_Pin GPIO_PIN_10
#define SEN14_GPIO_Port GPIOF
#define SEN16_Pin GPIO_PIN_0
#define SEN16_GPIO_Port GPIOC
#define SEN15_Pin GPIO_PIN_1
#define SEN15_GPIO_Port GPIOC
#define SEN12_Pin GPIO_PIN_0
#define SEN12_GPIO_Port GPIOA
#define SEN11_Pin GPIO_PIN_1
#define SEN11_GPIO_Port GPIOA
#define SEN10_Pin GPIO_PIN_2
#define SEN10_GPIO_Port GPIOA
#define SEN9_Pin GPIO_PIN_3
#define SEN9_GPIO_Port GPIOA
#define SEN8_Pin GPIO_PIN_4
#define SEN8_GPIO_Port GPIOA
#define SEN7_Pin GPIO_PIN_5
#define SEN7_GPIO_Port GPIOA
#define SEN6_Pin GPIO_PIN_6
#define SEN6_GPIO_Port GPIOA
#define SEN5_Pin GPIO_PIN_7
#define SEN5_GPIO_Port GPIOA
#define SEN4_Pin GPIO_PIN_4
#define SEN4_GPIO_Port GPIOC
#define SEN3_Pin GPIO_PIN_5
#define SEN3_GPIO_Port GPIOC
#define SEN2_Pin GPIO_PIN_0
#define SEN2_GPIO_Port GPIOB
#define SEN1_Pin GPIO_PIN_1
#define SEN1_GPIO_Port GPIOB
#define TIA_SHDN_Pin GPIO_PIN_0
#define TIA_SHDN_GPIO_Port GPIOG
#define CAN_STB_Pin GPIO_PIN_9
#define CAN_STB_GPIO_Port GPIOA
#define VCP_TX_Pin GPIO_PIN_6
#define VCP_TX_GPIO_Port GPIOB
#define VCP_RX_Pin GPIO_PIN_7
#define VCP_RX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
