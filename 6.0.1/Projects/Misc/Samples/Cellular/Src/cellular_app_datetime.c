/**
  ******************************************************************************
  * @file    cellular_app_datetime.c
  * @author  MCD Application Team
  * @brief   This file contains date and time management
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

/* module used only if USE_RTC activated */
#if (USE_RTC == 1)

#include <string.h>

#include "rtc.h"
#include "cellular_app_datetime.h"
#include "cellular_runtime_custom.h"
#include "cellular_runtime_standard.h"


/* Private macros ------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define CELLULAR_APP_DATETIME_DAY_LEN             3U
#define CELLULAR_APP_DATETIME_MONTH_LEN           3U

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static bool cellular_app_datetime_initialized;     /* false/true - datetime not initialized / datetime initialized */
static uint16_t cellular_app_datetime_year_start;  /* RTC is managing a year value between [0, 99]
                                                    * so when setting the date this value can be used
                                                    * in order to set RTC year to a value less than 99 */
/* To save the callback function to called when cellular_app_datetime_set() is called */
static cellular_app_datetime_set_registration_cb_t cellular_app_datetime_set_regitration_cb;
/* To save the callback context function to provide when cellular_app_datetime_set_regitration_cb() is called */
static void const *p_cellular_app_datetime_set_callback_ctx;

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#if 0 /* unused */
static uint16_t cellular_app_datetime_timedate_yearday_get(RTC_DateTypeDef const *p_rtc_date);
#endif /* unused */

/* Functions Definition ------------------------------------------------------*/
#if 0 /* unused */
/**
  * @brief  Get year day number
  * @param  p_rtc_date - RTC date
  * @retval uint16_t   - day of year
  */
static uint16_t cellular_app_datetime_timedate_yearday_get(RTC_DateTypeDef const *p_rtc_date)
{
  /* number of days by month */
  static const uint8_t month_day[12] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
  uint8_t  i = 0U;
  uint16_t result = 0U;
  uint16_t year;

  /* Add number of day for each month fully finished */
  /* Month January: 1 - December : 12 - month_day indice 0U to 11U */
  while (i < (p_rtc_date->Month - 1U))
  {
    result += month_day[i];
    i++;
  }

  /* Add number of day of the current month */
  result += p_rtc_date->Date;

  /* Add a day if it is a lead year after February */
  year = cellular_app_datetime_year_start + (uint16_t)p_rtc_date->Year;
  if ((p_rtc_date->Month > 2U) && (year % 4U) == 0U)
  {
    result++;
  }

  return (result);
}
#endif /* unused */

/**
  * @brief  Convert a date and time using a string to a cellular_app_datetime_t format
  * @param  datetime_str_len  - length of the data pointed by p_datetime_str
  * @param  p_datetime_str    - provided datetime string. Format is: Day, MonthDay Month Year Hour Minutes Seconds
  *         e.g: Mon 15 Nov 2021 13:50:10 - Day and Month must be the first 3 characters in English
  * @note   RTC last possible value is 99 for year do not forget to check year_start value
  * @param  p_datetime        - datetime in cellular_app_datetime_t format
  * @retval bool - false/true - conversion failed / conversion is ok
  */
