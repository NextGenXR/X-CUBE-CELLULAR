/**
  ******************************************************************************
  * @file    cellular_app_socket.c
  * @author  MCD Application Team
  * @brief   Cellular Application socket and distant services
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

#include "cellular_app_socket.h"

#include "cellular_app.h"
#include "cellular_app_trace.h"
#include "cellular_app_echoclient.h"

#include "cellular_control_api.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
/*
$ host echo.mbedcloudtesting.com
echo.mbedcloudtesting.com has address 52.215.34.155
echo.mbedcloudtesting.com has IPv6 address 2a05:d018:21f:3800:8584:60f8:bc9f:e614
echo.mbedcloudtesting.com has echo server on port 7
echo.mbedcloudtesting.com has date time server on port 13
*/
#define CELLULAR_APP_DISTANT_MBED_NAME                 ((uint8_t *)"echo.mbedcloudtesting.com")
#define CELLULAR_APP_DISTANT_MBED_PORT                 ((uint16_t)7U)
/* A first welcome msg is send / or not by the distant server */
/* In case of TCP or UDP connection, MBED don't send a first message */
#define CELLULAR_APP_DISTANT_MBED_TCP_WELCOME_MSG      (CELLULAR_APP_DISTANT_NO_WELCOME_MSG) /* No welcome msg in TCP */
#define CELLULAR_APP_DISTANT_MBED_UDP_WELCOME_MSG      (CELLULAR_APP_DISTANT_NO_WELCOME_MSG) /* No welcome msg in UDP */

/*
$ host echo.u-blox.com
echo.u-blox.com has address 195.34.89.241
echo.u-blox.com has echo server on port 7
*/
#define CELLULAR_APP_DISTANT_UBLOX_NAME                ((uint8_t *)"echo.u-blox.com")
#define CELLULAR_APP_DISTANT_UBLOX_PORT                ((uint16_t)7U)
/* A first welcome msg is send / or not by the distant server */
/* In case of TCP connection, u-blox send a first message before echoing the send data */
#define CELLULAR_APP_DISTANT_UBLOX_TCP_WELCOME_MSG     (uint8_t *)"U-Blox AG TCP/UDP test service\n"
/* In case of UDP connection, u-blox don't send a first message */
#define CELLULAR_APP_DISTANT_UBLOX_UDP_WELCOME_MSG     (CELLULAR_APP_DISTANT_NO_WELCOME_MSG) /* No welcome msg in UDP */

/*
$ host local server
local server has address xxx.xxx.xxx.xxx
local server has echo server on port 7
*/
/* DISTANT_LOCAL_NAME set to CELLULAR_APP_DISTANT_UNKNOWN_NAME to by-pass,
 * DNS resolution DNS server ignore local server IP */
#define CELLULAR_APP_DISTANT_LOCAL_NAME                (CELLULAR_APP_DISTANT_UNKNOWN_NAME)
#define CELLULAR_APP_DISTANT_LOCAL_PORT                ((uint16_t)7U)
/* In case of TCP / UDP connection, local server don't send a first message */
#define CELLULAR_APP_DISTANT_LOCAL_TCP_WELCOME_MSG     (CELLULAR_APP_DISTANT_NO_WELCOME_MSG) /* No welcome msg in TCP */
#define CELLULAR_APP_DISTANT_LOCAL_UDP_WELCOME_MSG     (CELLULAR_APP_DISTANT_NO_WELCOME_MSG) /* No welcome msg in UDP */

/* ToDo: to input an address - not supported yet */
/* #define CELLULAR_APP_NAME_SIZE_MAX  (uint8_t)(64U) */ /* MAX_SIZE_IPADDR of cellular_service.h */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Socket distant IP */
uint8_t cellular_app_distant_ip[CELLULAR_APP_DISTANT_TYPE_MAX - 1][4] =
{
  {  52U, 215U,  34U, 155U},  /* CELLULAR_APP_DISTANT_MBED_TYPE  */
  { 195U,  34U,  89U, 241U},  /* CELLULAR_APP_DISTANT_UBLOX_TYPE */
  /* ToDo: add a cmd to set distant local IP */
  /*  {   0U,   0U,   0U,   0U}, */ /* CELLULAR_APP_DISTANT_LOCAL_TYPE */
  { 192U, 168U,   2U,   1U},  /* CELLULAR_APP_DISTANT_LOCAL_TYPE */
  {   8U,   8U,   8U,   8U},  /* CELLULAR_APP_DISTANT_IP1_TYPE   */
  {  52U, 215U,  34U, 155U},  /* CELLULAR_APP_DISTANT_IP2_TYPE   */
  {   0U,   0U,   0U,   0U}   /* CELLULAR_APP_DISTANT_IPx_TYPE   */
};

