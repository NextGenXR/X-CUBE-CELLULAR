/**
  ******************************************************************************
  * @file    cellular_app_pingclient.c
  * @author  MCD Application Team
  * @brief   PingClt Cellular Application :
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

#include "cellular_app_pingclient.h"

#include "cellular_app.h"
#include "cellular_app_socket.h"
#include "cellular_app_trace.h"

#include "rtosal.h"

#include "com_sockets.h" /* includes all other includes */

#include "cellular_runtime_custom.h"

/* Private typedef -----------------------------------------------------------*/
/* Statistics during current session descriptor */
typedef struct
{
  uint8_t  count_ok; /* Count number ok */
  uint32_t rsp_min;  /* Response min    */
  uint32_t rsp_max;  /* Response max    */
  uint32_t rsp_tot;  /* Response sum    */
} pingclient_stat_desc_t;

/* Private defines -----------------------------------------------------------*/
#define PINGCLIENT_ITERATION_NB                   10U  /* Iteration number per session.        */
#define PINGCLIENT_ITERATION_PERIOD              500U  /* Period between each iteration in ms. */
#define PINGCLIENT_SND_RCV_TIMEOUT                10U  /* Send/Receive timeout in sec.         */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Trace shortcut */
static const uint8_t *p_cellular_app_pingclient_trace;

/* PingClt application        */
static cellular_app_desc_t cellular_app_pingclient;
/* PingClt application change */
static cellular_app_change_t cellular_app_pingclient_change;
/* PingClt socket variable    */
static cellular_app_socket_desc_t cellular_app_pingclient_socket;
/* PingClt socket change      */
static cellular_app_socket_change_t cellular_app_pingclient_socket_change;

/* Global variables ----------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void pingclient_thread(void *p_argument);

/* Public  functions  prototypes ---------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  PingClt thread
  * @note   Infinite loop PingClt body
  * @param  p_argument - unused
  * @retval -
  */