bool cellular_app_datetime_str_convert(uint8_t datetime_str_len, uint8_t const *p_datetime_str,
                                       cellular_app_datetime_t *p_datetime)
{
  /* Day strings */
  static const uint8_t *day_string[7] =
  {
    (uint8_t *)"Mon", (uint8_t *)"Tue", (uint8_t *)"Wed", (uint8_t *)"Thu", (uint8_t *)"Fri", (uint8_t *)"Sat",
    (uint8_t *)"Sun"
  };
  /* Month strings */
  static const uint8_t *month_string[12] =
  {
    (uint8_t *)"Jan", (uint8_t *)"Feb", (uint8_t *)"Mar", (uint8_t *)"Apr", (uint8_t *)"May", (uint8_t *)"Jun",
    (uint8_t *)"Jul", (uint8_t *)"Aug", (uint8_t *)"Sep", (uint8_t *)"Oct", (uint8_t *)"Nov", (uint8_t *)"Dec"
  };
  /* RTC week day values */
  static const uint8_t day_value[7] =
  {
    RTC_WEEKDAY_MONDAY, RTC_WEEKDAY_TUESDAY, RTC_WEEKDAY_WEDNESDAY, RTC_WEEKDAY_THURSDAY, RTC_WEEKDAY_FRIDAY,
    RTC_WEEKDAY_SATURDAY, RTC_WEEKDAY_SUNDAY
  };

  bool result = false; /* value returned when there is no issue */
  uint8_t offset = 0U; /* current offset in string */
  uint8_t i;
  int32_t atoi_res;

  /*=====*/
  /* Day */
  /*=====*/
  for (i = 0U; i < 7U; i++)
  {
    if (memcmp((const void *)&p_datetime_str[offset], (const void *)day_string[i], CELLULAR_APP_DATETIME_DAY_LEN) == 0)
    {
      break;
    }
  }
  if (i < 7U)
  {
    /* Update the result */
    p_datetime->date.week_day = day_value[i];
    /* Continue the analysis */
    offset += CELLULAR_APP_DATETIME_DAY_LEN + 1U; /* 1U: to skip the space */
    if (offset < datetime_str_len)
    {
      result = true;
    }
  }

  /*===========*/
  /* Month day */
  /*===========*/
  /* Continue the treatment only if previous analysis was ok */
  if (result == true)
  {
    result = false;
    atoi_res = crs_atoi(&p_datetime_str[offset]);
    if ((atoi_res > 0) && (atoi_res < 32)) /* 1 to 31 */
    {
      /* Look at the end of month day in string */
      for (i = 0U ; i < 3U ; i++)
      {
        if (p_datetime_str[offset + i] == (uint8_t)' ')
        {
          break;
        }
      }
      if (i < 3U)
      {
        /* Update the result */
        p_datetime->date.month_day = (uint8_t)atoi_res;
        /* Continue the analysis */
        offset += i + 1U; /* 1U: to skip the space */
        if (offset < datetime_str_len)
        {
          result = true;
        }
      }
    }
  }

  /*=======*/
  /* Month */
  /*=======*/
  /* Continue the treatment only if previous analysis was ok */
  if (result == true)
  {
    result = false;
    for (i = 0U; i < 12U; i++)
    {
      if (memcmp((const void *)&p_datetime_str[offset], (const void *)month_string[i],
                 CELLULAR_APP_DATETIME_MONTH_LEN) == 0)
      {
        break;
      }
    }
    if (i < 12U)
    {
      /* Update the result */
      p_datetime->date.month = i + 1U;
      /* Continue the analysis */
      offset += CELLULAR_APP_DATETIME_MONTH_LEN + 1U; /* 1U: to skip the space */
      if (offset < datetime_str_len)
      {
        result = true;
      }
    }
  }

  /*======*/
  /* Year */
  /*======*/
  /* Continue the treatment only if previous analysis was ok */
  if (result == true)
  {
    result = false;
    atoi_res = crs_atoi(&p_datetime_str[offset]);
    if (atoi_res > 0)
    {
      /* Look at the end of year in string */
      for (i = 0U; i < 5U; i++)
      {
        if (p_datetime_str[offset + i] == (uint8_t)' ')
        {
          break;
        }
      }
      if (i < 5U)
      {
        /* Update the result */
        p_datetime->date.year = 0U;
        p_datetime->date.year_start = (uint16_t)atoi_res;
        /* Continue the analysis */
        offset += i + 1U;
        if (offset < datetime_str_len)
        {
          result = true;
        }
      }
    }
  }

  /*======*/
  /* Hour */
  /*======*/
  /* Continue the treatment only if previous analysis was ok */
  if (result == true)
  {
    result = false;
    atoi_res = crs_atoi(&p_datetime_str[offset]);
    if ((atoi_res >= 0) && (atoi_res < 24)) /* 0 to 23 */
    {
      /* Look at the end of month day in string */
      for (i = 0U; i < 3U; i++)
      {
        if (p_datetime_str[offset + i] == (uint8_t)':')
        {
          break;
        }
      }
      if (i < 3U)
      {
        /* Update the result */
        p_datetime->time.hour = (uint8_t)atoi_res;
        /* Continue the analysis */
        offset += i + 1U;
        if (offset < datetime_str_len)
        {
          result = true;
        }
      }
    }
  }

  /*=========*/
  /* Minutes */
  /*=========*/
  /* Continue the treatment only if previous analysis was ok */
  if (result == true)
  {
    result = false;
    atoi_res = crs_atoi(&p_datetime_str[offset]);
    if ((atoi_res >= 0) && (atoi_res < 60)) /* 0 to 59 */
    {
      /* Look at the end of month day in string */
      for (i = 0U; i < 3U; i++)
      {
        if (p_datetime_str[offset + i] == (uint8_t)':')
        {
          break;
        }
      }
      if (i < 3U)
      {
        /* Update the result */
        p_datetime->time.min = (uint8_t)atoi_res;
        /* Continue the analysis */
        offset += i + 1U;
        if (offset < datetime_str_len)
        {
          result = true;
        }
      }
    }
  }

  /*=========*/
  /* Seconds */
  /*=========*/
  /* Continue the treatment only if previous analysis was ok */
  if (result == true)
  {
    result = false;
    atoi_res = crs_atoi(&p_datetime_str[offset]);
    if ((atoi_res >= 0) && (atoi_res < 60)) /* 0 to 59 */
    {
      /* Update the result */
      p_datetime->time.sec = (uint8_t)atoi_res;
      result = true;
    }
  }

  return (result);
}

