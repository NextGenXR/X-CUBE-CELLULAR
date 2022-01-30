/**
  ******************************************************************************
  * @file    cellular_app_uiclient.c
  * @author  MCD Application Team
  * @brief   UIClt Cellular Application :
  *          - Manage UI (display)
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

#if (USE_DISPLAY == 1)
#include <string.h>
#include <stdbool.h>

#if (USE_SENSORS ==1)
#include <math.h>
#endif /* USE_SENSORS ==1 */

#include "cellular_app_uiclient.h"

#include "cellular_app.h"
#include "cellular_app_trace.h"

#if (USE_RTC == 1)
#include "cellular_app_datetime.h"
#endif /* USE_RTC == 1 */

#include "cellular_app_display.h"
#if defined(APPLICATION_IMAGES_FILE)
#include APPLICATION_IMAGES_FILE
#endif /* defined(APPLICATION_IMAGES_FILE) */

#if (USE_SENSORS == 1)
#include "cellular_app_sensors.h"
#endif /* USE_SENSORS == 1 */

#include "cellular_control_api.h"

#include "rtosal.h"

/* Private typedef -----------------------------------------------------------*/
#if ((USE_RTC == 1) || (USE_SENSORS == 1))
/* Timer State */
typedef uint8_t cellular_app_uiclient_timer_state_t;
#define CELLULAR_APP_UICLIENT_TIMER_INVALID       (cellular_app_uiclient_timer_state_t)0
#define CELLULAR_APP_UICLIENT_TIMER_IDLE          (cellular_app_uiclient_timer_state_t)1
#define CELLULAR_APP_UICLIENT_TIMER_RUN           (cellular_app_uiclient_timer_state_t)2
#endif /* USE_SENSORS == 1 */

/* Private defines -----------------------------------------------------------*/
typedef uint8_t uiclient_screen_state_t;
#define UICLIENT_SCREEN_OFF                       (uiclient_screen_state_t)0
#define UICLIENT_SCREEN_WELCOME                   (uiclient_screen_state_t)1
#define UICLIENT_SCREEN_CELLULAR_INFO             (uiclient_screen_state_t)2
#define UICLIENT_SCREEN_DATETIME_INFO             (uiclient_screen_state_t)3
#if (USE_SENSORS == 1)
#define UICLIENT_SCREEN_SENSORS_INFO              (uiclient_screen_state_t)4
#endif /* USE_SENSORS == 1 */

#define UICLIENT_STRING_LENGTH_MAX                (uint8_t)40
#define UICLIENT_STRING_TMP_LENGTH_MAX            (uint8_t)20
#define UICLIENT_STRING_SPACE_LENGTH_MAX          (uint8_t)10

#define UICLIENT_CELLULAR_MSG                      ((CELLULAR_APP_VALUE_MAX_MSG) + (cellular_app_msg_type_t)1)
#define UICLIENT_TIMER_MSG                         ((CELLULAR_APP_VALUE_MAX_MSG) + (cellular_app_msg_type_t)2)
/* MSG id when MSG is UICLIENT_CELLULAR_MSG */
/* MSG id is Celular info                   */
#define UICLIENT_CELLULAR_INFO_CHANGE_ID           ((CELLULAR_APP_VALUE_MAX_ID) + (cellular_app_msg_id_t)1)
/* MSG id is DateTime info                  */
#define UICLIENT_CELLULAR_DATETIME_CHANGE_ID       ((CELLULAR_APP_VALUE_MAX_ID) + (cellular_app_msg_id_t)3)
/* MSG id when MSG is UICLIENT_TIMER_MSG    */
#if (USE_RTC == 1)
/* MSG id is DateTime increase 1s.          */
#define UICLIENT_DATETIME_READ_ID                  ((CELLULAR_APP_VALUE_MAX_ID) + (cellular_app_msg_id_t)4)
#endif /* USE_RTC == 1 */
#if (USE_SENSORS == 1)
/* MSG id is Sensors read                   */
#define UICLIENT_SENSORS_READ_ID                   ((CELLULAR_APP_VALUE_MAX_ID) + (cellular_app_msg_id_t)5)
#endif /* USE_SENSORS == 1 */

#if (USE_RTC == 1)
#define UICLIENT_DATETIME_READ_TIMER               (uint32_t)60000 /* Unit: in ms. - read every minute */
#endif /* USE_RTC == 1 */
#if (USE_SENSORS == 1)
#define UICLIENT_SENSORS_READ_TIMER                (uint32_t)5000 /* Unit: in ms. */
#endif /* USE_SENSORS == 1 */

/* Private variables ---------------------------------------------------------*/
/* Trace shortcut */
static const uint8_t *p_cellular_app_uiclient_trace;

/* UIClt application descriptor */
static cellular_app_desc_t     cellular_app_uiclient;
/* UIClt screen state           */
static uiclient_screen_state_t cellular_app_uiclient_screen_state;

#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
static bool cellular_app_uiclient_modem_is_on;
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
static bool cellular_app_uiclient_display_init_ok;

static uint8_t cellular_app_uiclient_string[UICLIENT_STRING_LENGTH_MAX];
static uint8_t cellular_app_uiclient_string_tmp[UICLIENT_STRING_TMP_LENGTH_MAX];
static uint8_t cellular_app_uiclient_string_space[UICLIENT_STRING_SPACE_LENGTH_MAX];