/* String used to display socket distant */
const uint8_t *cellular_app_distant_string[CELLULAR_APP_DISTANT_TYPE_MAX] =
{
  (uint8_t *)"MBED",
  (uint8_t *)"UBLOX",
  (uint8_t *)"LOCAL",
  (uint8_t *)"ACTUAL",
  (uint8_t *)"IP1",
  (uint8_t *)"IP2",
  (uint8_t *)"IPx"
};

/* String used to display socket protocol */
const uint8_t *cellular_app_protocol_string[CELLULAR_APP_SOCKET_PROTO_MAX] =
{
  (uint8_t *)"TCP",
  (uint8_t *)"UDP mode connected",
  (uint8_t *)"UDP mode not-connected"
};

/* Private functions prototypes ----------------------------------------------*/
/* Public  functions  prototypes ---------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Check Distant server data - Update Distant server IP
  * @note   Decide if Network DNS resolver has to be called
  * @param  type              - application type
  * @param  index             - application index
  * @param  p_distant         - pointer on distant server structure
  * @retval bool - false/true - distant ip is NOK / OK
  */
bool cellular_app_distant_check(cellular_app_type_t type, uint8_t index, cellular_app_distant_desc_t *p_distant)
{
#if (USE_TRACE_APPLICATION == 0U)
  UNUSED(type);  /* parameter only used in PRINT_INFO */
  UNUSED(index); /* parameter only used in PRINT_INFO */
#endif /* USE_TRACE_APPLICATION == 0U */
  bool result = true;

  /* If distant name is provided and distant ip is unknown, call DNS network resolution service */
  if ((crs_strlen(p_distant->p_name) > 0U) && (CELLULAR_APP_GET_DISTANTIP(p_distant->ip) == 0U))
  {
    com_sockaddr_t distantaddr;

    /* DNS network resolution request */
    PRINT_INFO("%s %d: Distant Name provided %s. DNS resolution started", cellular_app_type_string[type], index,
               p_distant->p_name)
    if (com_gethostbyname(p_distant->p_name, &distantaddr) == COM_SOCKETS_ERR_OK)
    {
      /* DNS resolution ok - save IP in EchoClt_distantip */
      /* Even next test doesn't suppress MISRA Warnings */
      /*
            if ((distantaddr.sa_len == (uint8_t)sizeof(com_sockaddr_in_t))
                && ((distantaddr.sa_family == (uint8_t)COM_AF_UNSPEC)
                    || (distantaddr.sa_family == (uint8_t)COM_AF_INET)))
      */
      {
        CELLULAR_APP_SET_DISTANTIP(&(p_distant->ip), ((com_sockaddr_in_t *)&distantaddr)->sin_addr.s_addr);
        PRINT_INFO("%s %d: DNS resolution OK - Echo Remote IP: %d.%d.%d.%d", cellular_app_type_string[type], index,
                   COM_IP4_ADDR1(&(p_distant->ip)), COM_IP4_ADDR2(&(p_distant->ip)),
                   COM_IP4_ADDR3(&(p_distant->ip)), COM_IP4_ADDR4(&(p_distant->ip)))
        /* No reset of error_current_nb wait to see if distant can be reached */
      }
    }
    else
    {
      result = false; /* DNS resolution NOK - increase fault counters */
      PRINT_INFO("%s %d: DNS resolution NOK!", cellular_app_type_string[type], index)
    }
  }

  return (result);
}

/**
  * @brief  Update Distant data
  * @param  distant_type - New Distant type
  * @param  p_distant    - new distant value to update
  * @retval -
  */