/**
  * @brief  Set date and time
  * @param  p_datetime        - provided datetime
  * @note   RTC last possible value is 99 for year do not forget to use year_start in p_datetime
  * @retval bool - false/true - update not done / update done
  */
bool cellular_app_datetime_set(cellular_app_datetime_t *p_datetime)
{
  bool result = false;
  RTC_TimeTypeDef rtc_time;
  RTC_DateTypeDef rtc_date;

  /* Parameters are checked by HAL RTC */
  /* Set RTC Time */
  rtc_time.Hours          = p_datetime->time.hour;
  rtc_time.Minutes        = p_datetime->time.min;
  rtc_time.Seconds        = p_datetime->time.sec;
  rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  rtc_time.StoreOperation = RTC_STOREOPERATION_RESET;
  rtc_time.TimeFormat     = 0x00U; /* Format AM/PM not managed */
  /* Set RTC time */
  if (HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN) == HAL_OK)
  {
    /* Set RTC date */
    rtc_date.WeekDay = p_datetime->date.week_day;
    rtc_date.Date    = p_datetime->date.month_day;
    rtc_date.Month   = p_datetime->date.month;
    rtc_date.Year    = p_datetime->date.year;
    if (HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN) == HAL_OK)
    {
      result = true;
      cellular_app_datetime_initialized = true;
      cellular_app_datetime_year_start = p_datetime->date.year_start;
      if (cellular_app_datetime_set_regitration_cb != NULL)
      {
        cellular_app_datetime_set_regitration_cb(p_cellular_app_datetime_set_callback_ctx);
      }
    }
  }

  return (result);
}

/**
  * @brief  Get date and time
  * @param  p_datetime        - datetime pointer
  * @note   add p_datetime->date.year_start to p_datetime->date.year to have a year formatted 4 digits
  * @retval bool - false/true - datetime not yet set / datetime set
  */
bool cellular_app_datetime_get(cellular_app_datetime_t *p_datetime)
{
  bool result;
  RTC_TimeTypeDef rtc_time;
  RTC_DateTypeDef rtc_date;

  /* WARNING : if HAL_RTC_GetTime is called it must be called before HAL_RTC_GetDate */
  /* Get RTC Time */
  /* HAL_RTC_GetTime return always HAL_OK so cast (void) to avoid warning useless test */
  (void)HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
  /* Get the RTC current Time */
  p_datetime->time.hour  = rtc_time.Hours;
  p_datetime->time.min   = rtc_time.Minutes;
  p_datetime->time.sec   = rtc_time.Seconds;

  /* WARNING : HAL_RTC_GetDate must be called after HAL_RTC_GetTime even if date get is not necessary */
  /* Get RTC Date */
  /* HAL_RTC_GetDate return always HAL_OK so cast (void) to avoid warning useless test */
  (void)HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
  p_datetime->date.year      = rtc_date.Year;
  p_datetime->date.month     = rtc_date.Month;
  p_datetime->date.month_day = rtc_date.Date;
  p_datetime->date.week_day  = rtc_date.WeekDay;

  p_datetime->date.year_start = cellular_app_datetime_year_start;
  result = cellular_app_datetime_initialized;

  return (result);
}

/**
  * @brief  Date and time callback registration.
  * @note   only one callback can be registered
  * @param  registration_cb - callback to register
  * @param  p_callback_ctx  - context to be passed when cellular_app_datetime_set_registration_cb() is called.
  * @retval -
  */
void cellular_app_datetime_set_cb_registration(cellular_app_datetime_set_registration_cb_t registration_cb,
                                               void const *p_callback_ctx)
{
  if (registration_cb != NULL)
  {
    cellular_app_datetime_set_regitration_cb = registration_cb;
    p_cellular_app_datetime_set_callback_ctx = p_callback_ctx;
  }
}

/**
  * @brief  Initialization CellularApp datetime
  * @param  -
  * @retval -
  */
void cellular_app_datetime_init(void)
{
  cellular_app_datetime_initialized = false;       /* datetime is not yet set                      */
  cellular_app_datetime_year_start = 0U;           /* datetime year start is not yet set           */
  cellular_app_datetime_set_regitration_cb = NULL; /* datetime set callback is not yet set         */
  p_cellular_app_datetime_set_callback_ctx = NULL; /* datetime set callback context is not yet set */
}

/**
  * @brief  Start CellularApp datetime
  * @param  -
  * @retval -
  */
void cellular_app_datetime_start(void)
{
  __NOP();
}

#endif /* (USE_RTC == 1) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
