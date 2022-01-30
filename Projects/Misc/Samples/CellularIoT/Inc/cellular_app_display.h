/**
  ******************************************************************************
  * @file    cellular_app_display.h
  * @author  MCD Application Team
  * @brief   Header for cellular_app_display.c module
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
#ifndef CELLULAR_APP_DISPLAY_H
#define CELLULAR_APP_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_DISPLAY == 1)

#include <stdbool.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  Clear LCD with a color
  * @param  color - color to use
  * @retval -
  */
void cellular_app_display_clear(uint16_t color);

/**
  * @brief  Sets the LCD background color.
  * @param  Color Layer background color code
  */
void cellular_app_display_set_BackColor(uint32_t color);

/**
  * @brief  Sets the LCD text color.
  * @param  Color Text color code
  */
void cellular_app_display_set_TextColor(uint32_t color);

/**
  * @brief  Refresh LCD
  * @param  -
  * @retval -
  */
void cellular_app_display_refresh(void);

/**
  * @brief  Set font
  * @param  size - font size to set : 0(to restore the default value), 8, 12, 16, 20, 24
  * @retval -
  */
void cellular_app_display_font_set(uint8_t size);

/**
  * @brief  Decrease font
  * @param  -
  * @retval -
  */
void cellular_app_display_font_decrease(void);

/**
  * @brief  Get font height
  * @param  -
  * @retval uint32_t - font height
  */
uint32_t cellular_app_display_font_get_height(void);

/**
  * @brief  Get the maximum number of characters per line (according to the current font)
  * @param  -
  * @retval uint32_t - maximum number of characters per line
  */
uint32_t cellular_app_display_characters_per_line(void);

/**
  * @brief  Gets the LCD X size.
  * @retval LCD X size
  */
uint32_t cellular_app_display_get_XSize(void);

/**
  * @brief  Gets the LCD Y size.
  * @retval LCD Y size
  */
uint32_t cellular_app_display_get_YSize(void);

/**
  * @brief  Display a string on LCD.
  * @param  Xpos   - X position (in pixels)
  * @param  Ypos   - Y position (in pixels)
  * @param  p_data - pointer to string to display on LCD
  * @retval -
  */
void cellular_app_display_string(uint16_t Xpos, uint16_t Ypos, uint8_t *p_data);

#if 0
/**
  * @brief  Draw a RGB image on the LCD
  * @param  Xpos   - X position (in pixels)
  * @param  Ypos   - Y position (in pixels)
  * @param  Xsize  - size on X axis
  * @param  Ysize  - size on Y axis
  * @param  p_data - pointer on image to draw
  * @retval -
  */
void cellular_app_display_draw_RGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *p_data);
#endif /* undefined */

#if 0
/**
  * @brief  Draw a frame buffer on the LCD
  * @param  p_data - pointer on frame buffer to draw
  * @retval -
  */
void cellular_app_display_draw_RawFrameBuffer(uint8_t *p_data);
#endif /* undefined */

/**
  * @brief  Draw a bitmap image on the LCD
  * @param  Xpos   - X position (in pixels)
  * @param  Ypos   - Y position (in pixels)
  * @param  p_data - pointer on Bitmap to draw
  * @retval -
  */
void cellular_app_display_draw_Bitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *p_data);

/**
  * @brief  Initialize the display
  * @retval bool - false/true - display init NOK/ display init OK
  */
bool cellular_app_display_init(void);

/**
  * @brief  De-initialize the display
  * @retval bool - false/true - display de-init NOK/ display de-init OK
  */
bool cellular_app_display_deinit(void);

#endif /* USE_DISPLAY == 1 */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_APP_DISPLAY_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