static void pingclient_thread(void *p_argument)
{
  UNUSED(p_argument);
  uint8_t counter;      /* Count the number of ping already done during the session */
  uint8_t iteration = PINGCLIENT_ITERATION_NB;
  com_sockaddr_in_t com_sockaddr_in;
  com_ping_rsp_t    ping_rsp;
  /* uint16_t msg_type; */  /* Msg type received from the queue */
  /* uint16_t msg_data; */  /* Msg id received from the queue   */
  uint32_t msg_queue;   /* Msg received from the queue      */
  int32_t result;
  /* Ping statistics */
  static pingclient_stat_desc_t  pingclient_stat;

  /* Specific treatment before the main loop: None */

  /* Thread main loop treatment */
  for (;;)
  {
    counter = 0U; /* Reset counter */

    /* Reset statistic */
    pingclient_stat.count_ok = 0U;           /* Count number ok */
    pingclient_stat.rsp_min  = 0xFFFFFFFFU;  /* Response min: set to the max. value to update it at first ping */
    pingclient_stat.rsp_max  = 0U;           /* Response max    */
    pingclient_stat.rsp_tot  = 0U;           /* Response sum    */

    /* Wait data is ready AND Process is requested to be On */
    while ((cellular_app_is_data_ready() == false)
           || (cellular_app_pingclient_change.process_status == false))
    {
      (void)rtosalMessageQueueGet(cellular_app_pingclient.queue_id, &msg_queue, RTOSAL_WAIT_FOREVER);
    }

    /* Update process status */
    cellular_app_pingclient.process_status = cellular_app_pingclient_change.process_status;
    if (cellular_app_pingclient.process_status == true)
    {
      PRINT_FORCE("\n\r<<< %s STARTED >>>\n\r", p_cellular_app_pingclient_trace)
    }

    /* Treatment while data is ready and Process is On */
    while ((cellular_app_is_data_ready() == true) && (cellular_app_pingclient.process_status == true))
    {
      if (counter == 0U)
      {
        /* Need to update distant ? */
        if ((cellular_app_pingclient_socket.distant.type != cellular_app_pingclient_socket_change.distant_type)
            || (cellular_app_pingclient_socket.distant.type == CELLULAR_APP_DISTANT_IPx_TYPE))
        {
          cellular_app_distant_update(cellular_app_pingclient_socket_change.distant_type,
                                      &(cellular_app_pingclient_socket.distant));
        }
        PRINT_FORCE("<<< %s Started on %d.%d.%d.%d>>>\n\r", p_cellular_app_pingclient_trace,
                    COM_IP4_ADDR1(&(cellular_app_pingclient_socket.distant.ip)),
                    COM_IP4_ADDR2(&(cellular_app_pingclient_socket.distant.ip)),
                    COM_IP4_ADDR3(&(cellular_app_pingclient_socket.distant.ip)),
                    COM_IP4_ADDR4(&(cellular_app_pingclient_socket.distant.ip)))

        com_sockaddr_in.sin_family      = (uint8_t)COM_AF_INET;
        com_sockaddr_in.sin_port        = (uint16_t)0U;
        com_sockaddr_in.sin_addr.s_addr = CELLULAR_APP_GET_DISTANTIP(cellular_app_pingclient_socket.distant.ip);
        com_sockaddr_in.sin_len         = (uint8_t)sizeof(com_sockaddr_in_t);
      }
      /* Reach the end of the session ? */
      if (counter < iteration)
      {
        /* If handle is invalid => request a handle */
        if ((cellular_app_pingclient_socket.id == COM_HANDLE_INVALID_ID)
            || (cellular_app_pingclient_socket.state == CELLULAR_APP_SOCKET_INVALID))
        {
          cellular_app_pingclient_socket.id = com_ping();
          if (cellular_app_pingclient_socket.id > COM_HANDLE_INVALID_ID)
          {
            cellular_app_pingclient_socket.state = CELLULAR_APP_SOCKET_CREATED;
          }
        }
        /* Do an iteration */
        if (cellular_app_pingclient_socket.id > COM_HANDLE_INVALID_ID)
        {
          result = com_ping_process(cellular_app_pingclient_socket.id,
                                    (const com_sockaddr_t *)&com_sockaddr_in, (int32_t)com_sockaddr_in.sin_len,
                                    PINGCLIENT_SND_RCV_TIMEOUT, &ping_rsp);

          /* Is Ping ok ? */
          if ((result == COM_ERR_OK) && (ping_rsp.status == COM_ERR_OK))
          {
            /* Ping is OK : display the result */
            PRINT_FORCE("%s: %d bytes from %d.%d.%d.%d: seq=%.2d time= %ldms ttl=%d",
                        p_cellular_app_pingclient_trace, ping_rsp.size,
                        COM_IP4_ADDR1(&(cellular_app_pingclient_socket.distant.ip)),
                        COM_IP4_ADDR2(&(cellular_app_pingclient_socket.distant.ip)),
                        COM_IP4_ADDR3(&(cellular_app_pingclient_socket.distant.ip)),
                        COM_IP4_ADDR4(&(cellular_app_pingclient_socket.distant.ip)),
                        counter + 1U, ping_rsp.time, ping_rsp.ttl)

            /* Update Ping statistics */
            if (ping_rsp.time > pingclient_stat.rsp_max)
            {
              pingclient_stat.rsp_max = ping_rsp.time;
            }
            if (ping_rsp.time < pingclient_stat.rsp_min)
            {
              pingclient_stat.rsp_min = ping_rsp.time;
            }
            if (pingclient_stat.rsp_tot < (0xFFFFFFFFU - ping_rsp.time))
            {
              pingclient_stat.rsp_tot += ping_rsp.time;
            }
            else
            {
              pingclient_stat.rsp_tot = 0xFFFFFFFFU;
            }
            pingclient_stat.count_ok++;
          }
          else
          {
            /* Ping is NOK - Display an error message for this ping */
            if (result == COM_SOCKETS_ERR_TIMEOUT)
            {
              PRINT_FORCE("%s: Timeout from %d.%d.%d.%d: seq=%.2d!", p_cellular_app_pingclient_trace,
                          COM_IP4_ADDR1(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR2(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR3(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR4(&(cellular_app_pingclient_socket.distant.ip)),
                          counter + 1U)
            }
            else
            {
              PRINT_FORCE("%s: ERROR from %d.%d.%d.%d: seq=%.2d!", p_cellular_app_pingclient_trace,
                          COM_IP4_ADDR1(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR2(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR3(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR4(&(cellular_app_pingclient_socket.distant.ip)),
                          counter + 1U)
            }
          }
          /* Next ping */
          counter++;
          (void)rtosalDelay(PINGCLIENT_ITERATION_PERIOD);
        }
        else
        {
          /* PingClt handle not received */
          PRINT_INFO("%s: low-level not ready! Wait before to try again!", p_cellular_app_pingclient_trace)
          /* Wait to try again */
          (void)rtosalDelay(1000U);
        }
      }
      /* Display the result if Session completed, stopped or interrupted */
      bool is_data_ready = cellular_app_is_data_ready();
      if ((counter == iteration) /* Session completed */
          || (cellular_app_pingclient_change.process_status == false) /* Session stopped before its end   */
          || (is_data_ready == false)) /* Session interrupted because data is no more ready   */
      {
        if (counter > 0U) /* and at least one Ping has been send */
        {
          /* Display the result even if it is partial */
          if ((pingclient_stat.count_ok != 0U)             /* at least one ping ok during the session */
              && (pingclient_stat.rsp_tot != 0xFFFFFFFFU)) /* and it was possible to count the total response */
          {
            PRINT_FORCE("%s: --- %d.%d.%d.%d : min/avg/max = %ld/%ld/%ld ms ok = %d/%d ---",
                        p_cellular_app_pingclient_trace,
                        COM_IP4_ADDR1(&(cellular_app_pingclient_socket.distant.ip)),
                        COM_IP4_ADDR2(&(cellular_app_pingclient_socket.distant.ip)),
                        COM_IP4_ADDR3(&(cellular_app_pingclient_socket.distant.ip)),
                        COM_IP4_ADDR4(&(cellular_app_pingclient_socket.distant.ip)),
                        pingclient_stat.rsp_min, (pingclient_stat.rsp_tot / pingclient_stat.count_ok),
                        pingclient_stat.rsp_max, pingclient_stat.count_ok, counter)
            TRACE_VALID("@valid@:ping:state:%d/%d\n\r", pingclient_stat.count_ok, counter)
          }
          else
          {
            if (pingclient_stat.count_ok == 0U) /* all pings of the session nok */
            {
              PRINT_FORCE("%s: --- %d.%d.%d.%d : min/avg/max = 0/0/0 ms ok = 0/%d ---",
                          p_cellular_app_pingclient_trace,
                          COM_IP4_ADDR1(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR2(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR3(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR4(&(cellular_app_pingclient_socket.distant.ip)),
                          counter)
              TRACE_VALID("@valid@:ping:state:0/%d\n\r", counter)
            }
            else /* some/all pings ok but total response maximum reached */
            {
              PRINT_FORCE("%s: --- %d.%d.%d.%d : min/avg/max = %ld/Overrun/%ld ms ok = %d/%d ---",
                          p_cellular_app_pingclient_trace,
                          COM_IP4_ADDR1(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR2(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR3(&(cellular_app_pingclient_socket.distant.ip)),
                          COM_IP4_ADDR4(&(cellular_app_pingclient_socket.distant.ip)),
                          pingclient_stat.rsp_min, pingclient_stat.rsp_max, pingclient_stat.count_ok, counter)
              TRACE_VALID("@valid@:ping:state:%d/%d\n\r", pingclient_stat.count_ok, counter)
            }
          }
          /* Session goes until its end with no issue ? */
          if ((counter == iteration) && (cellular_app_pingclient_change.process_status == true))
          {
            PRINT_FORCE("<<< %s Completed >>>", p_cellular_app_pingclient_trace)
          }
          else
          {
            PRINT_FORCE("<<< %s Stopped before the end >>>", p_cellular_app_pingclient_trace)
          }
        }
        /* Release Ping handle */
        if (cellular_app_pingclient_socket.state != CELLULAR_APP_SOCKET_INVALID)
        {
          cellular_app_pingclient_socket.closing = true;
          cellular_app_socket_close(CELLULAR_APP_TYPE_PINGCLIENT, 1U, (&cellular_app_pingclient_socket));
        }
        /* Stop the session */
        cellular_app_pingclient.process_status = false;
        cellular_app_pingclient_change.process_status = false;
      }
    }

    /* Data is no more ready or Process is off - force a close when data will be back */
    if (cellular_app_pingclient_socket.state != CELLULAR_APP_SOCKET_INVALID)
    {
      PRINT_INFO("%s: Data not ready or Process stopped! Closing the socket!", p_cellular_app_pingclient_trace)
      cellular_app_pingclient_socket.closing = true;
      /* If data is ready try to close the socket */
      do
      {
        if (cellular_app_is_data_ready() == true)
        {
          PRINT_INFO("%s: Data ready! Closing the session!", p_cellular_app_pingclient_trace)
          cellular_app_socket_close(CELLULAR_APP_TYPE_PINGCLIENT, 1U, &cellular_app_pingclient_socket);
        }
        else
        {
          PRINT_INFO("%s: Data not ready! Wait to close the socket properly!", p_cellular_app_pingclient_trace)
          (void)rtosalDelay(5000U);
        }
      } while (cellular_app_pingclient_socket.state != CELLULAR_APP_SOCKET_INVALID);
    }
  }
}

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Send a message to PingClt
  * @param  queue_msg - Message to send
  * @retval -
  */
bool cellular_app_pingclient_send_msg(uint32_t queue_msg)
{
  bool result = true;
  rtosalStatus status;

  /* A message has to be send */
  status = rtosalMessageQueuePut(cellular_app_pingclient.queue_id, queue_msg, 0U);
  if (status != osOK)
  {
    result = false;
    PRINT_FORCE("%s: ERROR CellularApi Msg Put Type:%d Id:%d - status:%d!", p_cellular_app_pingclient_trace,
                GET_CELLULAR_APP_MSG_TYPE(queue_msg), GET_CELLULAR_APP_MSG_ID(queue_msg), status)
  }

  return (result);
}

/**
  * @brief  Get status of PingClt application
  * @retval cellular_app_process_status_t - PingClt application process status
  */
cellular_app_process_status_t cellular_app_pingclient_get_status(void)
{
  cellular_app_process_status_t result;

  result = cellular_app_get_status(cellular_app_pingclient.process_status,
                                   cellular_app_pingclient_change.process_status);

  return (result);
}

/**
  * @brief  Set status of PingClt application
  * @param  process_status    - process status new value to set inactive/active
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_pingclient_set_status(bool process_status)
{
  bool result = false;
  cellular_app_process_status_t process_status_tmp;
  rtosalStatus status;

  process_status_tmp = cellular_app_get_status(cellular_app_pingclient.process_status,
                                               cellular_app_pingclient_change.process_status);
  /* Only one change at a time */
  if (((process_status == true) && (process_status_tmp == CELLULAR_APP_PROCESS_OFF))
      || ((process_status == false) && (process_status_tmp == CELLULAR_APP_PROCESS_ON)))
  {
    uint32_t queue_msg = 0U;

    SET_CELLULAR_APP_MSG_TYPE(queue_msg, CELLULAR_APP_PROCESS_MSG);
    SET_CELLULAR_APP_MSG_ID(queue_msg, CELLULAR_APP_PROCESS_CHANGE_ID);

    cellular_app_pingclient_change.process_status = process_status;
    status = rtosalMessageQueuePut(cellular_app_pingclient.queue_id, queue_msg, 0U);
    if (status != osOK)
    {
      /* Restore old value */
      cellular_app_pingclient_change.process_status = !process_status;
      PRINT_FORCE("%s: ERROR SetStatus Msg Put Type:%d Id:%d - status:%d!", p_cellular_app_pingclient_trace,
                  GET_CELLULAR_APP_MSG_TYPE(queue_msg), GET_CELLULAR_APP_MSG_ID(queue_msg), status)
    }
    else
    {
      result = true;
    }
  }
  else
  {
    PRINT_FORCE("%s: Oonly one process change at a time!", p_cellular_app_pingclient_trace)
  }

  return (result);
}

/**
  * @brief  Change distant of PingClt application
  * @param  index             - PingClt index to change (must be 0U)
  * @param  distant_type      - distant type value
  * @param  p_distantip       - distant ip value  (supported for PingClt only)
  * @param  distantip_len     - distant ip length (supported for PingClt only)
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_pingclient_distant_change(uint8_t index, cellular_app_distant_type_t distant_type,
                                            uint8_t *p_distantip, uint32_t distantip_len)
{
  bool result = false;
  cellular_app_process_status_t process_status;

  if (index == 0U)
  {
    process_status = cellular_app_get_status(cellular_app_pingclient.process_status,
                                             cellular_app_pingclient_change.process_status);
    result = cellular_app_distant_change(CELLULAR_APP_TYPE_PINGCLIENT, index, process_status,
                                         distant_type, p_distantip, distantip_len,
                                         &cellular_app_pingclient_socket.distant,
                                         &cellular_app_pingclient_socket_change);
  }

  return (result);
}
/**
  * @brief  Display PingClt status
  * @param  -
  * @retval -
  */
void cellular_app_pingclient_display_status(void)
{
  cellular_app_process_status_t process_status;

  /* Only one instance of PingClt */
  PRINT_FORCE("PingClt Status:")
  for (uint8_t i = CELLULAR_APP_DISTANT_IP1_TYPE; i <= CELLULAR_APP_DISTANT_IPx_TYPE; i++)
  {
    /* Check the validity of the distant ip */
    if ((cellular_app_distant_ip[i][0] != 0U) && (cellular_app_distant_ip[i][1] != 0U)
        && (cellular_app_distant_ip[i][2] != 0U) && (cellular_app_distant_ip[i][3] != 0U))
    {
      /* IP is valid - is it the Dynamic IP ? */
      if (i != CELLULAR_APP_DISTANT_IPx_TYPE)
      {
        /* it is IP1 or IP2 */
        PRINT_FORCE("IP%d: %d.%d.%d.%d", (i - CELLULAR_APP_DISTANT_IP1_TYPE + 1U),
                    cellular_app_distant_ip[i][0], cellular_app_distant_ip[i][1],
                    cellular_app_distant_ip[i][2], cellular_app_distant_ip[i][3])
      }
      else
      {
        /* it is Dynamic IP */
        PRINT_FORCE("Dynamic IP: %d.%d.%d.%d",
                    cellular_app_distant_ip[i][0], cellular_app_distant_ip[i][1],
                    cellular_app_distant_ip[i][2], cellular_app_distant_ip[i][3])
      }
    }
    else /* IP1 or IP2 is invalid or Dynamic IP not yet defined */
    {
      if (i != CELLULAR_APP_DISTANT_IPx_TYPE)
      {
        /* IPx invalid */
        PRINT_FORCE("IP%d: NOT valid, check Ping IP parameters !", (i - CELLULAR_APP_DISTANT_IP1_TYPE + 1U))
      }
      else
      {
        /* Dynamic IP undefined */
        PRINT_FORCE("Dynamic IP: UNDEFINED (use command 'ping ddd.ddd.ddd.ddd' to define/start it)")
      }
    }
  }
  /* Ping index status */
  if (cellular_app_pingclient_socket.distant.type != CELLULAR_APP_DISTANT_IPx_TYPE)
  {
    PRINT_FORCE("Ping index on: IP%d",
                (cellular_app_pingclient_socket.distant.type - CELLULAR_APP_DISTANT_IP1_TYPE + 1U))
  }
  else
  {
    PRINT_FORCE("Ping index on: Dynamic IP")
  }

  /* Process status */
  process_status = cellular_app_get_status(cellular_app_pingclient.process_status,
                                           cellular_app_pingclient_change.process_status);
  PRINT_FORCE("Status: %s", cellular_app_process_status_string[process_status])
}

/**
  * @brief  Initialize all needed structures to support PingClt feature
  * @param  -
  * @retval -
  */
void cellular_app_pingclient_init(void)
{
  /* Initialize trace shortcut */
  p_cellular_app_pingclient_trace = cellular_app_type_string[CELLULAR_APP_TYPE_PINGCLIENT];

  /* Application Id Initialization */
  cellular_app_pingclient.app_id = 0U;
  /* Process Status Initialization */
  cellular_app_pingclient.process_status = false;
  /* Processing Period Initialization */
  cellular_app_pingclient.process_period = 0U; /* Unused */
  /* Thread Id Initialization */
  cellular_app_pingclient.thread_id = NULL;
  /* Queue Id Initialization/Creation */
  cellular_app_pingclient.queue_id = rtosalMessageQueueNew(NULL, CELLULAR_APP_QUEUE_SIZE);

  /* Change Structure Initialization */
  cellular_app_pingclient_change.process_status = cellular_app_pingclient.process_status;
  cellular_app_pingclient_change.process_period = cellular_app_pingclient.process_period;

  /* Socket Generic Initialization : state, closing, protocol, id */
  cellular_app_socket_init(&cellular_app_pingclient_socket, &cellular_app_pingclient_socket_change);

  /* Socket Initialization Specific Parameters */
  /* Send Buffer Length Initialization */
  cellular_app_pingclient_socket.snd_buffer_len = 0U; /* Unusued */
  /*  Timeout Initialization */
  cellular_app_pingclient_socket.snd_rcv_timeout = PINGCLIENT_SND_RCV_TIMEOUT;
  /* Send/Receive Buffers Initialization */
  cellular_app_pingclient_socket.p_snd_buffer = NULL; /* No send     buffer needed */
  cellular_app_pingclient_socket.p_rcv_buffer = NULL; /* No received buffer needed */

  /* Distant Initialization - Default value CELLULAR_APP_DISTANT_IP1_TYPE */
  cellular_app_distant_update(CELLULAR_APP_DISTANT_IP1_TYPE, &(cellular_app_pingclient_socket.distant));

  /* NFM Initialization */
  (void)memset((void *) & (cellular_app_pingclient_socket.nfm), 0, sizeof(cellular_app_socket_nfm_desc_t));
  /* Statistic Initialization */
  (void)memset((void *) & (cellular_app_pingclient_socket.stat), 0, sizeof(cellular_app_socket_stat_desc_t));

  /* Change Structure Initialization */
  cellular_app_pingclient_socket_change.snd_buffer_len =  cellular_app_pingclient_socket.snd_buffer_len;
  cellular_app_pingclient_socket_change.distant_type = cellular_app_pingclient_socket.distant.type;

  /* Check Initialization is ok */
  if (cellular_app_pingclient.queue_id == NULL)
  {
    CELLULAR_APP_ERROR(CELLULAR_APP_ERROR_PINGCLIENT, ERROR_FATAL)
  }
}

/**
  * @brief  Start PingClt thread
  * @param  -
  * @retval -
  */
void cellular_app_pingclient_start(void)
{
  /* Application Initialization */
  uint8_t thread_name[CELLULAR_APP_THREAD_NAME_MAX];
  uint32_t len;

  /* Thread Name Generation */
  len = crs_strlen((const uint8_t *)"PingClt");
  /* '+1' to copy '\0' */
  (void)memcpy(thread_name, "PingClt", CELLULAR_APP_MIN((len + 1U), CELLULAR_APP_THREAD_NAME_MAX));

  /* Thread Creation */
  cellular_app_pingclient.thread_id = rtosalThreadNew((const rtosal_char_t *)thread_name, (os_pthread)pingclient_thread,
                                                      PINGCLIENT_THREAD_PRIO, PINGCLIENT_THREAD_STACK_SIZE, NULL);
  /* Check Creation is ok */
  if (cellular_app_pingclient.thread_id == NULL)
  {
    CELLULAR_APP_ERROR((CELLULAR_APP_ERROR_PINGCLIENT + (int32_t)PINGCLIENT_THREAD_NUMBER), ERROR_FATAL)
  }
}

#endif /* USE_CELLULAR_APP == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