void cellular_app_distant_update(cellular_app_distant_type_t distant_type, cellular_app_distant_desc_t *p_distant)
{
  switch (distant_type)
  {
    case CELLULAR_APP_DISTANT_MBED_TYPE :
      /* Distant Server type */
      p_distant->type = distant_type;
      /* Distant Server Name */
      p_distant->p_name = CELLULAR_APP_DISTANT_MBED_NAME;
      /* Distant Server IP */
      /* To by-pass DNS resolution: 1) Uncomment next line */
      /* COM_IP4_ADDR(&(p_distant->ip),
                   cellular_app_distant_ip[distant_type][0], cellular_app_distant_ip[distant_type][1],
                   cellular_app_distant_ip[distant_type][2], cellular_app_distant_ip[distant_type][3]); */
      /* To by-pass DNS resolution: 2) Comment next line */
      /* Distant Server IP set to 0 to force DNS resolution */
      CELLULAR_APP_SET_DISTANTIP_NULL(&(p_distant->ip));
      /* Distant Server Port */
      p_distant->port = CELLULAR_APP_DISTANT_MBED_PORT;
      /* Distant Server Welcome Msg TCP */
      p_distant->p_tcp_welcome_msg = CELLULAR_APP_DISTANT_MBED_TCP_WELCOME_MSG;
      /* Distant Server Welcome Msg UDP */
      p_distant->p_udp_welcome_msg = CELLULAR_APP_DISTANT_MBED_UDP_WELCOME_MSG;
      break;

    case CELLULAR_APP_DISTANT_UBLOX_TYPE :
      /* Distant Type */
      p_distant->type = distant_type;
      /* Distant Name */
      p_distant->p_name = CELLULAR_APP_DISTANT_UBLOX_NAME;
      /* Distant IP */
      /* To by-pass DNS resolution: 1) Uncomment next line */
      /* COM_IP4_ADDR(&(p_distant->ip),
                   cellular_app_distant_ip[distant_type][0], cellular_app_distant_ip[distant_type][1],
                   cellular_app_distant_ip[distant_type][2], cellular_app_distant_ip[distant_type][3]); */
      /* To by-pass DNS resolution: 2) Comment next line */
      /* Distant Server IP set to 0 to force DNS resolution */
      CELLULAR_APP_SET_DISTANTIP_NULL(&(p_distant->ip));
      /* Distant Port */
      p_distant->port = CELLULAR_APP_DISTANT_UBLOX_PORT;
#if (USE_RTC == 1)
      /* Distant Date and Time Port */
#endif /* USE_RTC == 1 */
      /* Distant Welcome Msg TCP */
      p_distant->p_tcp_welcome_msg = CELLULAR_APP_DISTANT_UBLOX_TCP_WELCOME_MSG;
      /* Distant Welcome Msg UDP */
      p_distant->p_udp_welcome_msg = CELLULAR_APP_DISTANT_UBLOX_UDP_WELCOME_MSG;
      break;

    case CELLULAR_APP_DISTANT_LOCAL_TYPE :
      /* Distant Type */
      p_distant->type = distant_type;
      /* Distant Name */
      p_distant->p_name = CELLULAR_APP_DISTANT_LOCAL_NAME;
      /* Distant IP */
      /* Local is unknown from DNS server - force IP address to its value to by pass DNS resolution */
      COM_IP4_ADDR(&(p_distant->ip),
                   (uint32_t)(cellular_app_distant_ip[distant_type][0]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][1]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][2]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][3]));
      /* Distant Server Port */
      p_distant->port = CELLULAR_APP_DISTANT_LOCAL_PORT;
      /* Distant Welcome Msg TCP */
      p_distant->p_tcp_welcome_msg = CELLULAR_APP_DISTANT_LOCAL_TCP_WELCOME_MSG;
      /* Distant Welcome Msg UDP */
      p_distant->p_udp_welcome_msg = CELLULAR_APP_DISTANT_LOCAL_UDP_WELCOME_MSG;
      break;

    case CELLULAR_APP_DISTANT_IP1_TYPE  :
      /* Distant Type */
      p_distant->type = distant_type;
      /* Distant Name */
      p_distant->p_name = CELLULAR_APP_DISTANT_UNKNOWN_NAME;
      /* Distant IP */
      COM_IP4_ADDR(&(p_distant->ip),
                   (uint32_t)(cellular_app_distant_ip[distant_type][0]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][1]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][2]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][3]));
      /* Distant Server Port */
      p_distant->port = CELLULAR_APP_DISTANT_UNKNOWN_PORT;
      /* Distant Welcome Msg TCP */
      p_distant->p_tcp_welcome_msg = CELLULAR_APP_DISTANT_NO_WELCOME_MSG;
      /* Distant Welcome Msg UDP */
      p_distant->p_udp_welcome_msg = CELLULAR_APP_DISTANT_NO_WELCOME_MSG;
      break;

    case CELLULAR_APP_DISTANT_IP2_TYPE  :
      /* Distant Type */
      p_distant->type = distant_type;
      /* Distant Name */
      p_distant->p_name = CELLULAR_APP_DISTANT_UNKNOWN_NAME;
      /* Distant IP */
      COM_IP4_ADDR(&(p_distant->ip),
                   (uint32_t)(cellular_app_distant_ip[distant_type][0]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][1]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][2]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][3]));
      /* Distant Server Port */
      p_distant->port = CELLULAR_APP_DISTANT_UNKNOWN_PORT;
      /* Distant Welcome Msg TCP */
      p_distant->p_tcp_welcome_msg = CELLULAR_APP_DISTANT_NO_WELCOME_MSG;
      /* Distant Welcome Msg UDP */
      p_distant->p_udp_welcome_msg = CELLULAR_APP_DISTANT_NO_WELCOME_MSG;
      break;

    case CELLULAR_APP_DISTANT_IPx_TYPE  :
      /* Distant Type */
      p_distant->type = distant_type;
      /* Distant Name */
      p_distant->p_name = CELLULAR_APP_DISTANT_UNKNOWN_NAME;
      /* Distant IP */
      COM_IP4_ADDR(&(p_distant->ip),
                   (uint32_t)(cellular_app_distant_ip[distant_type][0]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][1]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][2]),
                   (uint32_t)(cellular_app_distant_ip[distant_type][3]));
      /* Distant Server Port */
      p_distant->port = CELLULAR_APP_DISTANT_UNKNOWN_PORT;
      /* Distant Welcome Msg TCP */
      p_distant->p_tcp_welcome_msg = CELLULAR_APP_DISTANT_NO_WELCOME_MSG;
      /* Distant Welcome Msg UDP */
      p_distant->p_udp_welcome_msg = CELLULAR_APP_DISTANT_NO_WELCOME_MSG;
      break;

    case CELLULAR_APP_DISTANT_ACTUAL_TYPE :
      __NOP(); /* Let distant as it is */
      break;

    case CELLULAR_APP_DISTANT_TYPE_MAX    :
    default :
      /* Should not happen */
      __NOP();
      break;
  }
}

