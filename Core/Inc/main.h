/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "stm32f4xx_hal.h"

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
void MX_I2C1_Init(void);
void MX_TIM3_Init(void);
void MX_I2S3_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FRAME_RATE_Pin GPIO_PIN_3
#define FRAME_RATE_GPIO_Port GPIOE
#define MCU_ACTIVE_Pin GPIO_PIN_4
#define MCU_ACTIVE_GPIO_Port GPIOE
#define LCD_BL_CTRL_Pin GPIO_PIN_5
#define LCD_BL_CTRL_GPIO_Port GPIOF
#define LCD_RESET_Pin GPIO_PIN_11
#define LCD_RESET_GPIO_Port GPIOD
#define LCD_TE_Pin GPIO_PIN_4
#define LCD_TE_GPIO_Port GPIOG
#define LCD_TE_EXTI_IRQn EXTI4_IRQn
#define TS_INT_Pin GPIO_PIN_5
#define TS_INT_GPIO_Port GPIOG
#define TS_INT_EXTI_IRQn EXTI9_5_IRQn
#define VSYNC_FREQ_Pin GPIO_PIN_0
#define VSYNC_FREQ_GPIO_Port GPIOE
#define RENDER_TIME_Pin GPIO_PIN_1
#define RENDER_TIME_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
#define WS2812_TIM_GPIO_Port GPIOB
#define WS2812_TIM_Pin GPIO_PIN_0
#define MIC_WS_Pin GPIO_PIN_4
#define MIC_WS_GPIO_Port GPIOA
#define MIC_CK_Pin GPIO_PIN_10
#define MIC_CK_GPIO_Port GPIOC
#define MIC_SD_Pin GPIO_PIN_12
#define MIC_SD_GPIO_Port GPIOC
#define LOG_TX_Pin GPIO_PIN_2
#define LOG_TX_GPIO_Port GPIOA
#define LOG_RX_Pin GPIO_PIN_3
#define LOG_RX_GPIO_Port GPIOA

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