#if (USE_RTC == 1)
/* Timer to read periodically the DateTime */
static osTimerId cellular_app_uiclient_datetime_timer_id;
static cellular_app_uiclient_timer_state_t cellular_app_uiclient_datetime_timer_state;
#endif /* USE_RTC == 1 */

#if (USE_SENSORS == 1)
/* Timer to read periodically the Sensors */
static osTimerId cellular_app_uiclient_sensors_timer_id;
static cellular_app_uiclient_timer_state_t cellular_app_uiclient_sensors_timer_state;
#endif /* USE_SENSORS == 1 */

/* Private macro -------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
/* Callback for CellularInfo status */
static void cellular_app_uiclient_cellular_info_status_cb(ca_event_type_t event_type,
                                                          const cellular_info_t *const p_cellular_info,
                                                          void *const p_callback_ctx);

/* Callback for SignalInfo status */
static void cellular_app_uiclient_signal_info_status_cb(ca_event_type_t event_type,
                                                        const cellular_signal_info_t *const p_sigal_info,
                                                        void *const p_callback_ctx);

#if (USE_RTC == 1)
/* Callback called when DateTime set */
static void cellular_app_uiclient_datetime_set_cb(void const *p_argument);
/* Callback called when DateTime Timer is raised */
static void cellular_app_uiclient_datetime_timer_cb(void *p_argument);
#endif /* USE_RTC == 1*/

#if (USE_SENSORS == 1)
/* Callback called when Sensors Timer is raised */
static void cellular_app_uiclient_sensors_timer_cb(void *p_argument);
#endif /* USE_SENSORS == 1 */

static void uiclient_format_line(uint8_t *p_string1, uint8_t length1, uint8_t *p_string2, uint8_t length2,
                                 uint8_t *p_string_res);
static bool uiclient_update_welcome(void);
static bool uiclient_update_cellular_info(void);
static bool uiclient_update_cellular_ip_info(void);

#if (USE_RTC == 1)
static bool uiclient_update_datetime_info(void);
#endif /* USE_RTC == 1 */

#if (USE_SENSORS == 1)
static bool uiclient_update_sensors_info(void);
#endif /* USE_SENSORS == 1 */

static void uiclient_thread(void *p_argument);

/* Public  functions  prototypes ---------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Callback called when a value in cellular changed
  * @param  event_type      - Event that happened: CA_CELLULAR_INFO_EVENT.
  * @param  p_cellular_info - The new cellular information.
  * @param  p_callback_ctx  - The p_callback_ctx parameter in cellular_info_changed_registration function.
  * @retval -
  */
static void cellular_app_uiclient_cellular_info_status_cb(ca_event_type_t event_type,
                                                          const cellular_info_t *const p_cellular_info,
                                                          void *const p_callback_ctx)
{
  UNUSED(p_callback_ctx);
  uint32_t msg_queue = 0U;
  rtosalStatus status;

  /* Event to know Modem state ? */
  if ((event_type == CA_CELLULAR_INFO_EVENT) && (p_cellular_info != NULL))
  {
    SET_CELLULAR_APP_MSG_TYPE(msg_queue, UICLIENT_CELLULAR_MSG);
    SET_CELLULAR_APP_MSG_ID(msg_queue, UICLIENT_CELLULAR_INFO_CHANGE_ID);
    /* Send the message */
    status = rtosalMessageQueuePut(cellular_app_uiclient.queue_id, msg_queue, 0U);
    if (status != osOK)
    {
      PRINT_FORCE("%s: ERROR CellularInfo Msg Put Type:%d Id:%d - status:%d!", p_cellular_app_uiclient_trace,
                  GET_CELLULAR_APP_MSG_TYPE(msg_queue), GET_CELLULAR_APP_MSG_ID(msg_queue), status)
    }
  }
}

/**
  * @brief  Callback called when a value in signal changed
  * @param  event_type     - Event that happened: CA_CELLULAR_SIGNAL_INFO_EVENT.
  * @param  p_signal_info  - The new signal information.
  * @param  p_callback_ctx - The p_callback_ctx parameter in cellular_signal_info_changed_registration function.
  * @retval -
  */
static void cellular_app_uiclient_signal_info_status_cb(ca_event_type_t event_type,
                                                        const cellular_signal_info_t *const p_signal_info,
                                                        void *const p_callback_ctx)
{
  UNUSED(p_callback_ctx);
  uint32_t msg_queue = 0U;
  rtosalStatus status;

  /* Event to know Modem state ? */
  if ((event_type == CA_SIGNAL_INFO_EVENT) && (p_signal_info != NULL))
  {
    SET_CELLULAR_APP_MSG_TYPE(msg_queue, UICLIENT_CELLULAR_MSG);
    SET_CELLULAR_APP_MSG_ID(msg_queue, UICLIENT_CELLULAR_INFO_CHANGE_ID);
    /* Send the message */
    status = rtosalMessageQueuePut(cellular_app_uiclient.queue_id, msg_queue, 0U);
    if (status != osOK)
    {
      PRINT_FORCE("%s: ERROR SignalInfo Msg Put Type:%d Id:%d - status:%d!", p_cellular_app_uiclient_trace,
                  GET_CELLULAR_APP_MSG_TYPE(msg_queue), GET_CELLULAR_APP_MSG_ID(msg_queue), status)
    }
  }
}

