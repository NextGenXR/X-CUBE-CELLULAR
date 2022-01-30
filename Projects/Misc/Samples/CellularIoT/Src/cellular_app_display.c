/**
  ******************************************************************************
  * @file    cellular_app_display.c
  * @author  MCD Application Team
  * @brief   Implements functions for display actions
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
#include "cellular_app_display.h"

#if (USE_DISPLAY == 1)

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#if defined (DISPLAY_INTERFACE)
#if (DISPLAY_INTERFACE == SPI_INTERFACE)
#include "sys_spi.h"
#define CELLULAR_APP_DISPLAY_LINK_INIT()      sys_spi_init()
#define CELLULAR_APP_DISPLAY_LINK_POWERON()   sys_spi_power_on()
#define CELLULAR_APP_DISPLAY_LINK_IN()        sys_spi_acquire(SYS_SPI_DISPLAY_CONFIGURATION)
#define CELLULAR_APP_DISPLAY_LINK_OUT()       sys_spi_release(SYS_SPI_DISPLAY_CONFIGURATION)
#else /* (DISPLAY_INTERFACE != SPI_INTERFACE) */
#define CELLULAR_APP_DISPLAY_LINK_INIT()      true
#define CELLULAR_APP_DISPLAY_LINK_POWERON()   true
#define CELLULAR_APP_DISPLAY_LINK_IN()        __NOP()
#define CELLULAR_APP_DISPLAY_LINK_OUT()       __NOP()
#endif /* DISPLAY_INTERFACE == SPI_INTERFACE */
#else /* !defined DISPLAY_INTERFACE */
#define CELLULAR_APP_DISPLAY_LINK_INIT()      true
#define CELLULAR_APP_DISPLAY_LINK_POWERON()   true
#define CELLULAR_APP_DISPLAY_LINK_IN()        __NOP()
#define CELLULAR_APP_DISPLAY_LINK_OUT()       __NOP()
#endif /* !defined DISPLAY_INTERFACE */

#define CELLULAR_APP_DISPLAY_INSTANCE         (uint32_t)0

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#if 0
__weak void UTIL_LCD_Refresh(void);
__weak void UTIL_LCD_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *p_data);
__weak void UTIL_LCD_DrawRawFrameBuffer(uint8_t *p_data);
__weak void UTIL_LCD_DrawRawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *p_data);
#endif /* 0 */
/* Private Functions Definition ----------------------------------------------*/
#if 0
/**
  * @brief  Refresh LCD BSP function - not always defined
  * @param  -
  * @retval -
  */
__weak void UTIL_LCD_Refresh(void)
{
  __NOP();
}
/**
  * @brief  Draw a RGB Image on the LCD
  * @param  Xpos   - X position (in pixels)
  * @param  Ypos   - Y position (in pixels)
  * @param  Xsize  - size on X axis
  * @param  Ysize  - size on Y axis
  * @param  p_data - pointer on RGB image to draw
  * @retval -
  */
__weak void UTIL_LCD_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *p_data)
{
  UNUSED(Xpos);
  UNUSED(Ypos);
  UNUSED(Xsize);
  UNUSED(Ysize);
  UNUSED(p_data);
}
/**
  * @brief  Draw a frame buffer on the LCD
  * @param  p_data - pointer on frame buffer to draw
  * @retval -
  */
__weak void UTIL_LCD_DrawRawFrameBuffer(uint8_t *p_data)
{
  UNUSED(p_data);
}
/**
  * @brief  Draw a bitmap image on the LCD
  * @param  Xpos   - X position (in pixels)
  * @param  Ypos   - Y position (in pixels)
  * @param  p_data - pointer on Bitmap to draw
  * @retval -
  */
__weak void UTIL_LCD_DrawRawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *p_data)
{
  UNUSED(Xpos);
  UNUSED(Ypos);
  UNUSED(p_data);
}
#endif /* 0 */
/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Clear LCD with a color
  * @param  color - color to use
  * @retval -
  */
