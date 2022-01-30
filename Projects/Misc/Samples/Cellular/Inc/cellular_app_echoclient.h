/**
  ******************************************************************************
  * @file    cellular_app_echoclient.h
  * @author  MCD Application Team
  * @brief   Header for cellular_app_echoclient.c module
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
#ifndef CELLULAR_APP_ECHOCLIENT_H
#define CELLULAR_APP_ECHOCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_CELLULAR_APP == 1)
#include <stdbool.h>
#include <stdint.h>

#include "cellular_app_socket.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/**
  * @brief  Send a message to EchoClt
  * @param  index     - EchoClt index - if 0xFF then send to all EchoClt
  * @param  queue_msg - Message to send
  * @retval bool      - false/true - Message not send / Message send
  */
bool cellular_app_echoclient_send_msg(uint8_t index, uint32_t queue_msg);

/**
  * @brief  Get status of a specific EchoClt application
  * @param  index - EchoClt index
  * @retval cellular_app_process_status_t - EchoClt application process status
  */
cellular_app_process_status_t cellular_app_echoclient_get_status(uint8_t index);

/**
  * @brief  Set status of a specific EchoClt application
  * @param  index             - EchoClt index to change
  * @param  process_status    - process status new value to set inactive/active
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_set_status(uint8_t index, bool process_status);

/**
  * @brief  Set period of a specific EchoClt application
  * @param  index             - EchoClt index to change
  * @param  process_period    - new period process value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_set_period(uint8_t index, uint32_t process_period);

/**
  * @brief  Set send buffer length of a specific EchoClt application
  * @param  index             - EchoClt index to change
  * @param  snd_buffer_len    - send buffer length new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_set_snd_buffer_len(uint8_t index, uint16_t snd_buffer_len);

/**
  * @brief  Set protocol of a specific EchoClt application
  * @param  index    - EchoClt index to change
  * @param  protocol - protocol new value to set
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_set_protocol(uint8_t index, cellular_app_socket_protocol_t protocol);

/**
  * @brief  Change distant of a specific EchoClt application
  * @param  index             - EchoClt index to change
  * @param  distant_type      - distant type value
  * @param  p_distantip       - distant ip value  (supported for PingClt only)
  * @param  distantip_len     - distant ip length (supported for PingClt only)
  * @retval bool - false/true - application not updated / application update in progress
  */
bool cellular_app_echoclient_distant_change(uint8_t index, cellular_app_distant_type_t distant_type,
                                            uint8_t *p_distantip, uint32_t distantip_len);

/**
  * @brief  Get EchoClt socket statistics
  * @param  index  - application index to get statistics
  * @param  p_stat - statistics result pointer
  * @retval bool   - false/true - application not found / application found, *p_stat provided
  */
bool cellular_app_echoclient_get_socket_stat(uint8_t index, cellular_app_socket_stat_desc_t *p_stat);

/**
  * @brief  Reset EchoClt statistics
  * @param  index - application index to change
  * @retval -
  */
void cellular_app_echoclient_reset_socket_stat(uint8_t index);

/**
  * @brief  Provide EchoClt performance feature status
  * @param  -
  * @retval bool - false/true - not started / started
  */
bool cellular_app_echoclient_get_performance_status(void);

/**
  * @brief  EchoClt performance feature
  * @param  status  - false/true - performance to stop/performance to start
  * @param  iter_nb - iteration number (0: default value to use)
  * @retval bool    - false/true - not done/done
  */
bool cellular_app_echoclient_performance(bool status, uint8_t iter_nb);

/**
  * @brief  Display EchoClt status
  * @param  -
  * @retval -
  */
void cellular_app_echoclient_display_status(void);

/**
  * @brief  Initialize all needed structures to support EchoClt feature
  * @param  -
  * @retval -
  */
void cellular_app_echoclient_init(void);

/**
  * @brief  Start all EchoClt threads
  * @param  -
  * @retval -
  */
void cellular_app_echoclient_start(void);

#endif /* USE_CELLULAR_APP == 1 */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_APP_ECHOCLIENT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
