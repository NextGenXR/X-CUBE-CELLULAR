/**
  ******************************************************************************
  * @file    cellular_app_echoclient.c
  * @author  MCD Application Team
  * @brief   EchoClt Cellular Application :
  *          - Create and Manage X instances of EchoClt
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

#include "cellular_app_echoclient.h"

#include "cellular_app.h"
#include "cellular_app_socket.h"
#include "cellular_app_trace.h"
#if (USE_RTC == 1)
#include "cellular_app_datetime.h"
#endif /* USE_RTC == 1 */

#include "rtosal.h"

#include "com_sockets.h" /* includes all other includes */

#include "cellular_control_api.h"
#include "cellular_runtime_custom.h"

/* Private typedef -----------------------------------------------------------*/
/* Performance result structure */
typedef struct
{
  uint16_t iter_ok;
  uint32_t total_time;
} echoclient_performance_result_t; /* ToDo: only used for EchoClt */

/* Private defines -----------------------------------------------------------*/
#define ECHOCLIENT_DEFAULT_PROCESS_PERIOD      (uint32_t)(5000)  /* Default period between two send in ms. */
#define ECHOCLIENT_PROCESS_PERIOD_MIN          (uint32_t)(2000)  /* Minimum period between two send in ms. */
#define ECHOCLIENT_SND_RCV_TIMEOUT             (uint16_t)(20000) /* Send/Receive timeout in ms.            */
#if (USE_RTC == 1)
#define ECHOCLIENT_SND_RCV_MIN_SIZE            (uint16_t)(21)    /* %02d:%02d:%02d - %04d/%02d/%02d */
#else /* USE_RTC != 1 */
#define ECHOCLIENT_SND_RCV_MIN_SIZE            (uint16_t)(16)
#endif /* USE_RTC == 1 */

#define ECHOCLIENT_SND_RCV_MAX_SIZE            (uint16_t)(1500)  /* Send/Receive buffer size max in bytes. */

#define ECHOCLIENT_NFM_ERROR_LIMIT_NB_MAX      (uint8_t)(5)      /* Maximum number of consecutive errors
                                                                    before to start NFM feature */

#if (USE_RTC == 1)
/* Important : check echoclient_obtain_datetime() for re-ordering Date Time parameter */
/* Re-ordering date-time is supported with mbedcloudtesting server */
#define ECHOCLIENT_DISTANT_MBED_DATETIME_PORT       ((uint16_t)13U)
/* Re-ordering date-time is not supported with u-blox server */
#define ECHOCLIENT_DISTANT_UBLOX_DATETIME_PORT      (CELLULAR_APP_DISTANT_UNKNOWN_PORT)
/* Re-ordering date-time is not supported with local server */
#define ECHOCLIENT_DISTANT_LOCAL_DATETIME_PORT      (CELLULAR_APP_DISTANT_UNKNOWN_PORT)
#endif /* USE_RTC == 1 */

/* Private variables ---------------------------------------------------------*/
/* Trace shortcut */
static const uint8_t *p_cellular_app_echoclient_trace;

/* EchoClt application array        */
static cellular_app_desc_t cellular_app_echoclient[ECHOCLIENT_THREAD_NUMBER];
/* EchoClt application change array */
static cellular_app_change_t cellular_app_echoclient_change[ECHOCLIENT_THREAD_NUMBER];
/* EchoClt socket variable array    */
static cellular_app_socket_desc_t cellular_app_echoclient_socket[ECHOCLIENT_THREAD_NUMBER];
/* EchoClt socket change array      */
static cellular_app_socket_change_t cellular_app_echoclient_socket_change[ECHOCLIENT_THREAD_NUMBER];

/* EchoClt application index; index 0 request date time and execute performance test */
static uint8_t cellular_app_echoclient_index;
/* EchoClt mutex to protect access to cellular_app_echoclient_index variable */
static osMutexId cellular_app_echoclient_index_mutex_handle;

/* Current status of EchoClt performance */
static bool cellular_app_echoclient_perf_start; /* false: inactive, true: active */
static uint16_t cellular_app_echoclient_perf_iter_nb;

#if (USE_RTC == 1)
/* Set or not Date/Time => a specific request will be send to the EchoClt distant server */
static bool cellular_app_echoclient_set_datetime; /* false: date time has not to be requested,
                                                   * true:  date time has to be requested,
                                                   * default value : see echoclient_init() */
#endif /* USE_RTC == 1 */

/* Private macro -------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static uint16_t echoclient_format_buffer(uint16_t length, uint8_t *p_buffer);
static bool echoclient_process(uint8_t index, cellular_app_socket_desc_t *const p_socket, uint32_t *p_snd_rcv_time,
                               cellular_app_socket_change_t *p_change);
static void echoclient_performance_iteration(cellular_app_socket_desc_t *const p_socket,
                                             uint16_t iteration_nb, uint16_t trame_size,
                                             echoclient_performance_result_t *p_perf_result);
static void echoclient_performance(cellular_app_socket_desc_t *const p_socket);
static bool echoclient_is_blocked(uint8_t index);
static uint8_t echoclient_get_app_index(void);
static void echoclient_thread(void *p_argument);
#if (USE_RTC == 1)
static void echoclient_analyze_datetime_str(uint8_t rcv_len, uint8_t const *p_rcv);
static void echoclient_obtain_datetime(cellular_app_socket_desc_t *p_socket);
#endif /* USE_RTC == 1 */

/* Public  functions  prototypes ---------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

#if (USE_RTC)
/**
  * @brief  Analyze Date and Time string from a EchoClt distant server
  * @note   Reorder date/time network and update internal date/time
  * @param  rcv_len - length of the string provided by the distant server
  * @param  p_rcv   - string provided by the distant server
  * @retval -
  */
static void echoclient_analyze_datetime_str(uint8_t rcv_len, uint8_t const *p_rcv)
{
  /* Internal Date and Time must be ordered like Day MonthDay Month Year Hour Minutes Seconds */
  /* eg:  Mon 15 Nov 2021 13:50:10 - Day and Month must be the first 3 characters in English */
  /* Received answer is Mon Nov 15 13:50:10 2021 */
  /* Day:0 MonthDay:1 Month:2 Year:3 Time:4 */
  uint8_t distant_datetime_order_received[5] = {0, 2, 1, 4, 3};

  uint8_t argv[5];
  uint8_t len[5];
  uint8_t datetime_str[30];
  uint8_t offset;
  uint8_t i = 0U;
  uint8_t j = 0U;
  uint8_t start = 0U;
  bool next = true;
  cellular_app_datetime_t datetime;

  while ((i < rcv_len) && (j < 5U))
  {
    if ((p_rcv[i] == (uint8_t)' ') || (p_rcv[i] == (uint8_t)'\n') || (p_rcv[i] == (uint8_t)'\r'))
    {
      if (next == false)
      {
        len[j] = i - start;
        j++;
      }
      next = true;
    }
    else
    {
      if (next == true)
      {
        argv[j] = i;
        next = false;
        start = i;
      }
    }
    i++;
  }

  /* Find enough parameters ? */
  if (j == 5U)
  {
    offset = 0U;
    for (uint8_t k = 0U; k < j; k++)
    {
      (void)memcpy(&datetime_str[offset], &p_rcv[argv[distant_datetime_order_received[k]]],
                   len[distant_datetime_order_received[k]]);
      offset += len[distant_datetime_order_received[k]];
      datetime_str[offset] = (uint8_t)' ';
      offset++;
    }
    /* Replace last ' ' by '\0' */
    offset--;
    datetime_str[offset] = (uint8_t)'\0';
    if (cellular_app_datetime_str_convert(offset, (uint8_t const *)&datetime_str[0], &datetime) == true)
    {
      if (cellular_app_datetime_set(&datetime) == true)
      {
        PRINT_FORCE("%s 1: Update date and time OK", p_cellular_app_echoclient_trace)
      }
      else
      {
        PRINT_FORCE("%s 1: Update date and time NOK!", p_cellular_app_echoclient_trace)
      }
    }
    else
    {
      PRINT_FORCE("%s 1: Update date and time NOK! Conversion issue!", p_cellular_app_echoclient_trace)
    }
  }
  else
  {
    PRINT_FORCE("%s 1: Update date and time NOK! Not enough information!", p_cellular_app_echoclient_trace)
  }
}

