/**
  ******************************************************************************
  * @file    cellular_app.c
  * @author  MCD Application Team
  * @brief   Cellular Application :
  *          - Create and Manage X instances of EchoClt
  *          - Create and Manage 1 instance of Ping
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

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_CELLULAR_APP == 1)
#include <string.h>
#include <stdbool.h>

#include "cellular_app.h"
#include "cellular_app_trace.h"

#include "cellular_app_socket.h"

#include "cellular_app_echoclient.h"
#include "cellular_app_pingclient.h"
#if ((USE_DISPLAY == 1) || (USE_SENSORS == 1))
#include "cellular_app_uiclient.h"
#endif /* (USE_DISPLAY == 1) || (USE_SENSORS == 1) */

#include "rtosal.h"

#include "cellular_control_api.h"
#include "cellular_runtime_custom.h"

#if (USE_CMD_CONSOLE == 1)
#include "cellular_app_cmd.h"
#endif /* USE_CMD_CONSOLE == 1 */

#if (USE_RTC == 1)
#include "cellular_app_datetime.h"
#endif /* USE_RTC == 1 */

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Trace shortcut */
static const uint8_t *p_cellular_app_trace;

/* Status of Modem */
static bool cellular_app_data_is_ready; /* false/true: data is not ready/data is ready */

/* Global variables ----------------------------------------------------------*/
/* String used to display application process status */
const uint8_t *cellular_app_process_status_string[CELLULAR_APP_PROCESS_STATUS_MAX] =
{
  (uint8_t *)"Off",
  (uint8_t *)"On",
  (uint8_t *)"Stop requested",
  (uint8_t *)"Start requested"
};

/* String used to display application type */
const uint8_t *cellular_app_type_string[CELLULAR_APP_TYPE_MAX] =
{
  (uint8_t *)"CellularApp",
  (uint8_t *)"Echoclt",
  (uint8_t *)"Ping",
  (uint8_t *)"UIclt"
};

/* Private functions prototypes ----------------------------------------------*/
/* Callback for IP info: if (IP !=0U) then data is ready */
static void cellular_app_ip_info_cb(ca_event_type_t event_type, const cellular_ip_info_t *const p_ip_info,
                                    void *const p_callback_ctx);

static void cellular_app_propagate_info(ca_event_type_t event_type);

/* Public  functions  prototypes ---------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Callback called when IP info changed
  * @note   Used to know when IP info change => provide when Modem is not ready/ready to transmit
  * @param  event_type     - Event that happened: CA_IP_INFO_EVENT.
  * @param  p_ip_info      - The new IP information.
  * @param  p_callback_ctx - The p_callback_ctx parameter in cellular_ip_info_changed_registration function.
  * @retval -
  */
static void cellular_app_ip_info_cb(ca_event_type_t event_type, const cellular_ip_info_t *const p_ip_info,
                                    void *const p_callback_ctx)
{
  UNUSED(p_callback_ctx);

  /* Event to know Modem status ? */
  if ((event_type == CA_IP_INFO_EVENT)  && (p_ip_info != NULL))
  {
    /* If IP address is not null then it means Data is ready */
    if (p_ip_info->ip_addr.addr != 0U)
    {
      /* Data is ready */
      if (cellular_app_is_data_ready() == false)
      {
        /* Modem is ready */
        PRINT_FORCE("%s: Modem ready to transmit data", p_cellular_app_trace)
        cellular_app_data_is_ready = true;
        cellular_app_propagate_info(CA_IP_INFO_EVENT);
      }
      /* else: nothing to do because data ready status already known */
    }
    else
    {
      if (cellular_app_is_data_ready() == true)
      {
        /* Modem is not ready */
        PRINT_FORCE("%s: Modem NOT ready to transmit data!", p_cellular_app_trace)
        cellular_app_data_is_ready = false;
      }
      /* else: nothing to do because data ready status already known */
    }
  }
}

/**
  * @brief  Send to all applications a message
  * @param  event_type - event type
  * @retval -
  */
static void cellular_app_propagate_info(ca_event_type_t event_type)
{
  UNUSED(event_type);
  uint32_t queue_msg = 0U;

  /* if (event_type == CA_IP_INFO_EVENT) */
  SET_CELLULAR_APP_MSG_TYPE(queue_msg, CELLULAR_APP_PROCESS_MSG);
  SET_CELLULAR_APP_MSG_ID(queue_msg, CELLULAR_APP_MODEM_CHANGE_ID);
  (void)cellular_app_echoclient_send_msg(0xFFU, queue_msg);
  (void)cellular_app_pingclient_send_msg(queue_msg);
}

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Get data status
  * @retval bool - false/true - data is not ready to transmit / data is ready
  */
