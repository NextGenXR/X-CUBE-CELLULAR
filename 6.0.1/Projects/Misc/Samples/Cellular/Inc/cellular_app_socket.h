/**
  ******************************************************************************
  * @file    cellular_app_socket.h
  * @author  MCD Application Team
  * @brief   Header for cellular_app_socket.c module (socket and distant services)
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
#ifndef CELLULAR_APP_SOCKET_H
#define CELLULAR_APP_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_CELLULAR_APP == 1)
#include <stdbool.h>
#include <stdint.h>

#include "cellular_app.h"

#include "com_sockets.h" /* includes all other includes */

/* Exported constants --------------------------------------------------------*/

#define CELLULAR_APP_DISTANT_UNKNOWN_NAME              ((uint8_t *)"\0") /* Set to to by-pass DNS resolution */
#define CELLULAR_APP_DISTANT_UNKNOWN_PORT              ((uint16_t)0xFFFFU)
#define CELLULAR_APP_DISTANT_NO_WELCOME_MSG            NULL

/* Define to access or modify an IP */
#define CELLULAR_APP_GET_DISTANTIP(a)                  ((a).addr)
#define CELLULAR_APP_SET_DISTANTIP(a, b)               ((a)->addr = (b))
#define CELLULAR_APP_SET_DISTANTIP_NULL(a)             ((a)->addr = (uint32_t)0U)

/* Exported types ------------------------------------------------------------*/
/* Socket state */
typedef enum
{
  CELLULAR_APP_SOCKET_INVALID = 0,
  CELLULAR_APP_SOCKET_CREATED,
  CELLULAR_APP_SOCKET_CONNECTED,
  CELLULAR_APP_SOCKET_SENDING,
  CELLULAR_APP_SOCKET_WAITING_RSP,
  CELLULAR_APP_SOCKET_CLOSING
} cellular_app_socket_state_t;

/* Socket protocol */
typedef enum
{
  CELLULAR_APP_SOCKET_TCP_PROTO = 0,     /* create, connect, send,   recv     */
  CELLULAR_APP_SOCKET_UDP_PROTO,         /* create, connect, send,   recv     */
  CELLULAR_APP_SOCKET_UDP_SERVICE_PROTO, /* create,   NA   , sendto, recvfrom */
  CELLULAR_APP_SOCKET_PROTO_MAX          /* Must always be the last value     */
} cellular_app_socket_protocol_t;

/* Distant type */
typedef uint8_t cellular_app_distant_type_t;
#define CELLULAR_APP_DISTANT_MBED_TYPE          (cellular_app_distant_type_t)0    /* MBED   distant                */
#define CELLULAR_APP_DISTANT_UBLOX_TYPE         (cellular_app_distant_type_t)1    /* UBLOX  distant                */
#define CELLULAR_APP_DISTANT_LOCAL_TYPE         (cellular_app_distant_type_t)2    /* LOCAL  distant                */
#define CELLULAR_APP_DISTANT_IP1_TYPE           (cellular_app_distant_type_t)3    /* IP1    distant                */
#define CELLULAR_APP_DISTANT_IP2_TYPE           (cellular_app_distant_type_t)4    /* IP2    distant                */
#define CELLULAR_APP_DISTANT_IPx_TYPE           (cellular_app_distant_type_t)5    /* IPx    distant                */
#define CELLULAR_APP_DISTANT_ACTUAL_TYPE        (cellular_app_distant_type_t)6    /* Actual distant                */
#define CELLULAR_APP_DISTANT_TYPE_MAX           (cellular_app_distant_type_t)7    /* Must always be the last value */

/* Socket Distant descriptor */
typedef struct
{
  cellular_app_distant_type_t type; /* Distant Type */
  uint16_t      port;               /* Distant Port */
  com_ip_addr_t ip;                 /* Distant IP   */
  uint8_t       *p_name;            /* Distant name */
  uint8_t       *p_tcp_welcome_msg; /* Distant TCP welcome msg pointer */
  uint8_t       *p_udp_welcome_msg; /* Distant UDP welcome msg pointer */
} cellular_app_distant_desc_t;

/* Socket NFM descriptor */
typedef struct
{
  uint8_t error_current_nb; /* Current errors nb in NFM feature         */
  uint8_t error_limit_nb;   /* Limit   errors nb before to activate NFM */
  uint8_t index;            /* Sleep timer index in the NFM array       */
} cellular_app_socket_nfm_desc_t;

/* Socket Statistics counters */
typedef struct
{
  uint32_t ok; /* count number ok */
  uint32_t ko; /* count number ko */
} cellular_app_socket_counter_t; /* ToDo: uint16_t sounds enough */

/* Socket Statistics counters */
typedef struct
{
  uint32_t process_counter;              /* process count number */ /* ToDo: uint16_t seems enough */
  cellular_app_socket_counter_t connect; /* connect count number */
  cellular_app_socket_counter_t send;    /* send    count number */
  cellular_app_socket_counter_t receive; /* receive count number */
  cellular_app_socket_counter_t close;   /* close   count number */
} cellular_app_socket_stat_desc_t;