/**
  * @brief  Obtain Date and Time from a EchoClt distant server
  * @note   Open, read date & time network then Close a socket
  *         Update internal date & time according to format: %02ld/%02ld/%04ld - %02ld:%02ld:%02ld:dd/mm/yyyy - hh/mm/ss
  * @param  p_socket - pointer on the socket to use
  * @retval -
  */
static void echoclient_obtain_datetime(cellular_app_socket_desc_t *p_socket)
{
  uint16_t distant_datetime_port;

  /* Set Distant Date and Time Port according to the server port value */
  switch (p_socket->distant.type)
  {
    case CELLULAR_APP_DISTANT_MBED_TYPE :
      distant_datetime_port = ECHOCLIENT_DISTANT_MBED_DATETIME_PORT;
      break;
    case CELLULAR_APP_DISTANT_UBLOX_TYPE :
      distant_datetime_port = ECHOCLIENT_DISTANT_UBLOX_DATETIME_PORT;
      break;
    case CELLULAR_APP_DISTANT_LOCAL_TYPE :
      distant_datetime_port = ECHOCLIENT_DISTANT_LOCAL_DATETIME_PORT;
      break;
    default :
      distant_datetime_port = CELLULAR_APP_DISTANT_UNKNOWN_PORT;
      break;
  }
  /* Is re-ordering date-time known ? */
  if (distant_datetime_port != CELLULAR_APP_DISTANT_UNKNOWN_PORT)
  {
    int32_t ret;

    /* If distantip to contact is unknown, call DNS resolver service */
    if (CELLULAR_APP_GET_DISTANTIP(p_socket->distant.ip) == (uint32_t)0U)
    {
      (void)cellular_app_distant_check(CELLULAR_APP_TYPE_ECHOCLIENT, 1U, &(p_socket->distant));
      /* Whatever the cellular_app_distant_check() result, no fault counter to increase */
    }

    /* If distantip is known, send the request to obtain the dat time */
    if (CELLULAR_APP_GET_DISTANTIP(p_socket->distant.ip) != (uint32_t)0U)
    {
      bool connected;
      connected = false;

      /* Create socket */
      p_socket->id = com_socket(COM_AF_INET, COM_SOCK_STREAM, COM_IPPROTO_TCP);

      PRINT_INFO("%s 1: Date and time request to distant", p_cellular_app_echoclient_trace)

      /* Connect to the distant server */
      if (p_socket->id > COM_SOCKET_INVALID_ID)
      {
        com_sockaddr_in_t address;
        address.sin_family      = (uint8_t)COM_AF_INET;
        address.sin_port        = COM_HTONS(distant_datetime_port);
        address.sin_addr.s_addr = CELLULAR_APP_GET_DISTANTIP(p_socket->distant.ip);

        if (com_connect(p_socket->id, (com_sockaddr_t const *)&address, (int32_t)sizeof(com_sockaddr_in_t))
            == COM_SOCKETS_ERR_OK)
        {
          /* Connection is ok */
          connected = true;
        }
      }

      /* Is Connection ok ? */
      if (connected == true)
      {
        /* Send a trame e.g time to receive the date and time from the distant server */
        ret = com_send(p_socket->id, (const com_char_t *)"time", 4, COM_MSG_WAIT);
        /* Is send ok ? */
        if (ret > 0)
        {
          /* Send ok, wait for the answer */
          uint8_t receive[75];

          (void)memset(&receive, (int8_t)'\0', sizeof(receive));
          ret = com_recv(p_socket->id, (com_char_t *)&receive[0], (int32_t)sizeof(receive), COM_MSG_WAIT);
          /* If receive is ok ? */
          if (ret > 0)
          {
            /* Analyze the answer */
            echoclient_analyze_datetime_str((uint8_t)ret, &receive[0]);
          }
          else
          {
            PRINT_FORCE("%s 1: Date and time error at reception!", p_cellular_app_echoclient_trace)
          }
        }
        else
        {
          PRINT_FORCE("%s 1: Date and time error at send!", p_cellular_app_echoclient_trace)
        }
      }
      else
      {
        PRINT_FORCE("%s 1: Date and time error at connection!", p_cellular_app_echoclient_trace)
      }

      /* Close the socket - only try one time */
      if (p_socket->id > COM_SOCKET_INVALID_ID)
      {
        (void)com_closesocket(p_socket->id);
      }
    }
  }
}
#endif /* USE_RTC == 1 */

/**
  * @brief  Format buffer to send by EchoClt
  * @note   format of the buffer to send :
  *         time & date: %02ld:%02ld:%02ld - %04ld/%02ld/%02ld and complete with optional data
  * @param  length   - length to fill
  * @param  p_buffer - buffer to fill
  * @retval uint16_t - final length of the buffer to send
  */
static uint16_t echoclient_format_buffer(uint16_t length, uint8_t *p_buffer)
{
  /* Will add in the trame number 0 to 9 */
  uint8_t  number;
  /* Indice in the trame */
  uint16_t i;

  /* First number is 0 */
  number = 0U;

#if (USE_RTC == 1)
  cellular_app_datetime_t datetime;

  (void)cellular_app_datetime_get(&datetime);

  (void)sprintf((CRC_CHAR_t *)&p_buffer[0], "%02d:%02d:%02d - %04d/%02d/%02d",
                datetime.time.hour, datetime.time.min, datetime.time.sec,
                ((uint16_t)datetime.date.year + datetime.date.year_start),
                datetime.date.month, datetime.date.month_day);

  /* After setting potential default data, i must be updated */
  i = (uint16_t)crs_strlen(&p_buffer[0]);
#else /* USE_RTC == 0 */
  /* Date and time unknown */
  i = 0U;
#endif /* USE_RTC == 1 */

  /* Pad the rest of trame with ASCII number 0x30 0x31 ... 0x39 0x30 ...*/
  while (i < length)
  {
    /* Add next number in the trame */
    p_buffer[i] = 0x30U + number;
    number++;
    /* If number = 10 restart to 0 */
    if (number == 10U)
    {
      number = 0U;
    }
    i++;
  }
  /* Last byte is '\0' */
  p_buffer[length] = (uint8_t)'\0';

  return ((uint16_t)crs_strlen(&p_buffer[0]));
}

/**
  * @brief  Process a EchoClt request
  * @note   Create, Send, Receive and Close socket
  * @param  index           - EchoClt index
  * @param  p_socket        - pointer on socket to use
  * @param  p_snd_rcv_time  - pointer to save snd/rcv transaction time (for performance test)
  * @param  p_socket_change - pointer on socket change
  * @retval bool            - false/true : process NOK/OK
  */