bool cellular_app_is_data_ready(void)
{
  return (cellular_app_data_is_ready);
}

/**
  * @brief  Provide performance feature status
  * @param  -
  * @retval bool - false/true - not started / started
  */
bool cellular_app_get_performance_status(void)
{
  return (cellular_app_echoclient_get_performance_status());
}

/**
  * @brief  Start performance feature
  * @param  type    - application type - unused
  * @param  iter_nb - iteration number (0: default value to use)
  * @retval bool    - false/true - performance not started / performance starting requested
  */
bool cellular_app_performance_start(cellular_app_type_t type, uint8_t iter_nb)
{
  bool result = true;
  uint32_t queue_msg = 0U;

  UNUSED(type);

  /* Check a performance test is not already in progress */
  if (cellular_app_get_performance_status() == true)
  {
    result = false;
    PRINT_FORCE("%s: Performance already requested!", p_cellular_app_trace)
  }
  else
  {
    /* Check all process off ? */
    for (uint8_t i = 0U; i < ECHOCLIENT_THREAD_NUMBER; i++)
    {
      /* Is process[i] off and will not start */
      if (cellular_app_echoclient_get_status(i) != CELLULAR_APP_PROCESS_OFF)
      {
        result = false;
        PRINT_FORCE("%s %d: NOT fully stopped! Stop it or wait socket is closed before to retry!",
                    cellular_app_type_string[CELLULAR_APP_TYPE_ECHOCLIENT], (i + 1U))
      }
    }
    /* Is process[i] off and will not start */
    if (cellular_app_pingclient_get_status() != CELLULAR_APP_PROCESS_OFF)
    {
      result = false;
      PRINT_FORCE("%s: NOT fully stopped! Stop it or wait session end before to retry!",
                  cellular_app_type_string[CELLULAR_APP_TYPE_PINGCLIENT])
    }
  }

  if (result == true)
  {
    if (cellular_app_echoclient_performance(true, iter_nb) == true)
    {
      PRINT_FORCE("<<< %s 1: Performance START requested...>>>", cellular_app_type_string[CELLULAR_APP_TYPE_ECHOCLIENT])

      SET_CELLULAR_APP_MSG_TYPE(queue_msg, CELLULAR_APP_PROCESS_MSG);
      SET_CELLULAR_APP_MSG_ID(queue_msg, CELLULAR_APP_PERFORMANCE_ID);
      if (cellular_app_echoclient_send_msg(0U, queue_msg) != true)
      {
        PRINT_FORCE("<<< %s 1: Performance START aborted!>>>", cellular_app_type_string[CELLULAR_APP_TYPE_ECHOCLIENT])
        /* Restore default value */
        (void)cellular_app_echoclient_performance(false, 0U);
      }
    }
    else
    {
      PRINT_FORCE("<<< %s 1: Performance START rejected!>>>", cellular_app_type_string[CELLULAR_APP_TYPE_ECHOCLIENT])
    }
  }

  return (result);
}

/**
  * @brief  Get status of a specific CellularApp application
  * @param  process_status                  - current process status
  * @param  change_requested_process_status - change requested process status
  * @retval cellular_app_process_status_t   - process status value
  */
cellular_app_process_status_t cellular_app_get_status(bool process_status, bool change_requested_process_status)
{
  cellular_app_process_status_t result;

  if (process_status == true)
  {
    if (change_requested_process_status == false)
    {
      result = CELLULAR_APP_PROCESS_STOP_REQUESTED;
    }
    else
    {
      result = CELLULAR_APP_PROCESS_ON;
    }
  }
  else /* process_status == false */
  {
    if (change_requested_process_status == true)
    {
      result = CELLULAR_APP_PROCESS_START_REQUESTED;
    }
    else
    {
      result = CELLULAR_APP_PROCESS_OFF;
    }
  }

  return (result);
}