/* Socket descriptor */
typedef struct
{
  cellular_app_socket_state_t state;          /* Socket state */
  bool closing;                               /* false: socket don't need to be closed,
                                               * true:  socket need to be closed */
  cellular_app_socket_protocol_t protocol;    /* Protocol to use              */
  uint16_t snd_buffer_len;                    /* Length of the buffer to send */
  uint16_t snd_rcv_timeout;                   /* Send / Receive timeout       */
  uint8_t  *p_snd_buffer;                     /* Pointer on buffer to send to the distant */
  uint8_t  *p_rcv_buffer;                     /* Pointer on buffer to store the response of the distant */
  int32_t id;                                 /* Socket id = result of com_socket() */
  cellular_app_distant_desc_t distant;        /* Distant descriptor */
  cellular_app_socket_nfm_desc_t  nfm;        /* NFM     descriptor ToDo: change this to pointer */
  cellular_app_socket_stat_desc_t stat;       /* Socket  statistic  ToDo: change this to pointer */
} cellular_app_socket_desc_t;

/* Socket change structure */
typedef struct
{
  cellular_app_distant_type_t    distant_type;      /* Distant Type         */
  com_ip_addr_t distant_ip;                         /* Distant IP           */
  cellular_app_socket_protocol_t protocol;          /* Protocol to use      */
  uint16_t snd_buffer_len;                          /* Send buffer length   */
} cellular_app_socket_change_t;

/* External variables --------------------------------------------------------*/
/* Socket distant IP */
extern uint8_t cellular_app_distant_ip[CELLULAR_APP_DISTANT_TYPE_MAX - 1][4];

/* String used to display socket distant */
extern const uint8_t *cellular_app_distant_string[CELLULAR_APP_DISTANT_TYPE_MAX];

/* String used to display socket protocol */
extern const uint8_t *cellular_app_protocol_string[CELLULAR_APP_SOCKET_PROTO_MAX];

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Check Distant server data - Update Distant server IP
  * @note   Decide if Network DNS resolver has to be called
  * @param  type              - application type
  * @param  index             - application index
  * @param  p_distant         - pointer on distant server structure
  * @retval bool - false/true - distant ip is NOK / OK
  */
bool cellular_app_distant_check(cellular_app_type_t type, uint8_t index, cellular_app_distant_desc_t *p_distant);

/**
  * @brief  Update Distant data
  * @param  distant_type - New Distant type
  * @param  p_distant    - new distant value to update
  * @retval -
  */
void cellular_app_distant_update(cellular_app_distant_type_t distant_type, cellular_app_distant_desc_t *p_distant);

/**
  * @brief  Check if NFM sleep has to be done
  * @param  p_socket          - pointer on the socket to use
  * @param  p_nfmc_tempo      - pointer on the nfmc value to use
  * @retval bool - false/true - NFM sleep hasn't to be done/has to be done
  */
bool cellular_app_socket_is_nfm_sleep_requested(cellular_app_socket_desc_t *p_socket, uint32_t *p_nfmc_tempo);

/**
  * @brief  Obtain socket
  * @note   If needed close the socket before to create a new one according to the change requested
  * @param  type              - application type
  * @param  index             - application index
  * @param  p_socket          - pointer on the socket to use
  * @param  p_welcome_msg     - pointer on the welcome msg
  * @param  p_socket_change   - pointer on socket change requested
  * @retval bool - true/false - socket creation OK/NOK
  */
bool cellular_app_socket_obtain(cellular_app_type_t type, uint8_t index,
                                cellular_app_socket_desc_t *p_socket, uint8_t **p_welcome_msg,
                                cellular_app_socket_change_t *p_socket_change);


/**
  * @brief  Close socket if it was opened
  * @param  type     - application type
  * @param  index    - application index
  * @param  p_socket - pointer on the socket to use
  * @retval -
  */
void cellular_app_socket_close(cellular_app_type_t type, uint8_t index, cellular_app_socket_desc_t *p_socket);

/**
  * @brief  Set send buffer length of a specific CellularApp application
  * @param  type              - application type to change
  * @param  index             - application index to change
  * @param  snd_buffer_len    - send buffer length new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_set_snd_buffer_len(cellular_app_type_t type, uint8_t index, uint16_t snd_buffer_len);

/**
  * @brief  Set protocol of a specific CellularApp application
  * @param  type     - application type to change
  * @param  index    - application index to change
  * @param  protocol - protocol new value to set
  * @retval bool     - false/true - application not updated / application update in progress
  */
bool cellular_app_set_protocol(cellular_app_type_t type, uint8_t index, cellular_app_socket_protocol_t protocol);

/**
  * @brief  Change distant of a specific CellularApp application
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
                                 cellular_app_socket_change_t *p_socket_change);

/**
  * @brief  Get socket statistics of a specific CellularApp application
  * @param  type   - application type to change
  * @param  index  - application index to change
  * @param  p_stat - statistics result pointer
  * @retval bool   - false/true - application not found / application found and *p_stat provided
  */
bool cellular_app_socket_get_stat(cellular_app_type_t type, uint8_t index, cellular_app_socket_stat_desc_t *p_stat);

/**
  * @brief  Reset statistics of a specific CellularApp application
  * @param  type  - application type to change
  * @param  index - application index to change
  * @retval -
  */
void cellular_app_socket_reset_stat(cellular_app_type_t type, uint8_t index);

/**
  * @brief  Initialize a socket: state, closing, protocol, id fields only
  * @param  p_socket        -  pointer on the socket change
  * @param  p_socket_change -  pointer on the socket change
  * @retval -
  */
void cellular_app_socket_init(cellular_app_socket_desc_t *p_socket, cellular_app_socket_change_t *p_socket_change);

#endif /* USE_CELLULAR_APP == 1 */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_APP_SOCKET_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