/**
  * @brief  Check if NFM sleep has to be done
  * @param  p_socket          - pointer on the socket to use
  * @param  p_nfmc_tempo      - pointer on the nfmc value to use
  * @retval bool - false/true - NFM sleep hasn't to be done/has to be done
  */
bool cellular_app_socket_is_nfm_sleep_requested(cellular_app_socket_desc_t *p_socket, uint32_t *p_nfmc_tempo)
{
  bool result = false;
  *p_nfmc_tempo = 0U;

  /* Too many errors ? */
  if (p_socket->nfm.error_current_nb >= p_socket->nfm.error_limit_nb)
  {
    cellular_nfmc_info_t nfmc_info;

    /* Read NFMC infos to know if NFMC is enable and tempo values */
    cellular_get_nfmc_info(&nfmc_info);
    /* Is NFMC enable and tempo values defined ? */
    if ((nfmc_info.enable == true) && (nfmc_info.tempo_nb != 0U))
    {
      result = true;
      /* Cellular applications increase nfm.index until its maximum possible value - means CA_NFMC_VALUES_MAX_NB
       * but maybe tempo_values[p_socket->nfm.index] is not defined
       * nfm.index is [0, CA_NFMC_VALUES_MAX_NB - 1] but tempo_nb is a number so [1, CA_NFMC_VALUES_MAX_NB] */
      if (p_socket->nfm.index < nfmc_info.tempo_nb)
      {
        *p_nfmc_tempo = nfmc_info.tempo_values[p_socket->nfm.index];
      }
      else
      {
        if (nfmc_info.tempo_nb < CA_NFMC_VALUES_MAX_NB)
        {
          *p_nfmc_tempo = nfmc_info.tempo_values[(nfmc_info.tempo_nb - 1U)];
        }
        else
        {
          *p_nfmc_tempo = nfmc_info.tempo_values[(CA_NFMC_VALUES_MAX_NB - 1U)];
        }
      }
    }
    else
    {
      /* Reset NFM error current number */
      p_socket->nfm.error_current_nb = 0U;
    }
  }

  return (result);
}

/**
  * @brief  Obtain socket
  * @note   If needed close the socket before to create a new one
  * @param  type              - application type
  * @param  index             - application index
  * @param  p_socket          - pointer on the socket to use
  * @param  p_welcome_msg     - pointer on the welcome msg
  * @param  p_socket_change   - pointer on socket change requested
  * @retval bool - true/false - socket creation OK/NOK
  */
