/**
  ******************************************************************************
  * @file    cellular_app_sensors.h
  * @author  MCD Application Team
  * @brief   Header for cellular_app_sensors.c module
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


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CELLULAR_APP_SENSORS_H
#define CELLULAR_APP_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_SENSORS == 1)

#include <stdbool.h>
#include <stdint.h>
#include <float.h>

/* Exported types ------------------------------------------------------------*/
typedef uint8_t cellular_app_sensor_type_t;
#define CELLULAR_APP_SENSOR_TYPE_HUMIDITY      (cellular_app_sensor_type_t)0
#define CELLULAR_APP_SENSOR_TYPE_PRESSURE      (cellular_app_sensor_type_t)1
#define CELLULAR_APP_SENSOR_TYPE_TEMPERATURE   (cellular_app_sensor_type_t)2

typedef struct
{
  float float_data; /* used for  Humidity/PRESSURE/TEMPERATURE */
} cellular_app_sensors_data_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  Initialize a sensor
  * @retval bool - false/true - sensor init NOK/ sensor init OK
  */
bool cellular_app_sensors_initialize(cellular_app_sensor_type_t type);

/**
  * @brief  Read a sensor
  * @retval bool - false/true - sensor read NOK/ sensor read OK
  */
bool cellular_app_sensors_read(cellular_app_sensor_type_t type,
                               cellular_app_sensors_data_t *const p_data);

/**
  * @brief  Initialize sensor module
  * @retval -
  */
void cellular_app_sensors_init(void);

#endif /* USE_SENSORS == 1 */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_APP_SENSORS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
