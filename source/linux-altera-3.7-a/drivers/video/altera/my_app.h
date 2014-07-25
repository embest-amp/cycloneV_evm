/*****************************************************************************
 *  File: my_app.h
 *
 *  This file contains the constants, structure definitions, and funtion
 *  prototypes for the application selector.
 *
 ****************************************************************************/

#ifndef MY_APP_H_
#define MY_APP_H_

#include "alt_video_display.h"

#define ALT_VIDEO_DISPLAY_COLS 			(1920)
#define ALT_VIDEO_DISPLAY_ROWS 			(1080)
#define ALT_VIDEO_DISPLAY_COLOR_DEPTH 	32
// #define ALT_VIDEO_DISPLAY_SGDMA_NAME LCD_SGDMA_NAME

#define NUM_VIDEO_FRAMES 					1

// Main menu actions
#define AS_ACTION_NONE                  0
#define AS_ACTION_LOAD_APP              1
#define AS_ACTION_DISPLAY_APP_INFO      2
#define AS_ACTION_MY_APP_EXIT     3

//int AsInitTpoLcd( alt_tpo_lcd* lcd_serial );
//int AsInitTouchscreen( alt_touchscreen* touchscreen );

#endif /*MY_APP_H_*/