#if (USE_RTC == 1)
/**
  * @brief  Callback called when DateTime is set
  * @param  p_argument - UNUSED
  * @retval -
  */
static void cellular_app_uiclient_datetime_set_cb(void const *p_argument)
{
  UNUSED(p_argument);
  uint32_t msg_queue = 0U;
  rtosalStatus status;

  SET_CELLULAR_APP_MSG_TYPE(msg_queue, UICLIENT_CELLULAR_MSG);
  SET_CELLULAR_APP_MSG_ID(msg_queue, UICLIENT_CELLULAR_DATETIME_CHANGE_ID);

  status = rtosalMessageQueuePut(cellular_app_uiclient.queue_id, msg_queue, 0U);
  if (status != osOK)
  {
    PRINT_FORCE("%s: ERROR DateTime Msg Put Type:%d Id:%d - status:%d!", p_cellular_app_uiclient_trace,
                GET_CELLULAR_APP_MSG_TYPE(msg_queue), GET_CELLULAR_APP_MSG_ID(msg_queue), status)
  }
}

/**
  * @brief  Callback called when DateTime Timer raised
  * @param  p_argument - UNUSED
  * @retval -
  */
static void cellular_app_uiclient_datetime_timer_cb(void *p_argument)
{
  UNUSED(p_argument);
  uint32_t msg_queue = 0U;
  rtosalStatus status;

  if (cellular_app_uiclient_datetime_timer_state == CELLULAR_APP_UICLIENT_TIMER_RUN)
  {
    SET_CELLULAR_APP_MSG_TYPE(msg_queue, UICLIENT_TIMER_MSG);
    SET_CELLULAR_APP_MSG_ID(msg_queue, UICLIENT_DATETIME_READ_ID);

    /* A message has to be send */
    status = rtosalMessageQueuePut(cellular_app_uiclient.queue_id, msg_queue, 0U);
    if (status != osOK)
    {
      PRINT_FORCE("%s: ERROR DateTime Msg Put Type:%d Id:%d - status:%d!r", p_cellular_app_uiclient_trace,
                  GET_CELLULAR_APP_MSG_TYPE(msg_queue), GET_CELLULAR_APP_MSG_ID(msg_queue), status)
    }
  }
}
#endif /* USE_RTC == 1 */

#if (USE_SENSORS == 1)
/**
  * @brief  Callback called when Sensors Timer raised
  * @param  p_argument - UNUSED
  * @retval -
  */
static void cellular_app_uiclient_sensors_timer_cb(void *p_argument)
{
  UNUSED(p_argument);
  uint32_t msg_queue = 0U;
  rtosalStatus status;

  if (cellular_app_uiclient_sensors_timer_state == CELLULAR_APP_UICLIENT_TIMER_RUN)
  {
    SET_CELLULAR_APP_MSG_TYPE(msg_queue, UICLIENT_TIMER_MSG);
    SET_CELLULAR_APP_MSG_ID(msg_queue, UICLIENT_SENSORS_READ_ID);

    /* A message has to be send */
    status = rtosalMessageQueuePut(cellular_app_uiclient.queue_id, msg_queue, 0U);
    if (status != osOK)
    {
      PRINT_FORCE("%s: ERROR Sensors Msg Put Type:%d Id:%d - status:%d!", p_cellular_app_uiclient_trace,
                  GET_CELLULAR_APP_MSG_TYPE(msg_queue), GET_CELLULAR_APP_MSG_ID(msg_queue), status)
    }
  }
}
#endif /* USE_SENSORS == 1 */

/**
  * @brief  Format a line taking into account number of characters available on a line
  *         Enough space will be added between string1 and string2 to have string2 end always at last possible position
  * @param  p_string1    - string to display at left
  * @param  length1      - string1 length
  * @param  p_string2    - string to display at right
  * @param  length2      - string2 length
  * @param  p_string_res - string result to display
  * @retval -
  */
static void uiclient_format_line(uint8_t *p_string1, uint8_t length1, uint8_t *p_string2, uint8_t length2,
                                 uint8_t *p_string_res)
{
  uint32_t nb_character = cellular_app_display_characters_per_line();
  if ((uint32_t)(length1 + length2) < nb_character)
  {
    (void)memset(cellular_app_uiclient_string_space, 0x20, UICLIENT_STRING_SPACE_LENGTH_MAX);
    (void)sprintf((CRC_CHAR_t *)p_string_res, "%.*s%.*s%.*s", (int16_t)length1, p_string1,
                  (int16_t)(nb_character - (uint32_t)(length1 + length2)), cellular_app_uiclient_string_space,
                  (int16_t)length2, p_string2);
  }
  else
  {
    (void)sprintf((CRC_CHAR_t *)p_string_res, "%.*s %.*s", (int16_t)length1, p_string1,
                  (int16_t)CELLULAR_APP_MIN((nb_character - (uint32_t)(length1 + 1U)), (uint32_t)length2), p_string2);
  }
}

/**
  * @brief  Update display and status according to welcome screen
  * @retval true/false - refresh has to be done or not
  */
