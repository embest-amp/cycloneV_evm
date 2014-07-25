/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2007 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
*                                                                             *
******************************************************************************/
#ifndef __ALT_VIDEO_DISPLAY_H__
#define __ALT_VIDEO_DISPLAY_H__

#include "system.h"
//#include "altera_avalon_sgdma.h"
//#include "altera_avalon_sgdma_descriptor.h"

/* Maximum number of display buffers the driver will accept */
#define ALT_VIDEO_DISPLAY_MAX_BUFFERS 2

#define ALT_VIDEO_DISPLAY_USE_HEAP -1
#define ALT_VIDEO_DISPLAY_BLACK_8 0x00

/* SGDMA can only transfer this many bytes per descriptor */
#define ALT_VIDEO_DISPLAY_BYTES_PER_DESC 0xFF00

/*
 * Video disoplay interrupt mode:
 * SGDMA Interrupt mask: Global IE & Interrupt on frame (chain) completion
 */

typedef struct {
  void *desc_base; /* Pointer to SGDMA descriptor chain */
  void *buffer;                    /* Pointer to video data buffer */
} alt_video_frame;

typedef struct {
  void *sgdma;
  alt_video_frame* buffer_ptrs[ALT_VIDEO_DISPLAY_MAX_BUFFERS];
  int buffer_being_displayed;
  int buffer_being_written;
  int width;
  int height;
  int color_depth;
  int bytes_per_pixel;
  int bytes_per_frame;
  int num_frame_buffers;
  int descriptors_per_frame;
} alt_video_display;

/* Public API */

alt_video_display* alt_video_display_only_frame_init(// char* sgdma_name,
                                           int width,
                                           int height,
                                           int color_depth,
                                           int buffer_location,
//                                           int descriptor_location,
                                           int num_buffers);


inline void alt_video_display_clear_screen ( alt_video_display* frame_buffer,
                                             char color );

// Private functions

#endif // __ALT_VIDEO_DISPLAY_H__