bool cellular_app_socket_obtain(cellular_app_type_t type, uint8_t index,
                                cellular_app_socket_desc_t *p_socket, uint8_t **p_welcome_msg,
                                cellular_app_socket_change_t *p_socket_change)
{
  bool result;
  uint32_t timeout;

  /* close the socket if:
   - internal close is requested
   - or previous close request not ok
   - or if socket protocol or distant type changed and socket is still open
   */
  if ((p_socket->closing == true)
      || (p_socket->state == CELLULAR_APP_SOCKET_CLOSING)
      || ((p_socket_change != NULL)
          && ((p_socket->protocol != p_socket_change->protocol)
              || (p_socket->distant.type != p_socket_change->distant_type))
          && (p_socket->state != CELLULAR_APP_SOCKET_INVALID)))
  {
    PRINT_INFO("%s %d: Socket in closing mode - Request the close", cellular_app_type_string[type], index)
    cellular_app_socket_close(type, index, p_socket);
    result = false; /* if close nok - socket can no more be used
                       if close ok  - socket has to be created   */
  }
  else /* socket is already closed or can still be used */
  {
    result = true;
  }

  /* If socket is closed, create a new one */
  if (p_socket->state == CELLULAR_APP_SOCKET_INVALID)
  {
    /* socket need to be created */
    result = false;
    if (p_socket_change != NULL)
    {
      /* Update protocol value */
      p_socket->protocol = p_socket_change->protocol;
      /* Distant need to be updated ? */
      if (p_socket->distant.type != p_socket_change->distant_type)
      {
        /* Update Distant parameters */
        cellular_app_distant_update(p_socket_change->distant_type, &(p_socket->distant));
      }
    }

    /* If distantip to contact is unknown, call DNS resolver service */
    if (CELLULAR_APP_GET_DISTANTIP(p_socket->distant.ip) == (uint32_t)0U)
    {
      if (cellular_app_distant_check(type, index, &(p_socket->distant)) == false)
      {
        p_socket->stat.connect.ko++;
        p_socket->nfm.error_current_nb++;
      }
    }

    /* If distantip to contact is known, execute rest of the process */
    if (CELLULAR_APP_GET_DISTANTIP(p_socket->distant.ip) != (uint32_t)0U)
    {
      /* Create a socket */
      PRINT_DBG("%s %i: Socket creation in progress", cellular_app_type_string[type], index)

      if (p_socket->protocol == CELLULAR_APP_SOCKET_TCP_PROTO)
      {
        p_socket->id = com_socket(COM_AF_INET, COM_SOCK_STREAM, COM_IPPROTO_TCP);
        *p_welcome_msg = p_socket->distant.p_tcp_welcome_msg;
      }
      else if ((p_socket->protocol == CELLULAR_APP_SOCKET_UDP_PROTO)
               || (p_socket->protocol == CELLULAR_APP_SOCKET_UDP_SERVICE_PROTO))
      {
        p_socket->id = com_socket(COM_AF_INET, COM_SOCK_DGRAM, COM_IPPROTO_UDP);
        *p_welcome_msg = p_socket->distant.p_udp_welcome_msg;
      }
      else
      {
        p_socket->id = COM_SOCKET_INVALID_ID;
      }

      if (p_socket->id > COM_SOCKET_INVALID_ID)
      {
        /* Socket created, continue the process */
        PRINT_INFO("%s %d: Socket create OK", cellular_app_type_string[type], index)
        /* Call rcvtimeout and sdntimeout setsockopt */
        timeout = p_socket->snd_rcv_timeout;

        PRINT_DBG("%s %d: Socket setsockopt in progress", cellular_app_type_string[type], index)
        if (com_setsockopt(p_socket->id, COM_SOL_SOCKET, COM_SO_RCVTIMEO, &timeout, (int32_t)sizeof(timeout))
            == COM_SOCKETS_ERR_OK)
        {
          if (com_setsockopt(p_socket->id, COM_SOL_SOCKET, COM_SO_SNDTIMEO, &timeout, (int32_t)sizeof(timeout))
              == COM_SOCKETS_ERR_OK)
          {
            p_socket->state = CELLULAR_APP_SOCKET_CREATED;
            PRINT_INFO("%s %d: Socket setsockopt OK", cellular_app_type_string[type], index)
          }
          else
          {
            PRINT_INFO("%s %d: Socket setsockopt SNDTIMEO NOK!", cellular_app_type_string[type], index)
          }
        }
        else
        {
          PRINT_INFO("%s %d: Socket setsockopt RCVTIMEO NOK!", cellular_app_type_string[type], index)
        }

        if (p_socket->state != CELLULAR_APP_SOCKET_CREATED)
        {
          /* Issue during socket creation - close socket to restart properly */
          cellular_app_socket_close(type, index, p_socket);
        }
      }
      else
      {
        PRINT_INFO("%s %d: Socket create NOK!", cellular_app_type_string[type], index)
      }
    }

    /* According to the socket Protocol call or not the connect */
    if (p_socket->state == CELLULAR_APP_SOCKET_CREATED)
    {
      if ((p_socket->protocol == CELLULAR_APP_SOCKET_TCP_PROTO)
          || (p_socket->protocol == CELLULAR_APP_SOCKET_UDP_PROTO))
      {
        com_sockaddr_in_t address;
        /* Connect must be called */
        PRINT_DBG("%s %d: Socket connect rqt", cellular_app_type_string[type], index)
        address.sin_family      = (uint8_t)COM_AF_INET;
        address.sin_port        = COM_HTONS(p_socket->distant.port);
        address.sin_addr.s_addr = CELLULAR_APP_GET_DISTANTIP(p_socket->distant.ip);

        if (com_connect(p_socket->id, (com_sockaddr_t const *)&address, (int32_t)sizeof(com_sockaddr_in_t))
            == COM_SOCKETS_ERR_OK)
        {
          /* Connect is ok, reset nfm counters, increase counters and put socket state in connected */
          p_socket->nfm.error_current_nb = 0U;
          p_socket->nfm.index = 0U;
          p_socket->stat.connect.ok++;
          p_socket->state = CELLULAR_APP_SOCKET_CONNECTED;
          result = true;
          PRINT_INFO("%s %d: Socket connect OK", cellular_app_type_string[type], index)
        }
        else
        {
          /* Connect is ko, increase fault counters */
          p_socket->nfm.error_current_nb++;
          p_socket->stat.connect.ko++;
          PRINT_INFO("%s %d: Socket NOK! Closing the socket!", cellular_app_type_string[type], index)
          /* Issue during socket connection - close socket to restart properly */
          cellular_app_socket_close(type, index, p_socket);
          /* Maybe distant.ip is no more ok, then if distant.name is known, force next time a DNS network resolution */
          if (crs_strlen(p_socket->distant.p_name) > 0U)
          {
            PRINT_INFO("%s %d: Distant IP reset to force a new DNS network resolution next time!",
                       cellular_app_type_string[type], index)
            CELLULAR_APP_SET_DISTANTIP_NULL(&(p_socket->distant.ip));
          }
        }
      }
      else /* p_socket->protocol == CELLULAR_APP_SOCKET_UDPSERVICE_PROTO */
      {
        /* Connect is not needed */
        result = true;
      }
    }
  }

  return (result);
}