static bool uiclient_update_welcome(void)
{
  bool result = false;

  /* Display welcome screen only if UI comes from screen off */
  if (cellular_app_uiclient_screen_state == UICLIENT_SCREEN_OFF)
  {
    /* Board display init to be done only if modem is on ? */
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
    if (cellular_app_uiclient_modem_is_on == true)
    {
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
      cellular_app_uiclient_display_init_ok = cellular_app_display_init();
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
    }
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */

    if (cellular_app_uiclient_display_init_ok == true)
    {
      /* Display a welcome image if defined */
#if defined(APPLICATION_IMAGES_FILE)
#if defined(CELLULAR_APP_BMP_WELCOME_SIZE)
      uint32_t XPos = 0U;
      uint32_t YPos = 0U;
      if (CELLULAR_APP_BMP_WELCOME_HEIGHT < cellular_app_display_get_YSize())
      {
        YPos = (cellular_app_display_get_YSize() - CELLULAR_APP_BMP_WELCOME_HEIGHT) / 2U;
      }
      if (CELLULAR_APP_BMP_WELCOME_WIDTH < cellular_app_display_get_XSize())
      {
        XPos = (cellular_app_display_get_XSize() - CELLULAR_APP_BMP_WELCOME_WIDTH) / 2U;
      }
      cellular_app_display_set_BackColor(LCD_COLOR_WHITE);
      cellular_app_display_draw_Bitmap((uint16_t)XPos, (uint16_t)YPos, (uint8_t *)cellular_app_bmp_welcome);
      /* Update screen state */
#else
      cellular_app_display_set_BackColor(LCD_COLOR_BLACK);
#endif /* defined(CELLULAR_APP_BMP_WELCOME_SIZE) */
#endif /* defined(APPLICATION_IMAGES_FILE) */
      /* Update screen state */
      cellular_app_uiclient_screen_state = UICLIENT_SCREEN_WELCOME;
      result = true;
    }
  }

  return (result);
}

/**
  * @brief  Update display and status according to new cellular info received
  * @retval bool         - false/true - refresh has not to do/refresh has to be done
  */