static bool echoclient_process(uint8_t index, cellular_app_socket_desc_t *p_socket, uint32_t *p_snd_rcv_time,
                               cellular_app_socket_change_t *p_socket_change)
{
  bool result;
  int32_t read_buf_size;
  int32_t address_len;
  com_sockaddr_in_t address;
  uint32_t time_begin;
  uint32_t time_end;
  uint8_t *p_welcome_msg = NULL;

  result = false;
  address_len = (int32_t)sizeof(address);

  /* Increase process counter */
  p_socket->stat.process_counter++;

  /* Obtain a new socket if parameters changed or continue to use the actual one */
  if (cellular_app_socket_obtain(CELLULAR_APP_TYPE_ECHOCLIENT, index, p_socket, &p_welcome_msg, p_socket_change)
      == true)
  {
    if (((p_socket->state == CELLULAR_APP_SOCKET_CONNECTED) || (p_socket->state == CELLULAR_APP_SOCKET_CREATED))
        && (p_welcome_msg != NULL)) /* before to send anything a welcome msg will be received */
    {
      /* Socket is in good state to continue, welcome msg is expected */
      p_socket->state = CELLULAR_APP_SOCKET_WAITING_RSP;

      /* Read welcome msg according to the socket protocol */
      if ((p_socket->protocol == CELLULAR_APP_SOCKET_TCP_PROTO)
          || (p_socket->protocol == CELLULAR_APP_SOCKET_UDP_PROTO))
      {
        PRINT_INFO("%s %d: Waiting first msg with rcv", p_cellular_app_echoclient_trace, index)
        read_buf_size = com_recv(p_socket->id, p_socket->p_rcv_buffer, (int32_t)ECHOCLIENT_SND_RCV_MAX_SIZE,
                                 COM_MSG_WAIT);
      }
      else
      {
        PRINT_INFO("%s %d: Waiting first msg with rcvfrom", p_cellular_app_echoclient_trace, index)
        read_buf_size = com_recvfrom(p_socket->id, p_socket->p_rcv_buffer, (int32_t)ECHOCLIENT_SND_RCV_MAX_SIZE,
                                     COM_MSG_WAIT, (com_sockaddr_t *)&address, &address_len);
      }

      /* Check welcome msg is ok or not */
      if ((read_buf_size == (int32_t)crs_strlen(p_welcome_msg))
          && (memcmp((const void *)p_socket->p_rcv_buffer, (const void *)p_welcome_msg, (size_t)read_buf_size) == 0))
      {
        if ((p_socket->protocol == CELLULAR_APP_SOCKET_TCP_PROTO)
            || (p_socket->protocol == CELLULAR_APP_SOCKET_UDP_PROTO))
        {
          p_socket->state = CELLULAR_APP_SOCKET_CONNECTED;
        }
        else
        {
          p_socket->state = CELLULAR_APP_SOCKET_CREATED;
        }
        PRINT_INFO("%s %d: First msg OK", p_cellular_app_echoclient_trace, index)
      }
      else
      {
        /* Welcome msg is nok, the socket will be closed */
        PRINT_INFO("%s %d: First msg NOK! Closing the socket!", p_cellular_app_echoclient_trace, index)
        p_socket->closing = true;
        p_socket->state = CELLULAR_APP_SOCKET_CLOSING;
      }
    }

    /* Is it ok to continue the process ? */
    if ((p_socket->state == CELLULAR_APP_SOCKET_CONNECTED) || (p_socket->state == CELLULAR_APP_SOCKET_CREATED))
    {
      int32_t ret;

      /* Send data according to the socket protocol */
      if ((p_socket->protocol == CELLULAR_APP_SOCKET_TCP_PROTO)
          || (p_socket->protocol == CELLULAR_APP_SOCKET_UDP_PROTO))
      {
        PRINT_DBG("%s %d: Data send in progress", p_cellular_app_echoclient_trace, index)
        time_begin = HAL_GetTick();
        ret = com_send(p_socket->id, (const com_char_t *)p_socket->p_snd_buffer, (int32_t)p_socket->snd_buffer_len,
                       COM_MSG_WAIT);
      }
      else
      {
        PRINT_DBG("%s %d: Data sendto in progress", p_cellular_app_echoclient_trace, index)
        address.sin_family      = (uint8_t)COM_AF_INET;
        address.sin_port        = COM_HTONS(p_socket->distant.port);
        address.sin_addr.s_addr = CELLULAR_APP_GET_DISTANTIP(p_socket->distant.ip);

        time_begin = HAL_GetTick();
        ret = com_sendto(p_socket->id, p_socket->p_snd_buffer, (int32_t)p_socket->snd_buffer_len, COM_MSG_WAIT,
                         (const com_sockaddr_t *)&address, (int32_t)sizeof(com_sockaddr_in_t));
      }

      /* Data send ok ? */
      if (ret == (int32_t)p_socket->snd_buffer_len)
      {
        int32_t total_read_size = 0; /* data can be received in several packets */

        /* Data send ok, reset nfm counters, increase counters */
        p_socket->nfm.error_current_nb = 0U;
        p_socket->nfm.index = 0U;
        p_socket->stat.send.ok++;
        PRINT_INFO("%s %d: Data send OK", p_cellular_app_echoclient_trace, index)

        p_socket->state = CELLULAR_APP_SOCKET_WAITING_RSP;

        /* Receive response according to the protocol socket */
        if ((p_socket->protocol == CELLULAR_APP_SOCKET_TCP_PROTO)
            || (p_socket->protocol == CELLULAR_APP_SOCKET_UDP_PROTO))
        {
          bool exit = false;

          PRINT_DBG("%s %d: Data recv waiting", p_cellular_app_echoclient_trace, index)
          do
          {
            read_buf_size = com_recv(p_socket->id, &(p_socket->p_rcv_buffer[total_read_size]),
                                     ((int32_t)ECHOCLIENT_SND_RCV_MAX_SIZE - total_read_size), COM_MSG_WAIT);
            if (read_buf_size < 0) /* Error during data reception ? */
            {
              exit = true;
            }
            else /* Some data received */
            {
              total_read_size += read_buf_size;
              if (total_read_size < (int32_t)p_socket->snd_buffer_len)
              {
                PRINT_INFO("%s %d: Data recv expected more:%ld/%d", p_cellular_app_echoclient_trace, index,
                           total_read_size, p_socket->snd_buffer_len)
              }
            }
          } while ((total_read_size < (int32_t)p_socket->snd_buffer_len)             /* data still expected         */
                   && (((int32_t)ECHOCLIENT_SND_RCV_MAX_SIZE - total_read_size) > 0) /* memory still available      */
                   && (exit == false));                                              /* no error during transaction */

          time_end = HAL_GetTick(); /* end of reception */
          PRINT_DBG("%s %d: Rcv data exit", p_cellular_app_echoclient_trace, index)
        }
        else
        {
          bool exit = false;

          PRINT_DBG("%s %d: Data recvfrom waiting", p_cellular_app_echoclient_trace, index)
          do
          {
            read_buf_size = com_recvfrom(p_socket->id, &(p_socket->p_rcv_buffer[total_read_size]),
                                         ((int32_t)ECHOCLIENT_SND_RCV_MAX_SIZE - total_read_size), COM_MSG_WAIT,
                                         (com_sockaddr_t *)&address, &address_len);
#if (USE_TRACE_APPLICATION == 1U)
            /* Data received ? */
            if (read_buf_size > 0)
            {
              com_ip_addr_t distantip;
              CELLULAR_APP_SET_DISTANTIP(&distantip, address.sin_addr.s_addr);
              /* Data received, display the server IP */
              PRINT_INFO("%s %d: Data recvfrom %d.%d.%d.%d %d", p_cellular_app_echoclient_trace, index,
                         COM_IP4_ADDR1(&distantip), COM_IP4_ADDR2(&distantip),
                         COM_IP4_ADDR3(&distantip), COM_IP4_ADDR4(&distantip), COM_NTOHS(address.sin_port))
            }
#endif /* USE_TRACE_APPLICATION == 1U */
            if (read_buf_size < 0)
            {
              exit = true;
            }
            else
            {
              total_read_size += read_buf_size;
              if (total_read_size < (int32_t)p_socket->snd_buffer_len)
              {
                PRINT_DBG("%s %d: Data recvfrom expected more:%ld/%d", p_cellular_app_echoclient_trace, index,
                          total_read_size, p_socket->snd_buffer_len)
              }
            }
          } while ((total_read_size < (int32_t)p_socket->snd_buffer_len)             /* data still expected         */
                   && (((int32_t)ECHOCLIENT_SND_RCV_MAX_SIZE - total_read_size) > 0) /* memory still available      */
                   && (exit == false));                                              /* no error during transaction */

          time_end = HAL_GetTick(); /* end of reception */
          PRINT_DBG("%s %d: Data recvfrom exit", p_cellular_app_echoclient_trace, index)
        }
        /* all data send have been received ? */
        if ((int32_t)p_socket->snd_buffer_len == total_read_size)
        {
          /* Restore socket state at the end of exchange */
          if ((p_socket->protocol == CELLULAR_APP_SOCKET_TCP_PROTO)
              || (p_socket->protocol == CELLULAR_APP_SOCKET_UDP_PROTO))
          {
            p_socket->state = CELLULAR_APP_SOCKET_CONNECTED;
          }
          else
          {
            p_socket->state = CELLULAR_APP_SOCKET_CREATED;
          }

          /* Check that data received are ok */
          if (memcmp((const void *)p_socket->p_snd_buffer, (const void *)p_socket->p_rcv_buffer,
                     (size_t)p_socket->snd_buffer_len) == 0)
          {
            /* Data received are ok, increase counters */
            p_socket->stat.receive.ok++;
            PRINT_FORCE("%s %d: RSP received OK", p_cellular_app_echoclient_trace, index)
            result = true;
            if (p_snd_rcv_time != NULL)
            {
              *p_snd_rcv_time = time_end - time_begin;
            }
#if (USE_LOW_POWER == 1)
            PRINT_FORCE("%s %d: LowPower activated. Force socket close.", p_cellular_app_echoclient_trace, index)
            /* If low power is activated, force to close the socket */
            p_socket->closing = true;
#endif /* USE_LOW_POWER == 1 */
          }
          else
          {
            /* Data received are ko, increase fault counters, request close socket */
            p_socket->stat.receive.ko++;
            PRINT_FORCE("%s %d: RSP received NOK! memcmp error! Closing the socket!", p_cellular_app_echoclient_trace,
                        index)
            p_socket->closing = true;
          }
        }
        else /* read_buf != buf_snd_len */
        {
          /* Data received are ko, increase fault counters, request close socket  */
          p_socket->stat.receive.ko++;
          PRINT_FORCE("%s %d: RSP received NOK! error:%ld data:%ld/%d! Closing the socket!",
                      p_cellular_app_echoclient_trace, index, read_buf_size, total_read_size, p_socket->snd_buffer_len)
          p_socket->closing = true;
        }
      }
      else /* send data ret <=0 */
      {
        /* Send data is ko, increase fault counters, request close socket  */
        p_socket->stat.send.ko++;
        PRINT_FORCE("%s %d: SND NOK! error:%ld data:%d! Closing the socket!", p_cellular_app_echoclient_trace, index,
                    ret, p_socket->snd_buffer_len)
        p_socket->nfm.error_current_nb++;
        p_socket->closing = true;
      }
    }
    else
    {
      PRINT_FORCE("%s %d: Socket availability NOK!", p_cellular_app_echoclient_trace, index);
    }

    /* Is close socket requested ? */
    if ((p_socket->closing == true) || (p_socket->state == CELLULAR_APP_SOCKET_CLOSING))
    {
      /* Timeout to receive an answer or Closing has been requested */
      cellular_app_socket_close(CELLULAR_APP_TYPE_ECHOCLIENT, index, p_socket);
    }
  }
  return (result);
}

