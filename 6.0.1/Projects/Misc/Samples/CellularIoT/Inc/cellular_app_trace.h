/**
  ******************************************************************************
  * @file    cellular_app_trace.h
  * @author  MCD Application Team
  * @brief   Header for echoclient_setup.c module
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
#ifndef CELLULAR_APP_TRACE_H
#define CELLULAR_APP_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_CELLULAR_APP == 1)

#include "error_handler.h"

#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#else /* USE_PRINTF == 1U */
#include <stdio.h>
#endif /* USE_PRINTF == 0U */

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef int32_t cellular_app_error_type_t;
#define CELLULAR_APP_ERROR_CELLULARAPP          (cellular_app_error_type_t)(10)
#define CELLULAR_APP_ERROR_ECHOCLIENT           (cellular_app_error_type_t)(20)
#define CELLULAR_APP_ERROR_PINGCLIENT           (cellular_app_error_type_t)(30)
#if ((USE_DISPLAY == 1) || (USE_SENSORS == 1))
#define CELLULAR_APP_ERROR_UICLIENT             (cellular_app_error_type_t)(40)
#endif /* (USE_DISPLAY == 1) || (USE_SENSORS == 1) */

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Define Error macro */
#define CELLULAR_APP_ERROR(id, gravity)  ERROR_Handler(DBG_CHAN_APPLICATION, id, gravity);

/* PRINT FORCE must always be displayed whatever the configuration */
#if (USE_PRINTF == 0U)
#define PRINT_FORCE(format, args...)     TRACE_PRINT_FORCE(DBG_CHAN_APPLICATION, DBL_LVL_P0, format "\n\r", ## args)
#else /* USE_PRINTF == 1U */
#define PRINT_FORCE(format, args...)     (void)printf(format "\n\r", ## args);
#endif /* USE_PRINTF == 0U */

/* Optional Trace definition */
#if (USE_TRACE_APPLICATION == 1U)

#if (USE_PRINTF == 0U)
#define PRINT_INFO(format, args...)      TRACE_PRINT(DBG_CHAN_APPLICATION, DBL_LVL_P0, format "\n\r", ## args)
#define PRINT_DBG(format, args...)       TRACE_PRINT(DBG_CHAN_APPLICATION, DBL_LVL_P1, format "\n\r", ## args)

#else /* USE_PRINTF == 1U */
#define PRINT_INFO(format, args...)      (void)printf(format "\n\r", ## args);
/* To reduce trace PRINT_DBG is deactivated when using printf */
/* #define PRINT_DBG(format, args...)       (void)printf(format "\n\r", ## args); */
#define PRINT_DBG(...)               __NOP(); /* Nothing to do */
#endif /* USE_PRINTF == 0U */

#else /* USE_TRACE_APPLICATION == 0U */
/* No trace, deactivate all */
#define PRINT_INFO(...)              __NOP(); /* Nothing to do */
#define PRINT_DBG(...)               __NOP(); /* Nothing to do */

#endif /* USE_TRACE_APPLICATION == 1U */

/* Exported functions ------------------------------------------------------- */



#endif /* USE_CELLULAR_APP == 1 */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_APP_TRACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