/**
  * @brief  Close socket if it was opened
  * @param  type     - application type
  * @param  index    - application index
  * @param  p_socket - pointer on the socket to use
  * @retval -
  */
void cellular_app_socket_close(cellular_app_type_t type, uint8_t index, cellular_app_socket_desc_t *p_socket)
{
#if (USE_TRACE_APPLICATION == 0U)
  UNUSED(index); /* parameter only used in PRINT_INFO */
#endif /* USE_TRACE_APPLICATION == 0U */

  if (p_socket->state != CELLULAR_APP_SOCKET_INVALID)
  {
    switch (type)
    {
      case CELLULAR_APP_TYPE_ECHOCLIENT :
        if (com_closesocket(p_socket->id) == COM_SOCKETS_ERR_OK)
        {
          /* close socket ok - increase counters and put socket state in invalid */
          p_socket->stat.close.ok++;
          p_socket->state = CELLULAR_APP_SOCKET_INVALID;
          p_socket->id = COM_SOCKET_INVALID_ID;
          PRINT_INFO("%s %d: Socket close OK", cellular_app_type_string[CELLULAR_APP_TYPE_ECHOCLIENT], index)
        }
        else
        {
          /* close socket ko - increase fault counters and put socket state in closing */
          p_socket->stat.close.ko++;
          p_socket->state = CELLULAR_APP_SOCKET_CLOSING;
          PRINT_INFO("%s %d: Socket close NOK!", cellular_app_type_string[CELLULAR_APP_TYPE_ECHOCLIENT], index)
        }
        p_socket->closing = false;
        break;

      case CELLULAR_APP_TYPE_PINGCLIENT :
        if (com_closeping(p_socket->id) == COM_SOCKETS_ERR_OK)
        {
          /* close ping ok */
          p_socket->state = CELLULAR_APP_SOCKET_INVALID;
          p_socket->id = COM_SOCKET_INVALID_ID;
          PRINT_INFO("%s: Session close OK", cellular_app_type_string[CELLULAR_APP_TYPE_PINGCLIENT])
        }
        else
        {
          p_socket->state = CELLULAR_APP_SOCKET_CLOSING;
          PRINT_INFO("%s: Session close NOK!", cellular_app_type_string[CELLULAR_APP_TYPE_PINGCLIENT])
        }
        p_socket->closing = false;
        break;

      default :
        break;
    }
  }
}

/**
  * @brief  Set send buffer length of a specific CellularApp application
  * @param  type              - application type to change
  * @param  index             - application index to change
  * @param  snd_buffer_len    - send buffer length new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_set_snd_buffer_len(cellular_app_type_t type, uint8_t index, uint16_t snd_buffer_len)
{
  bool result = false;

  /* Change authorized if no performance test in progress */
  if (cellular_app_get_performance_status() == false)
  {
    switch (type)
    {
      case CELLULAR_APP_TYPE_ECHOCLIENT :
        result = cellular_app_echoclient_set_snd_buffer_len(index, snd_buffer_len);
        break;
      case CELLULAR_APP_TYPE_PINGCLIENT : /* Not supported */
      default :
        __NOP(); /* result already set to false */
        break;
    }
  }
  else
  {
    PRINT_FORCE("%s: Performance in progress! Wait its end before to retry!",
                cellular_app_type_string[CELLULAR_APP_TYPE_CELLULARAPP])
  }

  return (result);
}