/**
  * @brief  Process a EchoClt performance test iteration loop (same snd buffer len)
  * @param  p_socket      - pointer on the socket to use
  * @param  iteration_nb  - iteration number to do
  * @param  trame_size    - trame size to send
  * @param  p_perf_result - pointer on performance result
  * @retval -
  */
static void echoclient_performance_iteration(cellular_app_socket_desc_t *const p_socket,
                                             uint16_t iteration_nb, uint16_t trame_size,
                                             echoclient_performance_result_t *p_perf_result)
{
  bool exit;
  uint16_t i;
  uint32_t time_snd_rcv;

  i = 0U;

  /* Update buffer with new data and potentially new length */
  p_socket->snd_buffer_len = echoclient_format_buffer(trame_size, p_socket->p_snd_buffer);
  if (p_socket->snd_buffer_len != 0U)
  {
    exit = false;

    while ((i < iteration_nb) && (exit == false))
    {
      if (echoclient_process(1U, p_socket, &time_snd_rcv, NULL) == true)
      {
        p_perf_result->iter_ok++;
        p_perf_result->total_time += time_snd_rcv;
      }
      else
      {
        /* Try next occurrence */
        __NOP();
        /* Or Exit */
        /* exit = true; */
      }
      i++;
    }
  }
}

/**
  * @brief  Process a EchoClt performance test
  * @param  p_socket - pointer on the socket to use
  * @retval -
  */
static void echoclient_performance(cellular_app_socket_desc_t *const p_socket)
{
#define ECHOCLIENT_PERFORMANCE_NB_ITER 8U
#define ECHOCLIENT_PERFORMANCE_TCP_TRAME_MAX ((uint16_t)(1400U))
#define ECHOCLIENT_PERFORMANCE_UDP_TRAME_MAX ((uint16_t)(1400U))
  uint16_t trame_size_in_TCP[ECHOCLIENT_PERFORMANCE_NB_ITER] =
  {
    16U, 32U, 64U, 128U, 256U, 512U, 1024U, ECHOCLIENT_PERFORMANCE_TCP_TRAME_MAX
  };
  uint16_t trame_size_in_UDP[ECHOCLIENT_PERFORMANCE_NB_ITER] =
  {
    16U, 32U, 64U, 128U, 256U, 512U, 1024U, ECHOCLIENT_PERFORMANCE_UDP_TRAME_MAX
  };
  uint16_t iter[ECHOCLIENT_PERFORMANCE_NB_ITER] =
  {
    1000U, 1000U, 1000U, 1000U, 200U, 100U, 100U, 100U
  };
  uint16_t iter_ok;
  uint16_t iter_total;
  echoclient_performance_result_t perf_result[ECHOCLIENT_PERFORMANCE_NB_ITER];
  uint16_t *p_trame_size;

  if (p_socket->protocol == CELLULAR_APP_SOCKET_TCP_PROTO)
  {
    p_trame_size = &trame_size_in_TCP[0];
  }
  else
  {
    p_trame_size = &trame_size_in_UDP[0];
  }

  /* perf_result initialization */
  for (uint8_t i = 0U; i < ECHOCLIENT_PERFORMANCE_NB_ITER; i++)
  {
    perf_result[i].iter_ok    = 0U;
    perf_result[i].total_time = 0U;
  }
  for (uint8_t i = 0U; i < ECHOCLIENT_PERFORMANCE_NB_ITER; i++)
  {
    if (cellular_app_echoclient_perf_iter_nb == 0U)
    {
      echoclient_performance_iteration(p_socket, iter[i], p_trame_size[i], &perf_result[i]);
    }
    else
    {
      echoclient_performance_iteration(p_socket, cellular_app_echoclient_perf_iter_nb, p_trame_size[i],
                                       &perf_result[i]);
    }
  }
  /* Close the performance test socket */
  cellular_app_socket_close(CELLULAR_APP_TYPE_ECHOCLIENT, 1U, p_socket);

  /* Display the result */
  if (CELLULAR_APP_GET_DISTANTIP(p_socket->distant.ip) == 0U)
  {
    /* Distant Server IP unknown */
    PRINT_FORCE("%s: Distant:%s Name:%s IP:Unknown Port:%d Protocol:%s", p_cellular_app_echoclient_trace,
                cellular_app_distant_string[p_socket->distant.type], p_socket->distant.p_name,
                p_socket->distant.port, cellular_app_protocol_string[p_socket->protocol])
  }
  else
  {
    /* Distant Server IP known */
    PRINT_FORCE("%s 1: Distant:%s Name:%s IP:%d.%d.%d.%d Port:%d Protocol:%s", p_cellular_app_echoclient_trace,
                cellular_app_distant_string[p_socket->distant.type], p_socket->distant.p_name,
                COM_IP4_ADDR1(&(p_socket->distant.ip)), COM_IP4_ADDR2(&(p_socket->distant.ip)),
                COM_IP4_ADDR3(&(p_socket->distant.ip)), COM_IP4_ADDR4(&(p_socket->distant.ip)),
                p_socket->distant.port, cellular_app_protocol_string[p_socket->protocol])
  }

  iter_ok = 0U;
  iter_total = 0U;

  PRINT_FORCE("%s: Size  IterMax  IterOK   Data(B)   Time(ms) Throughput(Byte/s)", p_cellular_app_echoclient_trace)

  for (uint8_t i = 0U; i < ECHOCLIENT_PERFORMANCE_NB_ITER; i++)
  {
    uint32_t data_snd_rcv = (uint32_t)(p_trame_size[i]) * 2U * (uint32_t)(perf_result[i].iter_ok);
    if (cellular_app_echoclient_perf_iter_nb == 0U)
    {
      PRINT_FORCE("%s: %5d\t%5d\t%5d\t%7ld   %7ld      %6ld", p_cellular_app_echoclient_trace,
                  p_trame_size[i], iter[i], perf_result[i].iter_ok, data_snd_rcv, perf_result[i].total_time,
                  ((data_snd_rcv * 1000U) / (perf_result[i].total_time)))
      /* Update trace valid data */
      iter_total += iter[i];
      iter_ok    += perf_result[i].iter_ok;
    }
    else
    {
      PRINT_FORCE("%s: %5d\t%5d\t%5d\t%7ld   %7ld      %6ld", p_cellular_app_echoclient_trace,
                  p_trame_size[i], cellular_app_echoclient_perf_iter_nb, perf_result[i].iter_ok, data_snd_rcv,
                  perf_result[i].total_time, ((data_snd_rcv * 1000U) / (perf_result[i].total_time)))
      /* Update trace valid data */
      iter_total += cellular_app_echoclient_perf_iter_nb;
      iter_ok    += perf_result[i].iter_ok;
    }
  }
  TRACE_VALID("@valid@:echoclient:stat:%d/%d\n\r", iter_ok, iter_total)
}