static bool uiclient_update_cellular_info(void)
{
  bool result = false;
  uint8_t  operator_name_start;
  uint32_t line = 0U;
  uint32_t operator_name_length;
  uint8_t  *p_string1;
  uint8_t  *p_string2;
  static uint8_t *sim_string[3]      = {((uint8_t *)"SIM:?  "), \
                                        ((uint8_t *)"SIMCard"), \
                                        ((uint8_t *)"SIMSold")
                                       };
  static uint8_t *state_string[10]   = {((uint8_t *)"   Unknown"), \
                                        ((uint8_t *)"      Init"), \
                                        ((uint8_t *)" SimConctd"), \
                                        ((uint8_t *)" NwkSearch"), \
                                        ((uint8_t *)" NwkAttach"), \
                                        ((uint8_t *)" DataReady"), \
                                        ((uint8_t *)"FlightMode"), \
                                        ((uint8_t *)"    Reboot"), \
                                        ((uint8_t *)"  Updating"), \
                                        ((uint8_t *)"  ModemOff")
                                       };
  static uint8_t *operator_string[1] = {((uint8_t *)"Operator:?")};

  cellular_info_t cellular_info;
  /* Read Cellular information to know modem state */
  cellular_get_cellular_info(&cellular_info);

  /* Cellular data display only information */
  if (cellular_app_uiclient_display_init_ok == false)
  {
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
    /* Must wait modem power on information before to initialize the display */
    if (cellular_info.modem_state != CA_MODEM_POWER_OFF)
    {
      cellular_app_uiclient_modem_is_on = true;
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
      cellular_app_uiclient_display_init_ok = cellular_app_display_init();
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
    }
    else
    {
      cellular_app_uiclient_display_init_ok = false;
      cellular_app_uiclient_modem_is_on = false;
    }
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
  }

  if (cellular_app_uiclient_display_init_ok == true)
  {
    /* Display welcome screen ? */
    if (cellular_app_uiclient_screen_state == UICLIENT_SCREEN_OFF)
    {
      /* Display a welcome bitmap */
      result = uiclient_update_welcome();
    }
    else
    {
      /* If comes from Screen Welcome - Erase full screen */
      if (cellular_app_uiclient_screen_state == UICLIENT_SCREEN_WELCOME)
      {
        /* Exit of Welcome screen if something about modem has to be displayed */
        if ((cellular_info.modem_state != CA_MODEM_POWER_OFF) && (cellular_info.identity.manufacturer_id.len != 0U))
        {
          cellular_app_display_set_BackColor(LCD_COLOR_BLACK);
          cellular_app_display_set_TextColor(LCD_COLOR_WHITE);
          cellular_app_display_clear(LCD_COLOR_BLACK);
          /* And go to Screen Cellular Info */
          cellular_app_uiclient_screen_state = UICLIENT_SCREEN_CELLULAR_INFO;
#if (USE_SENSORS == 1)
          /* Start periodical Sensors Timer */
          if (cellular_app_uiclient_sensors_timer_state == CELLULAR_APP_UICLIENT_TIMER_IDLE)
          {
            if (rtosalTimerStart(cellular_app_uiclient_sensors_timer_id, UICLIENT_SENSORS_READ_TIMER) == osOK)
            {
              cellular_app_uiclient_sensors_timer_state = CELLULAR_APP_UICLIENT_TIMER_RUN;
            }
          }
#endif /* USE_SENSORS == 1 */
        }
      }

      /* Update screen only if screen is Cellular Info */
      if (cellular_app_uiclient_screen_state == UICLIENT_SCREEN_CELLULAR_INFO)
      {
        cellular_sim_info_t sim_info;
        cellular_signal_info_t signal_info;
        uint32_t nb_character;

        /* Set font to default font */
        cellular_app_display_font_set(0U);
        /* Get the maximum number of characters per line */
        nb_character = cellular_app_display_characters_per_line();

        (void)memset(cellular_app_uiclient_string, 0, UICLIENT_STRING_LENGTH_MAX);
        (void)memset(cellular_app_uiclient_string_space, 0x20, UICLIENT_STRING_SPACE_LENGTH_MAX);

#if (USE_RTC == 1)
        (void)uiclient_update_datetime_info();
#endif /* USE_RTC == 1 */
        /* Even if date time is not displayed, go to next line */
        line += cellular_app_display_font_get_height();
        /* Read SIM information */
        cellular_get_sim_info(&sim_info);

        /* line:[eSIM|SIM |    ] */
        if (sim_info.sim_status[sim_info.sim_index] == CA_SIM_READY)
        {
          /* ToDo: Make the difference between SIM and eSIM */
          switch (sim_info.sim_slot_type[sim_info.sim_index])
          {
            case CA_SIM_REMOVABLE_SLOT :
              p_string1 = sim_string[1];
              break;
            case CA_SIM_EXTERNAL_MODEM_SLOT :
            case CA_SIM_INTERNAL_MODEM_SLOT :
              p_string1 = sim_string[2];
              break;
            default :
              p_string1 = sim_string[0];
              break;
          }
        }
        else
        {
          p_string1 = sim_string[0];
        }

        /* line: Stat:$status */
        switch (cellular_info.modem_state)
        {
          case CA_MODEM_STATE_POWERED_ON:
            p_string2 = state_string[1];
            break;
          case CA_MODEM_STATE_SIM_CONNECTED:
            p_string2 = state_string[2];
            break;
          case CA_MODEM_NETWORK_SEARCHING:
            p_string2 = state_string[3];
            break;
          case CA_MODEM_NETWORK_REGISTERED:
            p_string2 = state_string[4];
            break;
          case CA_MODEM_STATE_DATAREADY:
            p_string2 = state_string[5];
            break;
          case CA_MODEM_IN_FLIGHTMODE:
            p_string2 = state_string[6];
            break;
          case CA_MODEM_REBOOTING:
            p_string2 = state_string[7];
            break;
          case CA_MODEM_FOTA_INPROGRESS:
            p_string2 = state_string[8];
            break;
          case CA_MODEM_POWER_OFF:
            p_string2 = state_string[9];
            break;
          default:
            p_string2 = state_string[0];
            PRINT_INFO("%s: Modem state Unknown: %d!!!", p_cellular_app_uiclient_trace, cellular_info.modem_state)
            break;
        }

        uiclient_format_line(p_string1, 7U, p_string2, 10U, (uint8_t *)&cellular_app_uiclient_string);
        cellular_app_display_string(1U, (uint16_t)line, cellular_app_uiclient_string);
        line += cellular_app_display_font_get_height();

        /* Read Cellular signal information */
        cellular_get_signal_info(&signal_info);

        /* line: $mno_name $cs_signal_level_db(dB) */
        operator_name_length = cellular_info.mno_name.len;
        if (cellular_info.mno_name.value[0] == (uint8_t)'"')
        {
          operator_name_start = 1U;
          operator_name_length = operator_name_length - 2U; /* by-pass '"' at string begin and end */
        }
        else
        {
          operator_name_start = 0U;
        }
        if (operator_name_length > 0U)
        {
          /* Point on mobile network mobile operator name */
          p_string1 = &cellular_info.mno_name.value[operator_name_start];
        }
        else
        {
          /* Point on mobile network mobile operator unknown*/
          p_string1 = operator_string[0];
          operator_name_length = 10U;
        }
        /* Add Signal level */
        (void)sprintf((CRC_CHAR_t *)&cellular_app_uiclient_string_tmp[0], "%3lddB",
                      signal_info.signal_strength.db_value);
        p_string2 = &cellular_app_uiclient_string_tmp[0];
        uiclient_format_line(p_string1, (uint8_t)CELLULAR_APP_MIN(operator_name_length, (nb_character - 5U - 1U)),
                             p_string2, 5U, (uint8_t *)&cellular_app_uiclient_string);

        cellular_app_display_string(1U, (uint16_t)line, cellular_app_uiclient_string);

        /* Finalize the cellular info screen with cellular ip info */
        (void)uiclient_update_cellular_ip_info();
#if (USE_SENSORS == 1)
        /* Finalize the cellular info screen with sensors info */
        /* line += cellular_app_display_font_get_height(); */
        (void)uiclient_update_sensors_info();
#endif /* USE_SENSORS == 1 */
        result = true;
      }
    }
  }

  return (result);
}