/**
  * @brief  Set protocol of a specific CellularApp application
  * @param  type     - application type to change
  * @param  index    - application index to change
  * @param  protocol - protocol new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_set_protocol(cellular_app_type_t type, uint8_t index, cellular_app_socket_protocol_t protocol)
{
  bool result = false;

  /* Change authorized if no performance test in progress */
  if (cellular_app_get_performance_status() == false)
  {
    switch (type)
    {
      case CELLULAR_APP_TYPE_ECHOCLIENT :
        result = cellular_app_echoclient_set_protocol(index, protocol);
        break;
      case CELLULAR_APP_TYPE_PINGCLIENT : /* Not supported */
      default :
        __NOP(); /* result already set to false */
        break;
    }
  }
  else
  {
    PRINT_FORCE("%s: Performance in progress! Wait its end before to retry!",
                cellular_app_type_string[CELLULAR_APP_TYPE_CELLULARAPP])
  }

  return (result);
}

/**
  * @brief  Set distant of a specific CellularApp application
  * @param  type              - application type to change
  * @param  index             - application index to change
  * @param  process_status    - process status (if = OFF then distant can be updated immediately)
  * @param  distant_type      - distant type value
  * @param  p_distantip       - distant ip value  (supported for PingClt only)
  * @param  distantip_len     - distant ip length (supported for PingClt only)
  * @param  p_distant_current -  pointer on the distant current
  * @param  p_socket_change   -  pointer on the socket change
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_distant_change(cellular_app_type_t type, uint8_t index, cellular_app_process_status_t process_status,
                                 cellular_app_distant_type_t distant_type, uint8_t *p_distantip, uint32_t distantip_len,
                                 cellular_app_distant_desc_t  *p_distant_current,
                                 cellular_app_socket_change_t *p_socket_change)
{
  bool result = false;
  uint8_t ip_addr[4];  /* Used to analyse p_distantip content */

  if ((p_distant_current != NULL) && (p_socket_change != NULL)) /* Pointers ok ? */
  {
    /* Change authorized if no performance test in progress */
    if (cellular_app_get_performance_status() == false)
    {
      /* Only one modification at a time */
      if (p_distant_current->type != p_socket_change->distant_type)
      {
        PRINT_FORCE("%s %d: Distant change already in progress!", cellular_app_type_string[type], (index + 1U))
      }
      /* Is a distant change really requested ? */
      else if ((distant_type != p_socket_change->distant_type) || (distant_type == CELLULAR_APP_DISTANT_IPx_TYPE))
      {
        /* Update distant value ? */
        switch (distant_type)
        {
          case CELLULAR_APP_DISTANT_IP1_TYPE    :
          case CELLULAR_APP_DISTANT_IP2_TYPE    :
            p_socket_change->distant_type = distant_type;
            result = true;
            break;
          case CELLULAR_APP_DISTANT_IPx_TYPE    :
            /* Analyse p_distant_ip */
            if ((p_distantip != NULL) && (distantip_len > 0U))
            {
              if ((crc_get_ip_addr(p_distantip, ip_addr, NULL) == 0U)
                  && (ip_addr[0] != 0U) && (ip_addr[1] != 0U) && (ip_addr[2] != 0U) && (ip_addr[3] != 0U))
              {
                p_socket_change->distant_type = distant_type;
                /* Update CELLULAR_APP_DISTANT_IPx distant ip */
                cellular_app_distant_ip[CELLULAR_APP_DISTANT_IPx_TYPE][0] = ip_addr[0];
                cellular_app_distant_ip[CELLULAR_APP_DISTANT_IPx_TYPE][1] = ip_addr[1];
                cellular_app_distant_ip[CELLULAR_APP_DISTANT_IPx_TYPE][2] = ip_addr[2];
                cellular_app_distant_ip[CELLULAR_APP_DISTANT_IPx_TYPE][3] = ip_addr[3];
                /* Analyze is ok, set to a current value */
                result = true;
              }
            }
            break;
          case CELLULAR_APP_DISTANT_MBED_TYPE   :
          case CELLULAR_APP_DISTANT_UBLOX_TYPE  :
          case CELLULAR_APP_DISTANT_LOCAL_TYPE  :
            p_socket_change->distant_type = distant_type;
            result = true;
            break;
          case CELLULAR_APP_DISTANT_ACTUAL_TYPE :
            result = true;
            break;
          case CELLULAR_APP_DISTANT_TYPE_MAX    :
          default :
            __NOP(); /* result already set to false */
            break;
        }
        if (result == true) /* Does analysis go well ? */
        {
          if (type == CELLULAR_APP_TYPE_ECHOCLIENT)
          {
            PRINT_FORCE("%s %d: Distant set to %s in progress...", cellular_app_type_string[type], (index + 1U),
                        cellular_app_distant_string[distant_type])
            p_socket_change->distant_type = distant_type;
            if (process_status == CELLULAR_APP_PROCESS_OFF)
            {
              /* Protocol change immediately because no impact on distant server parameters */
              p_distant_current->type = p_socket_change->distant_type;
              cellular_app_distant_update(distant_type, p_distant_current);
              PRINT_FORCE("%s %d: Distant set to %s OK", cellular_app_type_string[type], (index + 1U),
                          cellular_app_distant_string[distant_type])
            }
          }
          /* for PingClt this function call means do a ping */
        }
      }
      else /* Distant already on the requested value */
      {
        if (type == CELLULAR_APP_TYPE_ECHOCLIENT)
        {
          PRINT_FORCE("%s %d: Distant already on %s!", cellular_app_type_string[type], (index + 1U),
                      cellular_app_distant_string[distant_type])
        }
        else
        {
          /* for PingClt this function call means do a ping */
          result = true;
        }
      }
    }
  }

  return (result);
}