/**
  * @brief  Check if EchoClt is blocked or not(e.g data not ready, performance in progress, process status inactive)
  * @param  index - EchoClt index
  * @retval bool  - false/true - NOT blocked/blocked
  */
static bool echoclient_is_blocked(uint8_t index)
{
  bool result = false;
  /** Process is blocked if :
    * Data is not ready
    * || (index !=0) && ((Performance test is requested) || (Process status == false))
    * || (index = 0) && (    (Performance test is requested) && (Process status == true))
    *                        ( || (Performance test is not requested) && (Process status == false)))
    */
  if ((cellular_app_is_data_ready() == false)
      || ((index != 0U)
          && ((cellular_app_echoclient_perf_start == true)
              || (cellular_app_echoclient_change[index].process_status == false)))
      || ((index == 0U)
          && (((cellular_app_echoclient_perf_start == true)
               && (cellular_app_echoclient_change[index].process_status == true))
              || ((cellular_app_echoclient_perf_start == false)
                  && (cellular_app_echoclient_change[index].process_status == false)))))
  {
    result = true;
  }

  return (result);
}

/**
  * @brief  Get a EchoClt application index
  * @param  -
  * @retval uint8_t - EchoClt application index - 0xFF in case of error (too many call to this function)
  */
static uint8_t echoclient_get_app_index(void)
{
  uint8_t result = 0xFF; /* Impossible value */

  (void)rtosalMutexAcquire(cellular_app_echoclient_index_mutex_handle, RTOSAL_WAIT_FOREVER);
  if (cellular_app_echoclient_index < ECHOCLIENT_THREAD_NUMBER)
  {
    result = cellular_app_echoclient_index;
    cellular_app_echoclient_index++; /* Increment for next request */
  }
  (void)rtosalMutexRelease(cellular_app_echoclient_index_mutex_handle);

  return (result);
}

/**
  * @brief  EchoClt thread
  * @note   Infinite loop EchoClt body
  * @param  p_argument - unused
  * @retval -
  */
static void echoclient_thread(void *p_argument)
{
  UNUSED(p_argument);
  uint8_t app_index;   /* Index in cellular_app_echoclient_socket[]   */
  /* uint16_t msg_type; */  /* Msg type received from the queue */
  /* uint16_t msg_data; */  /* Msg id received from the queue   */
  uint32_t msg_queue;  /* Msg received from the queue      */
  uint32_t nfmc_tempo; /* NFMC tempo value according to nfm.index */

  app_index = echoclient_get_app_index();

  if (app_index < ECHOCLIENT_THREAD_NUMBER)
  {
    /* Specific treatment before the main loop: Update Date and Time using instance 0 */
#if (USE_RTC == 1)
    /* Check if date time is updated by EchoClt - Only the first instance is authorized to update it */
    if ((app_index == 0U) && (cellular_app_echoclient_set_datetime == true))
    {
      cellular_app_datetime_t datetime;
      /* Time and Date maybe already initialized by another process ? */
      if (cellular_app_datetime_get(&datetime) == true)
      {
        cellular_app_echoclient_set_datetime = false;
      }

      while (cellular_app_echoclient_set_datetime == true)
      {
        /* Wait data is ready AND Process is requested to be On */
        while ((cellular_app_is_data_ready() == false)
               || (cellular_app_echoclient_change[app_index].process_status == false))
        {
          /* Update process status */
          cellular_app_echoclient[app_index].process_status =
            cellular_app_echoclient_change[app_index].process_status;
          if (cellular_app_is_data_ready() == false)
          {
            PRINT_FORCE("\n\r<<< %s %d: wait data is ready to get date and time from network!>>>\n\r",
                        p_cellular_app_echoclient_trace, (app_index + 1U))
          }
          else
          {
            PRINT_FORCE("\n\r<<< %s %d: wait process activation to get date and time from network!>>>\n\r",
                        p_cellular_app_echoclient_trace, (app_index + 1U))
          }
          (void)rtosalMessageQueueGet(cellular_app_echoclient[app_index].queue_id, &msg_queue, RTOSAL_WAIT_FOREVER);
        }
        /* Update process status */
        cellular_app_echoclient[app_index].process_status =
          cellular_app_echoclient_change[app_index].process_status;
        if (cellular_app_echoclient[app_index].process_status == true)
        {
          if (cellular_app_is_data_ready() == true)
          {
            PRINT_FORCE("\n\r<<< %s %d STARTED - Obtain date and time from network >>>\n\r",
                        p_cellular_app_echoclient_trace, (app_index + 1U))
            /* Send trame to obtain date and time */
            echoclient_obtain_datetime(&cellular_app_echoclient_socket[app_index]);
            /* Request the date/time to the distant server only one time whatever the result */
            cellular_app_echoclient_set_datetime = false;
          }
        }
      }
    }
#endif /* USE_RTC == 1 */

    /* Thread main loop treatment */
    for (;;)
    {
      while (echoclient_is_blocked(app_index) == true)
      {
        if (cellular_app_echoclient[app_index].process_status
            != cellular_app_echoclient_change[app_index].process_status)
        {
          if (cellular_app_echoclient_change[app_index].process_status == false)
          {
            PRINT_FORCE("\n\r<<< %s %d STOPPED >>>\n\r", p_cellular_app_echoclient_trace, (app_index + 1U))
          }
          else
          {
            PRINT_FORCE("\n\r<<< %s %d Starting - wait data is ready >>>\n\r", p_cellular_app_echoclient_trace,
                        (app_index + 1U))
          }
          /* Update Process status */
          cellular_app_echoclient[app_index].process_status = cellular_app_echoclient_change[app_index].process_status;
        }
        /* Nothing to do except to wait data is ready or process is reactivated */
        (void)rtosalMessageQueueGet(cellular_app_echoclient[app_index].queue_id, &msg_queue, RTOSAL_WAIT_FOREVER);
      }
      /* Update process status */
      cellular_app_echoclient[app_index].process_status = cellular_app_echoclient_change[app_index].process_status;
      if (cellular_app_echoclient[app_index].process_status == true)
      {
        PRINT_FORCE("\n\r<<< %s %d STARTED >>>\n\r", p_cellular_app_echoclient_trace, (app_index + 1U))
      }

      /* Execute the performance test ? */
      if ((cellular_app_echoclient_perf_start == true) && (app_index == 0U))
      {
        PRINT_FORCE("\n\r<<< %s Performance Begin >>>\n\r", p_cellular_app_echoclient_trace)
        echoclient_performance(&cellular_app_echoclient_socket[app_index]);
        cellular_app_echoclient_perf_start = false;
        PRINT_FORCE("\n\r<<< %s Performance End >>>\n\r", p_cellular_app_echoclient_trace)
      }

      while (echoclient_is_blocked(app_index) == false)
      {
        /*  EchoClt active but is NFM sleep requested ?  */
        if (cellular_app_socket_is_nfm_sleep_requested(&cellular_app_echoclient_socket[app_index], &nfmc_tempo)
            == false)
        {
          /* Update buffer with new data and potentially new length */
          cellular_app_echoclient_socket[app_index].snd_buffer_len =
            echoclient_format_buffer(cellular_app_echoclient_socket_change[app_index].snd_buffer_len,
                                     cellular_app_echoclient_socket[app_index].p_snd_buffer);
          if (cellular_app_echoclient_socket[app_index].snd_buffer_len != 0U)
          {
            (void)echoclient_process((app_index + 1U), &cellular_app_echoclient_socket[app_index], NULL,
                                     &cellular_app_echoclient_socket_change[app_index]);
          }
          else
          {
            PRINT_INFO("%s %d: Buffer to send empty!", p_cellular_app_echoclient_trace, (app_index + 1U))
          }
        }
        else
        {
          PRINT_FORCE("%s %d: NFM too many errors! error/limit:%d/%d - timer activation:%ld ms",
                      p_cellular_app_echoclient_trace, (app_index + 1U),
                      cellular_app_echoclient_socket[app_index].nfm.error_current_nb,
                      cellular_app_echoclient_socket[app_index].nfm.error_limit_nb, nfmc_tempo)
          (void)rtosalDelay(nfmc_tempo);
          /* Reset NFM error */
          cellular_app_echoclient_socket[app_index].nfm.error_current_nb = 0U;
          /* Increase NFM index: if maximum index value is reached maintain it to its maximum possible value
             then xxx_is_nfm_sleep_requested() service is in charge to provide the correct nfm_tempo value */
          if (cellular_app_echoclient_socket[app_index].nfm.index < ((uint8_t)CA_NFMC_VALUES_MAX_NB - 1U))
          {
            cellular_app_echoclient_socket[app_index].nfm.index++;
          }
        }

        /* Update process period */
        cellular_app_echoclient[app_index].process_period = cellular_app_echoclient_change[app_index].process_period;
        (void)rtosalDelay(cellular_app_echoclient[app_index].process_period);
      }

      /* Data is no more ready or Process is off - force a close when data will be back */
      if (cellular_app_echoclient_socket[app_index].state != CELLULAR_APP_SOCKET_INVALID)
      {
        PRINT_INFO("%s %d: Data not ready or Process stopped! Closing the socket!", p_cellular_app_echoclient_trace,
                   (app_index + 1U))
        cellular_app_echoclient_socket[app_index].closing = true;
        /* If data is ready try to close the socket */
        do
        {
          if (cellular_app_is_data_ready() == true)
          {
            PRINT_INFO("%s %d: Data ready! Closing the socket!", p_cellular_app_echoclient_trace, (app_index + 1U))
            cellular_app_socket_close(CELLULAR_APP_TYPE_ECHOCLIENT, (app_index + 1U),
                                      &cellular_app_echoclient_socket[app_index]);
          }
          else
          {
            PRINT_INFO("%s %d: Data not ready! Waiting to close the socket!", p_cellular_app_echoclient_trace,
                       (app_index + 1U))
            (void)rtosalDelay(5000U);
          }
        } while (cellular_app_echoclient_socket[app_index].state != CELLULAR_APP_SOCKET_INVALID);
      }
    }
  }
  else
  {
    /* Abnormal value - something goes wrong */
    PRINT_FORCE("%s: Abnormal value at thread creation!", p_cellular_app_echoclient_trace)
    CELLULAR_APP_ERROR(CELLULAR_APP_ERROR_ECHOCLIENT, ERROR_FATAL)
  }
}

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Send a message to EchoClt
  * @param  index     - EchoClt index - if 0xFF then send to all EchoClt
  * @param  queue_msg - Message to send
  * @retval bool      - false/true - Message not send / Message send
  */
