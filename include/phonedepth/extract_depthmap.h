#pragma once

#include <cstddef>
// #include <cstdint>
// #include <stdbool.h>

typedef enum image_type_t {
    TYPE_NONE,
    TYPE_RAW,
    TYPE_JPEG
} image_type_t;

extern "C" {

    int extract_depth(  const char *filename, 
                        const unsigned char **cv, size_t *cv_size,
                        const unsigned char **dm, size_t *dm_size, image_type_t *dm_type,  
                        const unsigned char **extra_image, size_t *extra_image_size, image_type_t *extra_image_type);

}