/**
  * @brief  Update display and status according to new cellular ip info received
  * @retval bool         - false/true - refresh has not to be done/refresh has to be done
  */
static bool uiclient_update_cellular_ip_info(void)
{
  bool result = false;
  cellular_info_t cellular_info;
  uint16_t line;
  uint8_t *p_string1;
  uint8_t *p_string2;
  static uint8_t *ip_string[2] = {((uint8_t *)"IP:?"), ((uint8_t *)"IP:")};

  /* Update screen only if display is initialized and screen is Cellular info */
  if ((cellular_app_uiclient_display_init_ok == true)
      && (cellular_app_uiclient_screen_state == UICLIENT_SCREEN_CELLULAR_INFO))
  {
    /* Read Cellular information to know IP address */
    cellular_get_cellular_info(&cellular_info);

    /* Is IP known ? */
    if (cellular_info.ip_addr.addr != 0U)
    {
      p_string1 = ip_string[1];
      (void)sprintf((CRC_CHAR_t *)&cellular_app_uiclient_string_tmp[0], "%hhu.%hhu.%hhu.%.hhu",
                    (uint8_t)(COM_IP4_ADDR1(&cellular_info.ip_addr)), (uint8_t)(COM_IP4_ADDR2(&cellular_info.ip_addr)),
                    (uint8_t)(COM_IP4_ADDR3(&cellular_info.ip_addr)), (uint8_t)(COM_IP4_ADDR4(&cellular_info.ip_addr)));
      p_string2 = &cellular_app_uiclient_string_tmp[0];
      uiclient_format_line(p_string1, 3U, p_string2, (uint8_t)crs_strlen((const uint8_t *)p_string2),
                           (uint8_t *)&cellular_app_uiclient_string);
    }
    else
    {
      (void)memcpy(cellular_app_uiclient_string, ip_string[0], 5);
    }
    /* line: Ip:$local_ip */
    line = 3U * (uint16_t)cellular_app_display_font_get_height();
    cellular_app_display_string(1U, line, cellular_app_uiclient_string);

    result = true;
  }

  return (result);
}

#if (USE_RTC == 1)
/**
  * @brief  Update display and status according to new date time info received
  * @retval bool - false/true - refresh has not to be done/refresh has to be done
  */
static bool uiclient_update_datetime_info(void)
{
  bool result = false;
  uint32_t nb_character;
  cellular_app_datetime_t datetime;

  /* Update screen only if display is initialized and screen is Cellular info */
  if ((cellular_app_uiclient_display_init_ok == true)
      && (cellular_app_uiclient_screen_state == UICLIENT_SCREEN_CELLULAR_INFO))
  {
    /* Set font to default font */
    cellular_app_display_font_set(0U);
    /* Get the maximum number of characters per line */
    nb_character = cellular_app_display_characters_per_line();

    /* line: hh:mm year/month/day */
    if (cellular_app_datetime_get(&datetime) == true)
    {
      (void)sprintf((CRC_CHAR_t *)cellular_app_uiclient_string, "%02u:%02u %.*s%04u/%02u/%02u",
                    datetime.time.hour, datetime.time.min,
                    (int16_t)(nb_character - 16U), cellular_app_uiclient_string_space,
                    ((uint16_t)datetime.date.year + datetime.date.year_start),
                    datetime.date.month, datetime.date.month_day);
    }
    else
    {
      (void)sprintf((CRC_CHAR_t *)cellular_app_uiclient_string, "--:-- %.*s----/--/--",
                    (int16_t)(nb_character - 16U), cellular_app_uiclient_string_space);

    }
    cellular_app_display_string(1U, 0U, cellular_app_uiclient_string);

    result = true;
  }

  return (result);
}
#endif /* USE_RTC == 1 */

#if (USE_SENSORS ==1)
/**
  * @brief  Update display and status according to new sensors info read
  * @retval bool         - false/true - refresh has not to be done/refresh has to be done
  */
static bool uiclient_update_sensors_info(void)
{
  bool result = false;

  uint16_t line;
  cellular_app_sensors_data_t sensor_humidity;
  cellular_app_sensors_data_t sensor_pressure;
  cellular_app_sensors_data_t sensor_temperature;

  /* Update screen only if display is initialized and screen is Cellular info */
  if ((cellular_app_uiclient_display_init_ok == true)
      && (cellular_app_uiclient_screen_state == UICLIENT_SCREEN_CELLULAR_INFO))
  {
    if (cellular_app_sensors_read(CELLULAR_APP_SENSOR_TYPE_HUMIDITY, &sensor_humidity) == false)
    {
      sensor_humidity.float_data = (float_t)0;
    }
    if (cellular_app_sensors_read(CELLULAR_APP_SENSOR_TYPE_PRESSURE, &sensor_pressure) == false)
    {
      sensor_pressure.float_data = (float_t)0;
    }
    if (cellular_app_sensors_read(CELLULAR_APP_SENSOR_TYPE_TEMPERATURE, &sensor_temperature) == false)
    {
      sensor_temperature.float_data = (float_t)0;
    }

    (void)sprintf((CRC_CHAR_t *)cellular_app_uiclient_string, "T:%4.1fC H:%4.1f P:%6.1fP",
                  sensor_temperature.float_data, sensor_humidity.float_data, sensor_pressure.float_data);
    line = 4U * (uint16_t)cellular_app_display_font_get_height();
    /* Too many information to display on same line: reduce Font */
    cellular_app_display_font_decrease();
    cellular_app_display_string(1U, line, cellular_app_uiclient_string);
    /* Restore font to default font */
    cellular_app_display_font_set(0U);

    result = true;
  }

  return (result);
}
#endif /* USE_SENSORS == 1 */

