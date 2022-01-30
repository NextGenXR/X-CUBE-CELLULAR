/**
  ******************************************************************************
  * @file    cellular_app.h
  * @author  MCD Application Team
  * @brief   Header for cellular_app.c module
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
#ifndef CELLULAR_APP_H
#define CELLULAR_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_CELLULAR_APP == 1)
#include <stdbool.h>
#include <stdint.h>

#include "rtosal.h"

/* Exported constants --------------------------------------------------------*/

/* Thread Name length max */
#define CELLULAR_APP_THREAD_NAME_MAX                   (uint8_t)(15)

/* Exported types ------------------------------------------------------------*/

/* Process status */
typedef enum
{
  CELLULAR_APP_PROCESS_OFF = 0,
  CELLULAR_APP_PROCESS_ON,
  CELLULAR_APP_PROCESS_STOP_REQUESTED,
  CELLULAR_APP_PROCESS_START_REQUESTED,
  CELLULAR_APP_PROCESS_STATUS_MAX          /* Must always be the last value */
} cellular_app_process_status_t;

/* Application type */
typedef enum
{
  CELLULAR_APP_TYPE_CELLULARAPP,
  CELLULAR_APP_TYPE_ECHOCLIENT,
  CELLULAR_APP_TYPE_PINGCLIENT,
  CELLULAR_APP_TYPE_UICLIENT,
  CELLULAR_APP_TYPE_MAX                    /* Must always be the last value */
} cellular_app_type_t;

/* Cellular App descriptor */
typedef struct
{
  uint8_t      app_id;            /* Application identifier */
  bool         process_status;    /* Process status: false: inactive, true: active */
  uint32_t     process_period;    /* Process period         */
  osThreadId   thread_id;         /* Thread identifier      */
  osMessageQId queue_id;          /* Queue identifier : when a callback function is called,
                                   * a message is send to the queue to ask thread to treat the event */
} cellular_app_desc_t;

/* Cellular App change structure */
typedef struct
{
  bool     process_status; /* Process status: false: inactive, true: active */
  uint32_t process_period; /* Process period */
} cellular_app_change_t;

/* String used to display the process status */
extern const uint8_t *cellular_app_process_status_string[CELLULAR_APP_PROCESS_STATUS_MAX];

/* String used to display application type */
extern const uint8_t *cellular_app_type_string[CELLULAR_APP_TYPE_MAX];

/* Message Description
cellular_app_msg_t
{
  cellular_app_msg_type_t type;
  cellular_app_msg_id_t   id;
  uint16_t                data;
} */
typedef uint8_t cellular_app_msg_type_t;
#define CELLULAR_APP_PROCESS_MSG              (cellular_app_msg_type_t)1    /* MSG is Process type */
#define CELLULAR_APP_VALUE_MAX_MSG            (CELLULAR_APP_PROCESS_MSG)    /* MSG maximum value   */

typedef uint8_t cellular_app_msg_id_t;
#define CELLULAR_APP_MODEM_CHANGE_ID          (cellular_app_msg_id_t)1      /* MSG Id Modem       change */
#define CELLULAR_APP_PROCESS_CHANGE_ID        (cellular_app_msg_id_t)2      /* MSG Id Process     change */
#define CELLULAR_APP_PERFORMANCE_ID           (cellular_app_msg_id_t)3      /* MSG Id Performance change */
#define CELLULAR_APP_VALUE_MAX_ID             (CELLULAR_APP_PERFORMANCE_ID) /* MSG Id maximum value      */

/* Set / Get CellularApp message */
#define SET_CELLULAR_APP_MSG_TYPE(msg, type)  ((msg) = ((((uint32_t)(msg)) & 0x00FFFFFFU) | (((uint8_t)(type))<<24)))
#define SET_CELLULAR_APP_MSG_ID(msg,     id)  ((msg) = ((((uint32_t)(msg)) & 0xFF00FFFFU) | (((uint8_t)(id))<<16)))
#define SET_CELLULAR_APP_MSG_DATA(msg, data)  ((msg) = ((((uint32_t)(msg)) & 0xFFFF0000U) | ((uint16_t)(data))))
#define GET_CELLULAR_APP_MSG_TYPE(msg)        ((cellular_app_msg_type_t)((((uint32_t)(msg)) & 0xFF000000U)>>24))
#define GET_CELLULAR_APP_MSG_ID(msg)          ((cellular_app_msg_id_t)((((uint32_t)(msg)) & 0x00FF0000U)>>16))
#define GET_CELLULAR_APP_MSG_DATA(msg)        ((uint16_t)(((uint32_t)(msg)) & 0xFFFF0000U))

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
/* Define the MIN macro */
#define CELLULAR_APP_MIN(a,b) (((a) < (b))  ? (a) : (b))

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  Get data status
  * @retval bool - false/true - data is not ready to transmit / data is ready
  */
bool cellular_app_is_data_ready(void);

/**
  * @brief  Provide performance feature status
  * @param  -
  * @retval bool - false/true - not started / started
  */
bool cellular_app_get_performance_status(void);

/**
  * @brief  Start performance feature
  * @param  type    - application type - unused
  * @param  iter_nb - iteration number (0: default value to use)
  * @retval bool    - false/true - performance not started / performance starting requested
  */
bool cellular_app_performance_start(cellular_app_type_t type, uint8_t iter_nb);

/**
  * @brief  Get status of a specific CellularApp application
  * @param  process_status                  - current process status
  * @param  change_requested_process_status - change requested process status
  * @retval cellular_app_process_status_t   - process status value
  */
cellular_app_process_status_t cellular_app_get_status(bool process_status, bool change_requested_process_status);

/**
  * @brief  Set status of a specific CellularApp application
  * @param  type              - application type to change
  * @param  index             - application index to change
  * @param  process_status    - process status new value to set inactive/active
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_set_status(cellular_app_type_t type, uint8_t index, bool process_status);

/**
  * @brief  Set period of a specific CellularApp application
  * @param  type              - application type to change
  * @param  index             - application index to change
  * @param  process_period    - process period new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_set_period(cellular_app_type_t type, uint8_t index, uint32_t process_period);

/**
  * @brief  Initialize all needed structures to support CellularApp features and call Cellular init
  * @param  -
  * @retval -
  */
void application_init(void);

/**
  * @brief  Start all threads needed to activate CellularApp features and call Cellular start
  * @param  -
  * @retval -
  */
void application_start(void);

#endif /* USE_CELLULAR_APP == 1 */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
