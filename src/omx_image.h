#pragma once

#include <pthread.h>
#include "gl.h"
#include "omx_event_and_buffer_handler.h"

typedef enum {JPEG = 0, PNG} decoder_image_format_t;

void omx_image_fill_buffer_from_disk(char* filename, doctor_chompers_t *buffer);

int omx_image_loader(int texture_id,  EGLDisplay *eglDisplay, EGLContext *eglContext, int force_width, int force_height, int *output_width, int *output_height,  decoder_image_format_t image_format, doctor_chompers_t *input_buffer);