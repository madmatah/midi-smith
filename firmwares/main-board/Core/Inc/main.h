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
#define USER_LED_Pin GPIO_PIN_3
#define USER_LED_GPIO_Port GPIOE
#define LOAD_3_Pin GPIO_PIN_0
#define LOAD_3_GPIO_Port GPIOB
#define LOAD_5_Pin GPIO_PIN_1
#define LOAD_5_GPIO_Port GPIOB
#define LOAD_2_Pin GPIO_PIN_7
#define LOAD_2_GPIO_Port GPIOE
#define LOAD_1_Pin GPIO_PIN_8
#define LOAD_1_GPIO_Port GPIOE
#define LOAD_4_Pin GPIO_PIN_9
#define LOAD_4_GPIO_Port GPIOE
#define LCD_LED_Pin GPIO_PIN_10
#define LCD_LED_GPIO_Port GPIOE
#define LCD_CS_Pin GPIO_PIN_11
#define LCD_CS_GPIO_Port GPIOE
#define LCD_WR_RS_Pin GPIO_PIN_13
#define LCD_WR_RS_GPIO_Port GPIOE
#define MIDI_OUT_Pin GPIO_PIN_10
#define MIDI_OUT_GPIO_Port GPIOB
#define MIDI_IN_Pin GPIO_PIN_11
#define MIDI_IN_GPIO_Port GPIOB
#define LOAD_6_Pin GPIO_PIN_12
#define LOAD_6_GPIO_Port GPIOB
#define LOAD_7_Pin GPIO_PIN_13
#define LOAD_7_GPIO_Port GPIOB
#define LOAD_8_Pin GPIO_PIN_14
#define LOAD_8_GPIO_Port GPIOB
#define ROTARY_BTN_Pin GPIO_PIN_15
#define ROTARY_BTN_GPIO_Port GPIOB
#define FLASH_CS_Pin GPIO_PIN_6
#define FLASH_CS_GPIO_Port GPIOD
#define FDCAN_STANDBY_Pin GPIO_PIN_5
#define FDCAN_STANDBY_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