/**
  * @brief  Update information according to new one received
  * @param  info - information received
  * @retval -
  */
static void uiclient_update_info(uiclient_screen_state_t info)
{
  bool refresh_to_do = false;

  switch (info)
  {
    case UICLIENT_SCREEN_WELCOME :
      refresh_to_do = uiclient_update_welcome();
      break;
    case UICLIENT_SCREEN_CELLULAR_INFO :
      refresh_to_do = uiclient_update_cellular_info();
      break;
    case UICLIENT_SCREEN_DATETIME_INFO :
#if (USE_RTC == 1)
      refresh_to_do = uiclient_update_datetime_info();
#endif /* USE_RTC == 1 */
      break;
#if (USE_SENSORS == 1)
    case UICLIENT_SCREEN_SENSORS_INFO :
      refresh_to_do = uiclient_update_sensors_info();
      break;
#endif /* USE_SENSORS == 1 */
    default:
      break;
  }

  if (refresh_to_do == true)
  {
    /* Refresh Display */
    cellular_app_display_refresh();
  }
}
/**
  * @brief  UIClt thread
  * @note   Infinite loop UIClt body
  * @param  p_argument - unused
  * @retval -
  */
static void uiclient_thread(void *p_argument)
{
  UNUSED(p_argument);
  uint32_t msg_queue;
  uint8_t msg_type;  /* Msg type received from the queue */
  uint8_t msg_id;    /* Msg id received from the queue   */

  /* Display Screen Welcome as soon as possible */
  uiclient_update_info(UICLIENT_SCREEN_WELCOME);

  for (;;)
  {
    msg_queue = 0U; /* Re-initialize msg_queue to impossible value */
    /* Wait a notification to do something */
    (void)rtosalMessageQueueGet(cellular_app_uiclient.queue_id, &msg_queue, RTOSAL_WAIT_FOREVER);
    /* Analyze message */
    if (msg_queue != 0U)
    {
      msg_type = GET_CELLULAR_APP_MSG_TYPE(msg_queue);
      msg_id   = GET_CELLULAR_APP_MSG_ID(msg_queue);

      switch (msg_type)
      {
        case UICLIENT_CELLULAR_MSG :
          if (msg_id == UICLIENT_CELLULAR_INFO_CHANGE_ID)
          {
            uiclient_update_info(UICLIENT_SCREEN_CELLULAR_INFO);
          }
          else if (msg_id == UICLIENT_CELLULAR_DATETIME_CHANGE_ID)
          {
            uiclient_update_info(UICLIENT_SCREEN_DATETIME_INFO);
            /* Start periodical DateTime Timer */
            if (cellular_app_uiclient_datetime_timer_state == CELLULAR_APP_UICLIENT_TIMER_IDLE)
            {
              if (rtosalTimerStart(cellular_app_uiclient_datetime_timer_id, UICLIENT_DATETIME_READ_TIMER) == osOK)
              {
                cellular_app_uiclient_datetime_timer_state = CELLULAR_APP_UICLIENT_TIMER_RUN;
              }
            }
          }
          else /* Should not happen */
          {
            __NOP();
          }
          break;

        case UICLIENT_TIMER_MSG :
#if (USE_RTC == 1)
          if (msg_id == UICLIENT_DATETIME_READ_ID)
          {
            uiclient_update_info(UICLIENT_SCREEN_DATETIME_INFO);
          }
#endif /* USE_RTC == 1 */
#if (USE_SENSORS == 1)
          if (msg_id == UICLIENT_SENSORS_READ_ID)
          {
            uiclient_update_info(UICLIENT_SCREEN_SENSORS_INFO);
          }
#endif /* USE_SENSORS == 1 */
          break;

        default : /* Should not happen */
          __NOP();
          break;
      }
    }
  }
}

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Initialize all needed structures to support UIClt feature
  * @param  -
  * @retval -
  */
