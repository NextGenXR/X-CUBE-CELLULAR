/**
  ******************************************************************************
  * @file    cellular_app_sensors.c
  * @author  MCD Application Team
  * @brief   Implements functions for sensors actions.
  *          Supported sensors : humidity, pressure, temperature
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
#include "cellular_app_sensors.h"

#if (USE_SENSORS == 1)

/* Private typedef -----------------------------------------------------------*/
/* Cellular App Sensor descriptor */
typedef struct
{
  bool status;/* Sensor Status: false: not initialized, true: initialized */
} cellular_app_sensor_desc_t;

/* Private defines -----------------------------------------------------------*/
#define CELLULAR_APP_SENSOR_TYPE_MAX (CELLULAR_APP_SENSOR_TYPE_TEMPERATURE + 1U)

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static cellular_app_sensor_desc_t cellular_app_sensor[CELLULAR_APP_SENSOR_TYPE_MAX];

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private Functions Definition ----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Initialize a sensor
  * @retval bool - false/true - sensor init NOK / sensor init OK
  */
bool cellular_app_sensors_initialize(cellular_app_sensor_type_t type)
{
  bool result = false;

  switch (type)
  {
    case CELLULAR_APP_SENSOR_TYPE_HUMIDITY :
      if (BSP_ENV_SENSOR_Init_Humidity() == BSP_ERROR_NONE)
      {
        result = true;
      }
      break;
    case CELLULAR_APP_SENSOR_TYPE_PRESSURE :
      if (BSP_ENV_SENSOR_Init_Pressure() == BSP_ERROR_NONE)
      {
        result = true;
      }
      break;
    case CELLULAR_APP_SENSOR_TYPE_TEMPERATURE :
      if (BSP_ENV_SENSOR_Init_Temperature() == BSP_ERROR_NONE)
      {
        result = true;
      }
      break;
    default :
      __NOP(); /* result already set to false */
      break;
  }

  if (type < CELLULAR_APP_SENSOR_TYPE_MAX)
  {
    cellular_app_sensor[type].status = result;
  }

  return (result);
}

/**
  * @brief  Read a sensor
  * @retval bool - false/true - sensor read NOK/ sensor read OK
  */
bool cellular_app_sensors_read(cellular_app_sensor_type_t type,
                               cellular_app_sensors_data_t *const p_data)
{
  bool result = false;

  if (p_data != NULL)
  {
    switch (type)
    {
      case CELLULAR_APP_SENSOR_TYPE_HUMIDITY :
        if (cellular_app_sensor[CELLULAR_APP_SENSOR_TYPE_HUMIDITY].status == true)
        {
          if (BSP_ENV_SENSOR_ReadT_Humidity(&(p_data->float_data)) == BSP_ERROR_NONE)
          {
            result = true;
          }
        }
        break;
      case CELLULAR_APP_SENSOR_TYPE_PRESSURE :
        if (cellular_app_sensor[CELLULAR_APP_SENSOR_TYPE_PRESSURE].status == true)
        {
          if (BSP_ENV_SENSOR_Read_Pressure(&(p_data->float_data)) == BSP_ERROR_NONE)
          {
            result = true;
          }
        }
        break;
      case CELLULAR_APP_SENSOR_TYPE_TEMPERATURE :
        if (cellular_app_sensor[CELLULAR_APP_SENSOR_TYPE_TEMPERATURE].status == true)
        {
          if (BSP_ENV_SENSOR_Read_Temperature(&(p_data->float_data)) == BSP_ERROR_NONE)
          {
            result = true;
          }
        }
        break;
      default :
        __NOP(); /* result already set to false */
        break;
    }
  }

  return (result);
}

/**
  * @brief  Initialize sensor module
  * @retval -
  */
void cellular_app_sensors_init(void)
{
  for (uint8_t i = 0; i < CELLULAR_APP_SENSOR_TYPE_MAX; i++)
  {
    cellular_app_sensor[i].status = false;
  }
}

#endif /* USE_SENSORS == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
