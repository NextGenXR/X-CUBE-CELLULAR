/**
  ******************************************************************************
  * @file    freertos.c
  * @author  MCD Application Team
  * @brief   Default task : System init
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


/* Includes ------------------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Global variables ----------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

extern void application_init(void);
extern void application_start(void);

/* Private function Definition -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/* Init FreeRTOS */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /* Application components statical init  */
  application_init();

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* definition and creation of DebounceTimer */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_THREADS */

  /* Application components start */
  application_start();

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