void cellular_app_display_clear(uint16_t color)
{
  CELLULAR_APP_DISPLAY_LINK_IN();
  UTIL_LCD_Clear(color);
  CELLULAR_APP_DISPLAY_LINK_OUT();
}

/**
  * @brief  Sets the LCD background color.
  * @param  Color Layer background color code
  */
void cellular_app_display_set_BackColor(uint32_t color)
{
  UTIL_LCD_SetBackColor(color);
}

/**
  * @brief  Sets the LCD text color.
  * @param  Color Text color code
  */
void cellular_app_display_set_TextColor(uint32_t color)
{
  UTIL_LCD_SetTextColor(color);
}

/**
  * @brief  Refresh LCD
  * @param  -
  * @retval -
  */
void cellular_app_display_refresh(void)
{
  CELLULAR_APP_DISPLAY_LINK_IN();
  (void)BSP_LCD_Refresh(CELLULAR_APP_DISPLAY_INSTANCE);
  CELLULAR_APP_DISPLAY_LINK_OUT();
}

/**
  * @brief  Set font
  * @param  size - font size to set : 0(to restore the default value), 8, 12, 16, 20, 24
  * @retval -
  */
void cellular_app_display_font_set(uint8_t size)
{
  switch (size)
  {
    case 0U :
      UTIL_LCD_SetFont(&DISPLAY_DEFAULT_FONT);
      break;
    case 8U :
      UTIL_LCD_SetFont(&Font8);
      break;
    case 12U :
      UTIL_LCD_SetFont(&Font12);
      break;
    case 16U :
      UTIL_LCD_SetFont(&Font16);
      break;
    case 20U :
      UTIL_LCD_SetFont(&Font20);
      break;
    case 24U :
      UTIL_LCD_SetFont(&Font24);
      break;
    default :
      break;
  }
}

/**
  * @brief  Decrease font used
  * @param  -
  * @retval -
  */
void cellular_app_display_font_decrease(void)
{
  sFONT *p_font = UTIL_LCD_GetFont();

  if (p_font != NULL)
  {
    if (p_font == &Font12)
    {
      UTIL_LCD_SetFont(&Font8);
    }
    else if (p_font == &Font16)
    {
      UTIL_LCD_SetFont(&Font12);
    }
    else if (p_font == &Font20)
    {
      UTIL_LCD_SetFont(&Font16);
    }
    else if (p_font == &Font24)
    {
      UTIL_LCD_SetFont(&Font20);
    }
    else
    {
      __NOP();
    }
  }
}

/**
  * @brief  Get font height
  * @param  -
  * @retval uint32_t - font height
  */
uint32_t cellular_app_display_font_get_height(void)
{
  sFONT *p_font;
  uint32_t result = 0U;

  p_font = UTIL_LCD_GetFont();
  if (p_font != NULL)
  {
    /* Font height */
    result = p_font->Height;
  }

  return (result);
}

/**
  * @brief  Get the maximum number of characters per line (according to the current font)
  * @param  -
  * @retval uint32_t - maximum number of characters per line
  */
uint32_t cellular_app_display_characters_per_line(void)
{
  sFONT *p_font;
  uint32_t result = 0U;

  p_font = UTIL_LCD_GetFont();
  if ((p_font != NULL) && (p_font->Width != 0U))
  {
    result = (cellular_app_display_get_XSize() / p_font->Width);
  }

  return (result);
}

/**
  * @brief  Gets the LCD X size.
  * @retval LCD X size
  */
uint32_t cellular_app_display_get_XSize(void)
{
  uint32_t result;

  if (BSP_LCD_GetXSize(CELLULAR_APP_DISPLAY_INSTANCE, &result) != BSP_ERROR_NONE)
  {
    result = 0U;
  }

  return (result);
}

/**
  * @brief  Gets the LCD Y size.
  * @retval LCD Y size
  */
uint32_t cellular_app_display_get_YSize(void)
{
  uint32_t result;

  if (BSP_LCD_GetYSize(CELLULAR_APP_DISPLAY_INSTANCE, &result) != BSP_ERROR_NONE)
  {
    result = 0U;
  }

  return (result);
}