void cellular_app_uiclient_init(void)
{
  /* Initialize trace shortcut */
  p_cellular_app_uiclient_trace = cellular_app_type_string[CELLULAR_APP_TYPE_UICLIENT];

  /* Application Initialization */
  cellular_app_uiclient.app_id = 0U;
  /* Process Status Initialization */
  cellular_app_uiclient.process_status = true;
  /* Process Period Initialization */
  cellular_app_uiclient.process_period = 0U; /* Unused */
  /* Thread Id Initialization */
  cellular_app_uiclient.thread_id = NULL;
  /* Queue Id Initialization/Creation */
  cellular_app_uiclient.queue_id = rtosalMessageQueueNew(NULL, CELLULAR_APP_QUEUE_SIZE);

  /* Specific Initialization */
#if (DISPLAY_WAIT_MODEM_IS_ON == 1)
  /* Modem is on Initialization */
  cellular_app_uiclient_modem_is_on = false;
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1 */
  /* Display is Initialized */
  cellular_app_uiclient_display_init_ok = false;
  /* Screen State Initialization */
  cellular_app_uiclient_screen_state = UICLIENT_SCREEN_OFF;

#if (USE_RTC == 1)
  /* Need a timer to read periodically the DateTime */
  cellular_app_uiclient_datetime_timer_id = rtosalTimerNew(NULL, (os_ptimer)cellular_app_uiclient_datetime_timer_cb,
                                                           osTimerPeriodic, NULL);
  cellular_app_uiclient_datetime_timer_state = CELLULAR_APP_UICLIENT_TIMER_IDLE;
#endif /* USE_RTC == 1 */

#if (USE_SENSORS == 1)
  /* Need a timer to read periodically the Sensors */
  cellular_app_uiclient_sensors_timer_id = rtosalTimerNew(NULL, (os_ptimer)cellular_app_uiclient_sensors_timer_cb,
                                                          osTimerPeriodic, NULL);
  cellular_app_uiclient_sensors_timer_state = CELLULAR_APP_UICLIENT_TIMER_IDLE;
#endif /* USE_SENSORS == 1 */

  /* Check Initialization is ok */
  if (cellular_app_uiclient.queue_id == NULL)
  {
    CELLULAR_APP_ERROR(CELLULAR_APP_ERROR_UICLIENT, ERROR_FATAL)
  }
#if (USE_SENSORS == 1)
  if (cellular_app_uiclient_sensors_timer_id == NULL)
  {
    cellular_app_uiclient_sensors_timer_state = CELLULAR_APP_UICLIENT_TIMER_INVALID;
    CELLULAR_APP_ERROR((CELLULAR_APP_ERROR_UICLIENT + 1), ERROR_FATAL)
  }
#endif /* USE_SENSORS == 1 */
}

/**
  * @brief  Start UIClt thread
  * @param  -
  * @retval -
  */
void cellular_app_uiclient_start(void)
{
  uint8_t thread_name[CELLULAR_APP_THREAD_NAME_MAX];
  uint32_t len;

  /* Cellular initialization already done - Registration to services is OK */
  /* Registration to CellularInfo: needs to know all cellular status modification to update the display */
  if (cellular_info_cb_registration(cellular_app_uiclient_cellular_info_status_cb, (void *) NULL) != CELLULAR_SUCCESS)
  {
    CELLULAR_APP_ERROR((CELLULAR_APP_ERROR_UICLIENT + 2), ERROR_FATAL)
  }
  /* Registration to CellularSignalInfo: needs to know all signal status modification to update the display */
  if (cellular_signal_info_cb_registration(cellular_app_uiclient_signal_info_status_cb, (void *) NULL)
      != CELLULAR_SUCCESS)
  {
    CELLULAR_APP_ERROR((CELLULAR_APP_ERROR_UICLIENT + 3), ERROR_FATAL)
  }

#if (USE_RTC == 1)
  /* Registration to cellular_app_datetime_set_cb() to know when datetime is set */
  cellular_app_datetime_set_cb_registration(cellular_app_uiclient_datetime_set_cb, NULL);
#endif /* USE_DISPLAY == 1 */

#if (USE_SENSORS == 1)
  if (cellular_app_uiclient_sensors_timer_state == CELLULAR_APP_UICLIENT_TIMER_IDLE)
  {
    /* Initialization Sensors modules */
    cellular_app_sensors_init();
    /* Initialization Sensor Humidity */
    if (cellular_app_sensors_initialize(CELLULAR_APP_SENSOR_TYPE_HUMIDITY) == false)
    {
      PRINT_FORCE("%s: Humidity sensors init NOK!", p_cellular_app_uiclient_trace)
    }
    /* Initialization Sensor Pressure */
    if (cellular_app_sensors_initialize(CELLULAR_APP_SENSOR_TYPE_PRESSURE) == false)
    {
      PRINT_FORCE("%s: Pressure sensor init NOK!", p_cellular_app_uiclient_trace)
    }
    /* Initialization Sensors Temperature */
    if (cellular_app_sensors_initialize(CELLULAR_APP_SENSOR_TYPE_TEMPERATURE) == false)
    {
      PRINT_FORCE("%s: Temperature sensor init NOK!", p_cellular_app_uiclient_trace)
    }
  }
#endif /* USE_SENSORS == 1 */
  /* Thread Name Generation */
  len = crs_strlen((const uint8_t *)"UIClt");
  /* '+1' to copy '\0' */
  (void)memcpy(thread_name, "UIClt", CELLULAR_APP_MIN((len + 1U), CELLULAR_APP_THREAD_NAME_MAX));

  /* Thread Creation */
  cellular_app_uiclient.thread_id = rtosalThreadNew((const rtosal_char_t *)thread_name, (os_pthread)uiclient_thread,
                                                    UICLIENT_THREAD_PRIO, UICLIENT_THREAD_STACK_SIZE, NULL);
  /* Check Creation is ok */
  if (cellular_app_uiclient.thread_id == NULL)
  {
    CELLULAR_APP_ERROR((CELLULAR_APP_ERROR_UICLIENT + 4), ERROR_FATAL)
  }
}

#endif /* USE_DISPLAY == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