/**
  * @brief  Get socket statistics of a specific CellularApp application
  * @param  type   - application type to get statistics
  * @param  index  - application index to get statistics
  * @param  p_stat - statistics result pointer
  * @retval bool   - false/true - application not found / application found, *p_stat provided
  */
bool cellular_app_socket_get_stat(cellular_app_type_t type, uint8_t index, cellular_app_socket_stat_desc_t *p_stat)
{
  bool result = false;

  if (p_stat != NULL)
  {
    switch (type)
    {
      case CELLULAR_APP_TYPE_ECHOCLIENT :
        if (cellular_app_echoclient_get_socket_stat(index, p_stat) == true)
        {
          result = true;
        }
        /* else __NOP because result already set to false */
        break;
      case CELLULAR_APP_TYPE_PINGCLIENT :
      default :
        __NOP(); /* because result already set to false */
        break;
    }
  }
  /* else __NOP because result already set to false */

  return (result);
}

/**
  * @brief  Reset socket statistics of a specific CellularApp application
  * @param  type  - application type to reset statistics
  * @param  index - application index to reset statisctics
  * @retval -
  */
void cellular_app_socket_reset_stat(cellular_app_type_t type, uint8_t index)
{
  switch (type)
  {
    case CELLULAR_APP_TYPE_ECHOCLIENT :
      cellular_app_echoclient_reset_socket_stat(index);
      break;
    case CELLULAR_APP_TYPE_PINGCLIENT :
    default :
      __NOP();
      break;
  }
}

/**
  * @brief  Initialize a socket: state, closing, protocol, id fields only
  * @param  p_socket        -  pointer on the socket change
  * @param  p_socket_change -  pointer on the socket change
  * @retval -
  */
void cellular_app_socket_init(cellular_app_socket_desc_t *p_socket, cellular_app_socket_change_t *p_socket_change)
{
  if ((p_socket != NULL) && (p_socket_change != NULL))
  {
    p_socket->state = CELLULAR_APP_SOCKET_INVALID;
    p_socket->closing = false;
    /* Socket protocol initialization : if supported default protocol value is UDP not-connected
     *                                  if not supported then default protocol value is UDP */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* Set default socket Protocol value according to modem capacity */
#if (UDP_SERVICE_SUPPORTED == 1U)
    p_socket->protocol = CELLULAR_APP_SOCKET_UDP_SERVICE_PROTO;
#else /* UDP_SERVICE_SUPPORTED == 0U */
    p_socket->protocol = CELLULAR_APP_SOCKET_UDP_PROTO;
#endif /* UDP_SERVICE_SUPPORTED == 1U */

#else /* USE_SOCKETS_TYPE != USE_SOCKETS_MODEM */
    p_socket->protocol = CELLULAR_APP_SOCKET_UDP_SERVICE_PROTO;
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */

    /* Initialize id to invalid */
    p_socket->id = COM_SOCKET_INVALID_ID;

    /* Initialize change protocol to protocol */
    p_socket_change->protocol = p_socket->protocol;
  }
}

#endif /* USE_CELLULAR_APP == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