/**
  * @brief  Set status of a specific CellularApp application
  * @param  type              - application type to change
  * @param  index             - application index to change
  * @param  process_status    - process status new value to set inactive/active
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_set_status(cellular_app_type_t type, uint8_t index, bool process_status)
{
  bool result = false;

  /* During performance test on/off not authorized */
  if (cellular_app_get_performance_status() == false)
  {
    switch (type)
    {
      case CELLULAR_APP_TYPE_ECHOCLIENT :
        result = cellular_app_echoclient_set_status(index, process_status);
        break;
      case CELLULAR_APP_TYPE_PINGCLIENT :
        if (index == 0U)
        {
          result = cellular_app_pingclient_set_status(process_status);
        }
        break;
      default :
        __NOP(); /* result already set to false */
        break;
    }
  }
  else
  {
    PRINT_FORCE("%s: Performance test in progress! Wait its end before to retry!", p_cellular_app_trace)
  }

  return (result);
}

/**
  * @brief  Set period of a specific CellularApp application
  * @param  type              - application type to change
  * @param  index             - application index to change
  * @param  process_period    - process period new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_set_period(cellular_app_type_t type, uint8_t index, uint32_t process_period)
{
  bool result = false;

  /* During performance test, process period change not authorized */
  if (cellular_app_get_performance_status() == false)
  {
    switch (type)
    {
      case CELLULAR_APP_TYPE_ECHOCLIENT :
        result = cellular_app_echoclient_set_period(index, process_period);
        break;
      case CELLULAR_APP_TYPE_PINGCLIENT : /* Not supported */
      default :
        __NOP(); /* result already set to false */
        break;
    }
  }
  else
  {
    PRINT_FORCE("%s: Performance test in progress! Wait its end before to retry!", p_cellular_app_trace)
  }

  return (result);
}

/**
  * @brief  Initialize all needed structures to support CellularApp features and call Cellular init
  * @param  -
  * @retval -
  */
void application_init(void)
{
  /**** BEGIN Cellular Application initialization ****/
  /* Initialize trace shortcut */
  p_cellular_app_trace = cellular_app_type_string[CELLULAR_APP_TYPE_CELLULARAPP];

  /* Initialize data ready status to false */
  cellular_app_data_is_ready = false;

#if (USE_CMD_CONSOLE == 1)
  /* Command initialization */
  cellular_app_cmd_init();
#endif /* USE_CMD_CONSOLE == 1 */

  /* EchoClt initialization */
  cellular_app_echoclient_init();

  /* PingClt initialization */
  cellular_app_pingclient_init();

#if (USE_RTC == 1)
  /* DateTime initialization */
  cellular_app_datetime_init();
#endif /* USE_RTC == 1 */

#if ((USE_DISPLAY == 1) || (USE_SENSORS == 1))
  /* UIClt initialization */
  cellular_app_uiclient_init();
#endif /* (USE_DISPLAY == 1) || (USE_SENSORS == 1) */
  /**** END   Cellular Application initialization ****/

  /**** BEGIN Cellular initialization ****/
  cellular_init();
  /**** END   Cellular initialization ****/
}

/**
  * @brief  Start all threads needed to activate CellularApp features and call Cellular start
  * @param  -
  * @retval -
  */
void application_start(void)
{
  /**** BEGIN Cellular Application start ****/
  /* Cellular initialization already done - Registration to services is OK */
  /* Registration to Cellular: only needs to know when IP is obtained */
  if (cellular_ip_info_cb_registration(cellular_app_ip_info_cb, (void *) NULL) != CELLULAR_SUCCESS)
  {
    CELLULAR_APP_ERROR(CELLULAR_APP_ERROR_CELLULARAPP, ERROR_FATAL)
  }

#if (USE_CMD_CONSOLE == 1)
  /* Command start */
  cellular_app_cmd_start();
#endif /* USE_CMD_CONSOLE == 1 */

  /* EchoClt start */
  cellular_app_echoclient_start();

  /* PingClt start */
  cellular_app_pingclient_start();

#if (USE_RTC == 1)
  /* DateTime start */
  cellular_app_datetime_start();
#endif /* USE_RTC == 1 */

#if ((USE_DISPLAY == 1) || (USE_SENSORS == 1))
  /* UIClt start */
  cellular_app_uiclient_start();
#endif /* (USE_DISPLAY == 1) || (USE_SENSORS == 1) */
  /**** END   Cellular Application start ****/

  /**** BEGIN Cellular start ****/
  cellular_start();
  /**** END   Cellular start ****/
}

#endif /* USE_CELLULAR_APP == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