bool cellular_app_echoclient_send_msg(uint8_t index, uint32_t queue_msg)
{
  bool result = true;
  uint8_t i = 0U;
  uint8_t index_limit;
  rtosalStatus status;

  if (index == 0xFFU)
  {
    index_limit = ECHOCLIENT_THREAD_NUMBER;
  }
  else if (index < ECHOCLIENT_THREAD_NUMBER)
  {
    index_limit = index + 1U;
  }
  else
  {
    index_limit = 0U; /* Error */
    result = false;
  }

  while (i < index_limit)
  {
    /* A message has to be send */
    status = rtosalMessageQueuePut(cellular_app_echoclient[i].queue_id, queue_msg, 0U);
    if (status != osOK)
    {
      result = false;
      PRINT_FORCE("%s: ERROR CellularApi Msg Put App:%d Type:%d Id:%d - status:%d!", p_cellular_app_echoclient_trace,
                  (i + 1U), GET_CELLULAR_APP_MSG_TYPE(queue_msg), GET_CELLULAR_APP_MSG_ID(queue_msg), status)
    }
    i++;
  }

  return (result);
}

/**
  * @brief  Get status of a specific EchoClt application
  * @param  index - EchoClt index
  * @retval cellular_app_process_status_t - EchoClt application process status
  */
cellular_app_process_status_t cellular_app_echoclient_get_status(uint8_t index)
{
  cellular_app_process_status_t result = CELLULAR_APP_PROCESS_STATUS_MAX;

  if (index < ECHOCLIENT_THREAD_NUMBER)
  {
    result = cellular_app_get_status(cellular_app_echoclient[index].process_status,
                                     cellular_app_echoclient_change[index].process_status);
  }

  return (result);
}

/**
  * @brief  Set status of a specific EchoClt application
  * @param  index             - EchoClt index to change
  * @param  process_status    - process status new value to set inactive/active
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_set_status(uint8_t index, bool process_status)
{
  bool result = false;
  cellular_app_process_status_t process_status_tmp;
  rtosalStatus status;

  if (index < ECHOCLIENT_THREAD_NUMBER)
  {
    process_status_tmp = cellular_app_get_status(cellular_app_echoclient[index].process_status,
                                                 cellular_app_echoclient_change[index].process_status);
    /* Only one change at a time */
    if (((process_status == true) && (process_status_tmp == CELLULAR_APP_PROCESS_OFF))
        || ((process_status == false) && (process_status_tmp == CELLULAR_APP_PROCESS_ON)))
    {
      uint32_t queue_msg = 0U;

      SET_CELLULAR_APP_MSG_TYPE(queue_msg, CELLULAR_APP_PROCESS_MSG);
      SET_CELLULAR_APP_MSG_ID(queue_msg, CELLULAR_APP_PROCESS_CHANGE_ID);

      cellular_app_echoclient_change[index].process_status = process_status;
      status = rtosalMessageQueuePut(cellular_app_echoclient[index].queue_id, queue_msg, 0U);
      if (status != osOK)
      {
        /* Restore old value */
        cellular_app_echoclient_change[index].process_status = !process_status;
        PRINT_FORCE("%s ERROR SetStatus Msg Put App:%d Type:%d Id:%d - status:%d!", p_cellular_app_echoclient_trace,
                    (index + 1U), GET_CELLULAR_APP_MSG_TYPE(queue_msg), GET_CELLULAR_APP_MSG_ID(queue_msg), status)
      }
      else
      {
        result = true;
      }
    }
    else
    {
      PRINT_FORCE("%s %d: Only one process change at a time!", p_cellular_app_echoclient_trace, (index + 1U))
    }
  }

  return (result);
}

/**
  * @brief  Set period of a specific EchoClt application
  * @param  index             - EchoClt index to change
  * @param  process_process   - new period process value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_set_period(uint8_t index, uint32_t process_period)
{
  bool result;

  if ((index < ECHOCLIENT_THREAD_NUMBER) && (process_period > ECHOCLIENT_PROCESS_PERIOD_MIN))
  {
    cellular_app_echoclient_change[index].process_period = process_period;
    result = true;
  }
  else
  {
    result = false;
  }

  return (result);
}

/**
  * @brief  Set send buffer length of a specific EchoClt application
  * @param  index             - EchoClt index to change
  * @param  snd_buffer_len    - send buffer length new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_set_snd_buffer_len(uint8_t index, uint16_t snd_buffer_len)
{
  bool result = false;

  if (index < ECHOCLIENT_THREAD_NUMBER)
  {
    if ((snd_buffer_len >= ECHOCLIENT_SND_RCV_MIN_SIZE) && (snd_buffer_len <= ECHOCLIENT_SND_RCV_MAX_SIZE))
    {
      cellular_app_echoclient_socket_change[index].snd_buffer_len = snd_buffer_len;
      result = true;
    }
    else
    {
      /* Display a reminder about size min, size max */
      PRINT_FORCE("%s %d: value for 'size' must be provided and in [%d,%d] !", p_cellular_app_echoclient_trace,
                  (index + 1U), ECHOCLIENT_SND_RCV_MIN_SIZE, ECHOCLIENT_SND_RCV_MAX_SIZE)
    }
  }

  return (result);
}