/**
  * @brief  Display a string on LCD.
  * @param  Xpos   - X position (in pixels)
  * @param  Ypos   - Y position (in pixels)
  * @param  p_data - pointer to string to display on LCD
  * @retval -
  */
void cellular_app_display_string(uint16_t Xpos, uint16_t Ypos, uint8_t *p_data)
{
  CELLULAR_APP_DISPLAY_LINK_IN();
  UTIL_LCD_DisplayStringAt(Xpos, Ypos, p_data, LEFT_MODE);
  CELLULAR_APP_DISPLAY_LINK_OUT();
}

#if 0
/**
  * @brief  Draw a RGB image on the LCD
  * @param  Xpos   - X position (in pixels)
  * @param  Ypos   - Y position (in pixels)
  * @param  Xsize  - size on X axis
  * @param  Ysize  - size on Y axis
  * @param  p_data - pointer on RGB image to draw
  * @retval -
  */
void cellular_app_display_draw_RGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *p_data)
{
  CELLULAR_APP_DISPLAY_LINK_IN();
  UTIL_LCD_DrawRGBImage(Xpos, Ypos, Xsize, Ysize, p_data);
  CELLULAR_APP_DISPLAY_LINK_OUT();
}
#endif /* undefined */

#if 0
/**
  * @brief  Draw a frame buffer on the LCD
  * @param  p_data - pointer on frame buffer to draw
  * @retval -
  */
void cellular_app_display_draw_RawFrameBuffer(uint8_t *p_data)
{
  CELLULAR_APP_DISPLAY_LINK_IN();
  UTIL_LCD_DrawRawFrameBuffer(p_data);
  CELLULAR_APP_DISPLAY_LINK_OUT();
}
#endif /* undefined */

/**
  * @brief  Draw a bitmap image on the LCD
  * @param  Xpos   - X position (in pixels)
  * @param  Ypos   - Y position (in pixels)
  * @param  p_data - pointer on Bitmap to draw
  * @retval -
  */
void cellular_app_display_draw_Bitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *p_data)
{
  CELLULAR_APP_DISPLAY_LINK_IN();
  UTIL_LCD_DrawBitmap(Xpos, Ypos, p_data);
  CELLULAR_APP_DISPLAY_LINK_OUT();
}

/**
  * @brief  Initialize the display
  * @retval bool - false/true - display init NOK/ display init OK
  */
bool cellular_app_display_init(void)
{
  bool result;

  result = CELLULAR_APP_DISPLAY_LINK_INIT();
  if (result == true)
  {
    CELLULAR_APP_DISPLAY_LINK_IN();
    result = CELLULAR_APP_DISPLAY_LINK_POWERON();
    if (result == true)
    {
      /* 1- Initialize LCD */
      if (BSP_LCD_Init(CELLULAR_APP_DISPLAY_INSTANCE) == BSP_ERROR_NONE)
      {
        /* 2- Link board LCD drivers to STM32 LCD Utility drivers */
        UTIL_LCD_SetFuncDriver(&LCD_Driver);
        /* 3- Set the LCD instance to be used */
        UTIL_LCD_SetDevice(CELLULAR_APP_DISPLAY_INSTANCE);
        cellular_app_display_font_set(0U);
        UTIL_LCD_SetTextColor(LCD_COLOR_WHITE);
        UTIL_LCD_SetBackColor(LCD_COLOR_BLACK);
        result = true;
      }
    }
    CELLULAR_APP_DISPLAY_LINK_OUT();
  }

  return (result);
}

/**
  * @brief  De-initialize the display
  * @retval bool - false/true - display de-init NOK/ display de-init OK
  */
bool cellular_app_display_deinit(void)
{
  bool result = false;

  CELLULAR_APP_DISPLAY_LINK_IN();
  if (BSP_LCD_DeInit(CELLULAR_APP_DISPLAY_INSTANCE) == BSP_ERROR_NONE)
  {
    result = true;
  }
  CELLULAR_APP_DISPLAY_LINK_OUT();

  return (result);
}

#endif /* USE_DISPLAY == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
