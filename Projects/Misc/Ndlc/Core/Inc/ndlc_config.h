/**
  ******************************************************************************
  * @file    ndlc_config.h
  * @author  MCD Application Team
  * @brief   Ndlc Configuration definition
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef NDLC_CONFIG_H
#define NDLC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_ST33 == 1)

/* Exported constants --------------------------------------------------------*/
typedef enum
{
  NDLC_IDLE = 0,
  NDLC_SENDCMD,
  NDLC_READACK,
  NDLC_READHEADER,
  NLDC_READDATA,
  NLDC_SENDACK
} ndlc_status;

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  void *handlePtr;  /* Handle to a physical link */
  ndlc_status status;
  uint16_t dataLen;
  uint8_t *data;
} ndlc_device_t;

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
#if (NDLC_INTERFACE == SPI_INTERFACE)

#include "spi_ndlc.h"
#include "spi_ndlc_device.h"

#define NDLC_INIT              spi_ndlc_init
#define NDLC_DEINIT            spi_ndlc_deinit
#define NDLC_POWER             spi_ndlc_power
#define NDLC_ATR               spi_ndlc_atr
#define NDLC_SEND_RECEIVE_APDU spi_ndlc_transceive_apdu

#define NDLC_SEND_RECEIVE_PHY  spi_ndlc_send_receive_phy

#else

#define NDLC_INIT
#define NDLC_DEINIT
#define NDLC_POWER
#define NDLC_ATR
#define NDLC_SEND_RECEIVE_APDU

#define NDLC_SEND_RECEIVE_PHY

#endif /* NDLC_INTERFACE == SPI_INTERFACE */

#endif /* USE_ST33 == 1 */

#ifdef __cplusplus
}
#endif

#endif /* NDLC_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
