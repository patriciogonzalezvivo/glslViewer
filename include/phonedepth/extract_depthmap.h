#pragma once

#include <cstddef>

typedef enum image_type_t {
    TYPE_NONE,
    TYPE_RAW,
    TYPE_JPEG
} image_type_t;

extern "C" {

    int extract_depth(  const char *filename, 
                        const unsigned char **cv, size_t *cv_size,
                        const unsigned char **dm, size_t *dm_size, image_type_t *dm_type);

}