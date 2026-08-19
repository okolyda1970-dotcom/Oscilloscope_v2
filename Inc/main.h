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
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_dma.h"

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
#define BUTTON_3_Pin LL_GPIO_PIN_2
#define BUTTON_3_GPIO_Port GPIOE
#define BUTTON_4_Pin LL_GPIO_PIN_3
#define BUTTON_4_GPIO_Port GPIOE
#define BUTTON_1_Pin LL_GPIO_PIN_0
#define BUTTON_1_GPIO_Port GPIOC
#define BUTTON_2_Pin LL_GPIO_PIN_1
#define BUTTON_2_GPIO_Port GPIOC
#define EN_VDD_Pin LL_GPIO_PIN_1
#define EN_VDD_GPIO_Port GPIOB
#define LED_1_Pin LL_GPIO_PIN_15
#define LED_1_GPIO_Port GPIOB
#define SPI_SCK_DISPL_Pin LL_GPIO_PIN_3
#define SPI_SCK_DISPL_GPIO_Port GPIOB
#define SPI_MOSI_DISPL_Pin LL_GPIO_PIN_5
#define SPI_MOSI_DISPL_GPIO_Port GPIOB
#define A0_DISP_Pin LL_GPIO_PIN_6
#define A0_DISP_GPIO_Port GPIOB
#define RESET_DISPL_Pin LL_GPIO_PIN_7
#define RESET_DISPL_GPIO_Port GPIOB
#define CS_DISPL_Pin LL_GPIO_PIN_8
#define CS_DISPL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