/**
  * @brief  Set protocol of a specific EchoClt application
  * @param  index    - EchoClt index to change
  * @param  protocol - protocol new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_set_protocol(uint8_t index, cellular_app_socket_protocol_t protocol)
{
  bool result = false;

  if (index < ECHOCLIENT_THREAD_NUMBER)
  {
    if (cellular_app_echoclient_socket[index].protocol != cellular_app_echoclient_socket_change[index].protocol)
    {
      /* Only one modification at a time */
      PRINT_FORCE("%s %d: Only one protocol change at a time!", p_cellular_app_echoclient_trace, (index + 1U))
    }
    else
    {
      if (protocol == cellular_app_echoclient_socket[index].protocol)
      {
        PRINT_FORCE("%s %d: Protocol already %s!", p_cellular_app_echoclient_trace, (index + 1U),
                    cellular_app_protocol_string[protocol])
      }
      else
      {
        result = true;
        PRINT_FORCE("%s %d: Protocol change to %s in progress...", p_cellular_app_echoclient_trace, (index + 1U),
                    cellular_app_protocol_string[protocol])
        cellular_app_echoclient_socket_change[index].protocol = protocol;
        if (cellular_app_get_status(cellular_app_echoclient[index].process_status,
                                    cellular_app_echoclient_change[index].process_status)
            == CELLULAR_APP_PROCESS_OFF)
        {
          /* Protocol change immediately because no impact on distant server parameters */
          cellular_app_echoclient_socket[index].protocol = protocol;
          PRINT_FORCE("%s %d: Protocol change to %s done", p_cellular_app_echoclient_trace, (index + 1U),
                      cellular_app_protocol_string[protocol])
        }
      }
    }
  }

  return (result);
}

/**
  * @brief  Change distant of a specific EchoClt application
  * @param  index             - EchoClt index to change
  * @param  distant_type      - distant type value
  * @param  p_distantip       - distant ip value  (supported for PingClt only)
  * @param  distantip_len     - distant ip length (supported for PingClt only)
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_distant_change(uint8_t index, cellular_app_distant_type_t distant_type,
                                            uint8_t *p_distantip, uint32_t distantip_len)
{
  bool result = false;
  cellular_app_process_status_t process_status;

  if (index < ECHOCLIENT_THREAD_NUMBER)
  {
    process_status = cellular_app_get_status(cellular_app_echoclient[index].process_status,
                                             cellular_app_echoclient_change[index].process_status);
    result = cellular_app_distant_change(CELLULAR_APP_TYPE_ECHOCLIENT, index, process_status,
                                         distant_type, p_distantip, distantip_len,
                                         &cellular_app_echoclient_socket[index].distant,
                                         &cellular_app_echoclient_socket_change[index]);
  }

  return (result);
}

/**
  * @brief  Get EchoClt socket statistics
  * @param  index  - application index to get statistics
  * @param  p_stat - statistics result pointer
  * @retval bool   - false/true - application not found / application found, *p_stat provided
  */
bool cellular_app_echoclient_get_socket_stat(uint8_t index, cellular_app_socket_stat_desc_t *p_stat)
{
  bool result = false;

  if (index < ECHOCLIENT_THREAD_NUMBER)
  {
    (void)memcpy((void *)p_stat, (const void *) & (cellular_app_echoclient_socket[index].stat),
                 sizeof(cellular_app_socket_stat_desc_t));
    result = true;
  }

  return (result);
}

/**
  * @brief  Reset EchoClt statistics
  * @param  index - application index to change
  * @retval -
  */
void cellular_app_echoclient_reset_socket_stat(uint8_t index)
{
  if (index < ECHOCLIENT_THREAD_NUMBER)
  {
    (void)memset((void *) & (cellular_app_echoclient_socket[index].stat), 0, sizeof(cellular_app_socket_stat_desc_t));
  }
}

/**
  * @brief  Provide  EchoClt performance feature status
  * @param  -
  * @retval bool - false/true - not started/started
  */
bool cellular_app_echoclient_get_performance_status(void)
{
  return (cellular_app_echoclient_perf_start);
}

/**
  * @brief  EchoClt performance feature
  * @param  status  - false/true - performance to stop/performance to start
  * @param  iter_nb - iteration number (0: default value to use)
  * @retval bool    - false/true - not done/done
  */
bool cellular_app_echoclient_performance(bool status, uint8_t iter_nb)
{
  bool result = true;

  /* Check echoclient instance 0 is available */
  if (status == true)
  {
#if (USE_RTC == 1)
    if (cellular_app_echoclient_set_datetime == true)
    {
      result = false;
      PRINT_FORCE("%s %d: Date and Time not yet obtain! Wait date and time is obtain before to retry!",
                  p_cellular_app_echoclient_trace, (1U))
    }
    else
    {
      cellular_app_echoclient_perf_start = true;
      cellular_app_echoclient_perf_iter_nb = iter_nb;
    }
#else /* USE_RTC == 0 */
    cellular_app_echoclient_perf_start = true;
    cellular_app_echoclient_perf_iter_nb = iter_nb;
#endif /* USE_RTC == 1 */
  }
  else
  {
    cellular_app_echoclient_perf_start = false;
    cellular_app_echoclient_perf_iter_nb = iter_nb;
  }

  return (result);
}

/**
  * @brief  Display EchoClt status
  * @param  -
  * @retval -
  */
void cellular_app_echoclient_display_status(void)
{
  cellular_app_process_status_t process_status;

  for (uint8_t i = 0U; i < ECHOCLIENT_THREAD_NUMBER; i++)
  {
    PRINT_FORCE("%s %d Status:", p_cellular_app_echoclient_trace, (i + 1U))
    /* Is Distant Server IP known ? */
    if (CELLULAR_APP_GET_DISTANTIP(cellular_app_echoclient_socket[i].distant.ip) == 0U)
    {
      /* Distant Server IP unknown */
      PRINT_FORCE("Distant server:%s Name:%s IP:Unknown Port:%d",
                  cellular_app_distant_string[cellular_app_echoclient_socket[i].distant.type],
                  cellular_app_echoclient_socket[i].distant.p_name,
                  cellular_app_echoclient_socket[i].distant.port)
    }
    else
    {
      /* Distant Server IP known */
      PRINT_FORCE("Distant server:%s Name:%s IP:%d.%d.%d.%d Port:%d",
                  cellular_app_distant_string[cellular_app_echoclient_socket[i].distant.type],
                  cellular_app_echoclient_socket[i].distant.p_name,
                  COM_IP4_ADDR1(&cellular_app_echoclient_socket[i].distant.ip),
                  COM_IP4_ADDR2(&cellular_app_echoclient_socket[i].distant.ip),
                  COM_IP4_ADDR3(&cellular_app_echoclient_socket[i].distant.ip),
                  COM_IP4_ADDR4(&cellular_app_echoclient_socket[i].distant.ip),
                  cellular_app_echoclient_socket[i].distant.port)
    }
    /* Actual Protocol - Period and Size parameters displayed */
    PRINT_FORCE("Parameters: Protocol:%s Period:%ldms Size:%dbytes",
                cellular_app_protocol_string[cellular_app_echoclient_socket[i].protocol],
                cellular_app_echoclient[i].process_period, cellular_app_echoclient_socket[i].snd_buffer_len)
    /* Requested Distant displayed */
    if (cellular_app_echoclient_socket[i].distant.type != cellular_app_echoclient_socket_change[i].distant_type)
    {
      PRINT_FORCE("Distant change in progress New value:%s",
                  cellular_app_distant_string[cellular_app_echoclient_socket_change[i].distant_type])
    }
    /* Requested Protocol displayed */
    if (cellular_app_echoclient_socket[i].protocol != cellular_app_echoclient_socket_change[i].protocol)
    {
      PRINT_FORCE("Protocol change in progress New value:%s",
                  cellular_app_protocol_string[cellular_app_echoclient_socket_change[i].protocol])
    }
    /* Requested Process Period displayed */
    if (cellular_app_echoclient[i].process_period != cellular_app_echoclient_change[i].process_period)
    {
      PRINT_FORCE("Process period change in progress New value:%ldms",
                  cellular_app_echoclient_change[i].process_period)
    }
    /* Requested Size of send buffer displayed */
    if (cellular_app_echoclient_socket[i].snd_buffer_len != cellular_app_echoclient_socket_change[i].snd_buffer_len)
    {
      PRINT_FORCE("Size of buffer to send change in progress New value:%dbytes",
                  cellular_app_echoclient_socket_change[i].snd_buffer_len)
    }
    /* Process status */
    process_status = cellular_app_get_status(cellular_app_echoclient[i].process_status,
                                             cellular_app_echoclient_change[i].process_status);
    PRINT_FORCE("Status: %s\r\n", cellular_app_process_status_string[process_status])
  }
}

