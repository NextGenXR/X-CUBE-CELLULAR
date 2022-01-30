/**
  ******************************************************************************
  * @file    cellular_app_uiclient.h
  * @author  MCD Application Team
  * @brief   Header for cellular_app_uiclient.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
#ifndef CELLULAR_APP_UICLIENT_H
#define CELLULAR_APP_UICLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_DISPLAY == 1)

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/**
  * @brief  Initialize all needed structures to support UIClt feature
  * @param  -
  * @retval -
  */
void cellular_app_uiclient_init(void);

/**
  * @brief  Start UIClt thread
  * @param  -
  * @retval -
  */
void cellular_app_uiclient_start(void);

#endif /* USE_DISPLAY == 1 */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_APP_UICLIENT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
