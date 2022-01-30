/**
  ******************************************************************************
  * @file    cellular_app_datetime.h
  * @author  MCD Application Team
  * @brief   Header for cellular_app_datetime.c module
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
#ifndef CELLULAR_APP_DATETIME_H
#define CELLULAR_APP_DATETIME_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_RTC == 1)

#include <stdint.h>
#include <stdbool.h>

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Time structure */
typedef struct
{
  uint8_t sec;  /* Seconds : 00 - 59 */
  uint8_t min;  /* Minutes : 00 - 59 */
  uint8_t hour; /* Hours   : 00 - 23 - means format AM/PM not managed */
} cellular_app_time_t;

/* Date structure */
typedef struct
{
  uint8_t  week_day;   /* Days since Monday      : 01 (means Monday)  - 07 (means Sunday)      */
  uint8_t  month_day;  /* Day of the month       : 01 - 31                                     */
  uint8_t  month;      /* Month                  : 01 (means January) - 12 (means December)    */
  uint8_t  year;       /* Year since year_start; e.g value 21 means year_start + 21*/
  uint16_t year_start; /* RTC is managing a value between [0, 99] so when setting the date this value can be used
                        * set RTC year to a value less than 99
                        * e.g with year = 21 and year_start = 2000, RTC.year = 21 < 99 */
} cellular_app_date_t;

/* Date and Time structure */
typedef struct
{
  cellular_app_time_t time; /* Time */
  cellular_app_date_t date; /* Date */
} cellular_app_datetime_t;

/**
  * @brief     Callback definition used to inform about date time set.
  * @note      Callback is called only after a cellular_app_datetime_set() and not every ms or every minutes.
  * @param[in] p_callback_ctx - context to be passed when cellular_app_datetime_set_registration_cb_t is called.
  */
typedef void (* cellular_app_datetime_set_registration_cb_t)(void const *p_callback_ctx);

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Convert a date and time using a string to a cellular_app_datetime_t format.
  * @param  datetime_str_len  - length of the data pointed by p_datetime_str
  * @param  p_datetime_str    - provided datetime string. Format is: Day MonthDay Month Year Hour Minutes Seconds
  *         e.g: Mon 15 Nov 2021 13:50:10 - Day and Month must be the first 3 characters in English
  * @note   RTC last possible value is 99 for year do not forget to check year_start value
  * @param  p_datetime        - datetime in cellular_app_datetime_t format
  * @retval bool - false/true - conversion failed / conversion is ok
  */
bool cellular_app_datetime_str_convert(uint8_t datetime_str_len, uint8_t const *p_datetime_str,
                                       cellular_app_datetime_t *p_datetime);

/**
  * @brief  Set date and time.
  * @param  p_datetime        - provided datetime
  * @note   RTC last possible value is 99 for year do not forget to use year_start in p_datetime
  * @retval bool - false/true - update not done / update done
  */
bool cellular_app_datetime_set(cellular_app_datetime_t *p_datetime);

/**
  * @brief  Get date and time.
  * @param  p_datetime        - datetime pointer
  * @note   add p_datetime->date.year_start to p_datetime->date.year to have a year formatted 4 digits
  * @retval bool - false/true - datetime not yet set / datetime set
  */
bool cellular_app_datetime_get(cellular_app_datetime_t *p_datetime);

/**
  * @brief  Date and time callback registration.
  * @note   only one callback can be registered
  * @param  registration_cb - callback to register
  * @param  p_callback_ctx  - context to be passed when cellular_app_datetime_set_registration_cb() is called.
  * @retval -
  */
void cellular_app_datetime_set_cb_registration(cellular_app_datetime_set_registration_cb_t registration_cb,
                                               void const *p_callback_ctx);

/**
  * @brief  Initialization CellularApp datetime
  * @param  -
  * @retval -
  */
void cellular_app_datetime_init(void);

/**
  * @brief  Start CellularApp datetime
  * @param  -
  * @retval -
  */
void cellular_app_datetime_start(void);

#endif /* USE_RTC == 1 */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_APP_DATETIME_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
