/**
  ******************************************************************************
  * @file    plf_hw_config.h
  * @author  MCD Application Team
  * @brief   This file contains the hardware configuration of the platform
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLF_HW_CONFIG_H
#define PLF_HW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* MISRAC messages linked to HAL include are ignored */
/*cstat -MISRAC2012-* */
#include "stm32l4xx_hal.h"
#include "stm32l4xx.h"
/*cstat +MISRAC2012-* */

#include "main.h"
#include "plf_modem_config.h"
#include "usart.h" /* for huartX */

/* Exported constants --------------------------------------------------------*/

/* Platform defines ----------------------------------------------------------*/

/* MODEM configuration */
#if defined(STM32L4S5xx)
#define MODEM_UART_HANDLE       huart4
#define MODEM_UART_INSTANCE     UART4
#define MODEM_UART_AUTOBAUD     (0)
#define MODEM_UART_IRQN         UART4_IRQn
#define MODEM_UART_ALTERNATE    GPIO_AF8_UART4

#else
#error Modem connector not specified
#endif  /* defined(STM32L4S5xx) */

/* UART interface */
#define MODEM_UART_BAUDRATE     (CONFIG_MODEM_UART_BAUDRATE)
#define MODEM_UART_WORDLENGTH   UART_WORDLENGTH_8B
#define MODEM_UART_STOPBITS     UART_STOPBITS_1
#define MODEM_UART_PARITY       UART_PARITY_NONE
#define MODEM_UART_MODE         UART_MODE_TX_RX

#if (CONFIG_MODEM_UART_RTS_CTS == 1)
#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_RTS_CTS
#else
#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_NONE
#endif /* (CONFIG_MODEM_UART_RTS_CTS == 1) */

/* UART interface */
#define MODEM_TX_GPIO_PORT      ((GPIO_TypeDef *)GPIOA)   /* PA0 */
#define MODEM_TX_PIN            GPIO_PIN_0                /* PA0 */
#define MODEM_RX_GPIO_PORT      ((GPIO_TypeDef *)GPIOA)   /* PA1 */
#define MODEM_RX_PIN            GPIO_PIN_1                /* PA1 */
#define MODEM_CTS_GPIO_PORT     ((GPIO_TypeDef *)GPIOC)   /* PC5 - NOT USED as CTS */
#define MODEM_CTS_PIN           GPIO_PIN_5                /* PC5 - NOT USED as CTS */
#define MODEM_RTS_GPIO_PORT     ((GPIO_TypeDef *)GPIOC)   /* PC4 - NOT USED as RTS */
#define MODEM_RTS_PIN           GPIO_PIN_4                /* PC4 - NOT USED as RTS */

/* ---- MODEM other pins configuration ---- */
#if defined(STM32L4S5xx) /* B-L45SI-IOT01 - USE ARDUINO TO STMOD ADAPTER BOARD */
/* PA15 = D9 = RESET */
#define MODEM_RST_GPIO_PORT             ((GPIO_TypeDef *)MDM_RST_GPIO_Port)
#define MODEM_RST_PIN                   MDM_RST_Pin
/* PB4 = D5 = PWR EN */
#define MODEM_PWR_EN_GPIO_PORT          ((GPIO_TypeDef *)MDM_PWR_EN_GPIO_Port)
#define MODEM_PWR_EN_PIN                MDM_PWR_EN_Pin
/* PB1 = D6 = DTR */
#define MODEM_DTR_GPIO_PORT             ((GPIO_TypeDef *)MDM_DTR_GPIO_Port)
#define MODEM_DTR_PIN                   MDM_DTR_Pin
/* PC1 = A4 = RING */
#define MODEM_RING_GPIO_PORT            ((GPIO_TypeDef *)GPIOC)
#define MODEM_RING_PIN                  GPIO_PIN_1
#define MODEM_RING_IRQN                 EXTI1_IRQn

#else
#error Modem connector not specified or invalid for this board
#endif  /* defined(STM32L4S5xx) */

/* ---- MODEM SIM SELECTION pins ---- */
#define MODEM_SIM_SELECT_0_GPIO_PORT  MDM_SIM_SELECT_0_GPIO_Port
#define MODEM_SIM_SELECT_0_PIN        MDM_SIM_SELECT_0_Pin
#define MODEM_SIM_SELECT_1_GPIO_PORT  MDM_SIM_SELECT_1_GPIO_Port
#define MODEM_SIM_SELECT_1_PIN        MDM_SIM_SELECT_1_Pin

/* DEBUG INTERFACE CONFIGURATION */
#if defined(STM32L4S5xx)
#define TRACE_INTERFACE_UART_HANDLE     huart1
#define TRACE_INTERFACE_INSTANCE        ((USART_TypeDef *)USART1)

#else
#error Connector not specified
#endif  /* defined(STM32L4S5xx) */

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_HW_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