/**
  * @brief  Initialize all needed structures to support EchoClt feature
  * @param  -
  * @retval -
  */
void cellular_app_echoclient_init(void)
{
  /* EchoClt buffer to send the request to the distant */
  static uint8_t echoclient_snd_buffer[ECHOCLIENT_THREAD_NUMBER][ECHOCLIENT_SND_RCV_MAX_SIZE + 1U];
  /* EchoClt buffer to store the response of the distant */
  static uint8_t echoclient_rcv_buffer[ECHOCLIENT_THREAD_NUMBER][ECHOCLIENT_SND_RCV_MAX_SIZE + 1U];

  /* Set DateTime Initialization */
#if (USE_RTC == 1)
#if (ECHOCLIENT_DATETIME_ACTIVATED == 1)
  cellular_app_echoclient_set_datetime = true;
#else /* ECHOCLIENT_DATETIME_ACTIVATED == 0 */
  cellular_app_echoclient_set_datetime = false;
#endif /* ECHOCLIENT_DATETIME_ACTIVATED == 1 */
#endif /* USE_RTC == 1 */

  /* EchoClt Index Initialization */
  cellular_app_echoclient_index = 0U;

  /* Initialize trace shortcut */
  p_cellular_app_echoclient_trace = cellular_app_type_string[CELLULAR_APP_TYPE_ECHOCLIENT];

  /* Mutex to protect socket descriptor list access Creation */
  cellular_app_echoclient_index_mutex_handle = rtosalMutexNew(NULL);
  if (cellular_app_echoclient_index_mutex_handle == NULL)
  {
    CELLULAR_APP_ERROR(CELLULAR_APP_ERROR_ECHOCLIENT, ERROR_FATAL)
  }

  for (uint8_t i = 0U; i < ECHOCLIENT_THREAD_NUMBER; i++)
  {
    /* Application Id Initialization */
    cellular_app_echoclient[i].app_id = i;
    /* Process Status Initialization */
    cellular_app_echoclient[i].process_status = false;
    /* Process Period Initialization */
    cellular_app_echoclient[i].process_period = ECHOCLIENT_DEFAULT_PROCESS_PERIOD;
    /* Thread Id Initialization */
    cellular_app_echoclient[i].thread_id = NULL;
    /* Queue Id Initialization/Creation */
    cellular_app_echoclient[i].queue_id = rtosalMessageQueueNew(NULL, CELLULAR_APP_QUEUE_SIZE);

    /* Change Structure Initialization */
    cellular_app_echoclient_change[i].process_status = cellular_app_echoclient[i].process_status;
    cellular_app_echoclient_change[i].process_period = cellular_app_echoclient[i].process_period;

    /* Socket Generic Initialization : state, closing, protocol, id */
    cellular_app_socket_init(&cellular_app_echoclient_socket[i], &cellular_app_echoclient_socket_change[i]);

    /* Socket Initialization Specific Parameters */
    /* Send Buffer Length Initialization */
    cellular_app_echoclient_socket[i].snd_buffer_len = ECHOCLIENT_SND_RCV_MIN_SIZE;

    /*  Timeout Initialization */
    cellular_app_echoclient_socket[i].snd_rcv_timeout = ECHOCLIENT_SND_RCV_TIMEOUT;

    /* Send/Receive Buffers Initialization */
    cellular_app_echoclient_socket[i].p_snd_buffer = &(echoclient_snd_buffer[i][0]);
    cellular_app_echoclient_socket[i].p_rcv_buffer = &(echoclient_rcv_buffer[i][0]);

    /* Distant Initialization - Default value CELLULAR_APP_DISTANT_MBED_TYPE */
    cellular_app_distant_update(CELLULAR_APP_DISTANT_MBED_TYPE, &(cellular_app_echoclient_socket[i].distant));

    /* NFM Initialization */
    cellular_app_echoclient_socket[i].nfm.error_current_nb = 0U;
    cellular_app_echoclient_socket[i].nfm.error_limit_nb   = ECHOCLIENT_NFM_ERROR_LIMIT_NB_MAX;
    cellular_app_echoclient_socket[i].nfm.index            = 0U;

    /* Statistic Initialization */
    (void)memset((void *) & (cellular_app_echoclient_socket[i].stat), 0, sizeof(cellular_app_socket_stat_desc_t));

    /* Change Structure Initialization */
    cellular_app_echoclient_socket_change[i].snd_buffer_len = cellular_app_echoclient_socket[i].snd_buffer_len;
    cellular_app_echoclient_socket_change[i].distant_type = cellular_app_echoclient_socket[i].distant.type;

    /* Check Initialization is ok */
    if (cellular_app_echoclient[i].queue_id == NULL)
    {
      CELLULAR_APP_ERROR((CELLULAR_APP_ERROR_ECHOCLIENT + (int32_t)(i) + 1), ERROR_FATAL)
    }
  }

#if (ECHOCLIENT1_ACTIVATED == 1U)
  /* Specific Initialization */
  cellular_app_echoclient[0].process_status = true;
  cellular_app_echoclient_change[0].process_status = cellular_app_echoclient[0].process_status;
#endif /* ECHOCLIENT1_ACTIVATED == 1U */
}

/**
  * @brief  Start all EchoClt threads
  * @param  -
  * @retval -
  */
void cellular_app_echoclient_start(void)
{
  uint8_t thread_name[CELLULAR_APP_THREAD_NAME_MAX];
  uint32_t len;

  /* Thread Name Generation */
  /* Let a space to add the EchoClt number and '+1' to copy '\0' */
  len = crs_strlen((const uint8_t *)"EchoClt ");
  (void)memcpy(thread_name, "EchoClt ", CELLULAR_APP_MIN((len + 1U), CELLULAR_APP_THREAD_NAME_MAX));
  /* Thread Creation */
  for (uint8_t i = 0U; i < ECHOCLIENT_THREAD_NUMBER; i++)
  {
    /* Thread Name Insatnce Update */
    thread_name[(len - 1U)] = (uint8_t)0x30 + (i + 1U); /* start at EchoClt1 */

    /* Thread Creation */
    cellular_app_echoclient[i].thread_id = rtosalThreadNew((const rtosal_char_t *)thread_name,
                                                           (os_pthread)echoclient_thread,
                                                           ECHOCLIENT_THREAD_PRIO, ECHOCLIENT_THREAD_STACK_SIZE, NULL);
    /* Check Creation is ok */
    if (cellular_app_echoclient[i].thread_id == NULL)
    {
      CELLULAR_APP_ERROR((CELLULAR_APP_ERROR_ECHOCLIENT + (int32_t)ECHOCLIENT_THREAD_NUMBER + (int32_t)(i) + 1),
                         ERROR_FATAL)
    }
  }
}

#endif /* USE_CELLULAR_APP == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
