#include "extract_depthmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32

#else
#include <unistd.h>
#endif
#include <sys/types.h>

#define MIN(a, b)  ( (a) < (b) ? (a) : (b) )

extern "C" {

// // having these as global variables is ugly, but it made writing down the code easier :-P
unsigned char *file_data = NULL;
size_t file_size = 0;
int do_endianess_swap = 0;

// helper functions
void close_jpeg() {
    free(file_data);
    file_data = NULL;
    file_size = 0;
}

// read the file and get its size
bool open_jpeg(const char *filename) {
    FILE *fd = fopen(filename, "rb");
    if (!fd) {
        fprintf(stderr, "error opening file\n");
        return false;
    }

    fseek(fd, 0, SEEK_END);
    file_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    file_data = (unsigned char *)malloc(file_size);

    if (fread(file_data, 1, file_size, fd) != file_size) {
        fprintf(stderr, "error reading file\n");
        close_jpeg();
        return false;
    }

    fclose(fd);
    return true;
}

static void print_offset(const uint32_t offset) {
    //printf("%04X:%04X   ", (offset >> 16) & 0xffff, offset & 0xffff);
}

static void print_hex_string(const unsigned char *data, const size_t len) {
    fputs("  ", stdout);
    for(size_t i = 0; i < MIN(16, len); i++) {
        const unsigned char c = data[i];
        putchar(isprint(c) ? c : '.');
    }
    putchar('\n');
}

static void print_hex(const unsigned char* file_data, const unsigned char* data, const size_t len); //__attribute__((unused));
static void print_hex(const unsigned char *file_data, const unsigned char *data, const size_t len) {
    const size_t len_round_up = ((len + 15) / 16) * 16;
    for (size_t i = 0; i < len_round_up; i++) {
        if (i % 16 == 0)
            print_offset(data + i - file_data);

        if (i < len)
            printf("%02X", data[i]);
        else
            fputs("  ", stdout);

        if (i % 16 == 15)
            print_hex_string(data + i - 15, MIN(16, len - i + 15));
        else if (i % 4 == 3)
            fputs("   ", stdout);
        else
            putchar(' ');
    }
}

static uint16_t read_ui16(const unsigned char *data) {  return *(uint16_t *)data; }
static uint32_t read_ui32(const unsigned char *data) {  return *(uint32_t *)data; }

static uint16_t maybe_swap_16(const uint16_t value) {
    if (do_endianess_swap)
        return ((value << 8) & 0xff00) | ((value >> 8) & 0xff);
    else
        return value;
}

static uint32_t maybe_swap_32(const uint32_t value) {
    if (do_endianess_swap)
        return ((value << 24) & 0xff000000) | ((value << 8) & 0xff0000) | ((value >> 8) & 0xff00) | ((value >> 24) & 0xff);
    else
        return value;
}

// incomplete code for parsing JPEG tags. it doesn't support everything there is,
// just enough to read my sample files from a Samsung phone and get the image orientation

static int parse_exif (const unsigned char *data, const size_t size, uint32_t *orientation) {
    int result = 0;
    const unsigned char *start = data;

    // printf("parsing EXIF data\n");

    // we need at least 8 bytes of data:
    // 2 bytes for byte order -- either "II" or "MM"
    // 2 bytes for a fixed 0x002a
    // 4 bytes for offset to 0th IFD
    if (size < 8) {
        // fprintf(stderr, "premature end of Exif data\n");
        return result;
    }

    //   print_hex(data, size);
    //   FILE *fd = fopen("exif.data", "wb");
    //   fwrite(data, 1, size, fd);
    //   fclose(fd);

    // technically this is a TIFF file. we are only going to parse enough of it to get the orientation
    union {
        unsigned char c[2];
        uint16_t s;
    } endianess_check;

    endianess_check.c[0] = 0xbe;
    endianess_check.c[1] = 0xef;
    if (data[0] == 'I' && data[1] == 'I')
        do_endianess_swap = endianess_check.s == 0xbeef; // file is little endian
    else if (data[0] == 'M' && data[1] == 'M')
        do_endianess_swap = endianess_check.s != 0xbeef; // file is big endian
    else
        return result;

    data += 2;

    if (maybe_swap_16(read_ui16(data)) != 0x002a)
        return result;

    data += 2;

    // according to Exif specs the IFD0 is the first one and it's the one containing the data we want

    const uint32_t ifd_offset = maybe_swap_32(read_ui32(data));

    // the IFD takes at least 2 bytes of data for the number of fields
    if (ifd_offset + 1 + 2 > size) {
        // fprintf(stderr, "premature end of Exif data\n");
        return result;
    }

    const unsigned char *ifd = start + ifd_offset;

    const uint16_t ifd_count = maybe_swap_16(read_ui16(ifd));

    // the field array takes 12 bytes per field, and then there are 4 bytes for the pointer to the next IFD
    if (ifd_offset + 1 + 2 + 12 * ifd_count + 4 > size) {
        // fprintf(stderr, "premature end of Exif data\n");
        return result;
    }

    const unsigned char *field = ifd + 2;

    for(int i = 0; i < ifd_count; i++) {
        const uint16_t tag = maybe_swap_16(read_ui16(field));
        const uint16_t type = maybe_swap_16(read_ui16(field + 2));
        const uint32_t count = maybe_swap_32(read_ui32(field + 4));
      //     const uint32_t offset = maybe_swap_32(read_ui32(field + 8));

        if (tag == 0x0112) {
            // the orientation. this is all we are interested in
            if (type != 3 || count != 1) {
                fprintf(stderr, "malformed Exif data, Orientation tag is not a single short");
                return result;
            }
            // values of type SHORT are stored in the offset field directly
            *orientation = maybe_swap_16(read_ui16(field + 8));
            break;
        }

        field += 12;
    }

    result = 1;

    // puts("Exif parsing done");
    return result;
}

// static char* get_jpeg_tag_name(const unsigned char c) {
//     switch(c) {
//         case 0xc0: return (char*)"SOF0";
//         case 0xc4: return (char*)"DHT";

//         case 0xd0: return (char*)"RST0";
//         case 0xd1: return (char*)"RST1";
//         case 0xd2: return (char*)"RST2";
//         case 0xd3: return (char*)"RST3";
//         case 0xd4: return (char*)"RST4";
//         case 0xd5: return (char*)"RST5";
//         case 0xd6: return (char*)"RST6";
//         case 0xd7: return (char*)"RST7";
//         case 0xd8: return (char*)"SOI";
//         case 0xd9: return (char*)"EOI";
//         case 0xda: return (char*)"SOS";
//         case 0xdb: return (char*)"DQT";
//         case 0xdd: return (char*)"DRI";

//         case 0xe0: return (char*)"APP0";
//         case 0xe1: return (char*)"APP1";
//         case 0xe2: return (char*)"APP2";
//         case 0xe3: return (char*)"APP3";
//         case 0xe4: return (char*)"APP4";
//         case 0xe5: return (char*)"APP5";
//         case 0xe6: return (char*)"APP6";
//         case 0xe7: return (char*)"APP7";
//         case 0xe8: return (char*)"APP8";
//         case 0xe9: return (char*)"APP9";
//         case 0xea: return (char*)"APPa";
//         case 0xeb: return (char*)"APPb";
//         case 0xec: return (char*)"APPc";
//         case 0xed: return (char*)"APPd";
//         case 0xee: return (char*)"APPe";
//         case 0xef: return (char*)"APPf";

//         default: return (char*)"<unknown tag>";
//     }
// }

// variable length tags have the size stored after the tag type
static size_t get_jpeg_tag_size(const unsigned char *data) {
    return (size_t)256 * data[0] + data[1];
}

#define ADD_OFFSET_CHECK_BOUNDS(d, s) { \
                                          if (d + s > file_data + file_size) {\
                                              puts("            error: end of tag points behind end of file");\
                                              *_data = data;\
                                              return result;\
                                          }\
                                          d += s;\
                                      }

static int parse_jpeg_tag(const unsigned char **_data, uint32_t *orientation) {
    static const char EXIF_START_MARKER[] = {'E', 'x', 'i', 'f', '\0', '\0'};
    int result = 0;
    const unsigned char *data = *_data;

    print_offset(data - file_data);

    // we need at least 2 bytes of data
    if (data + 2 > file_data + file_size) {
        puts("error: premature end of file");
        *_data = data;
        return result;
    }

    // JPEG tags start with 0xFF and then the tag type in the next byte
    if (data[0] != 0xff) {
        printf("error: not a JPEG tag: %02X%02X\n", data[0], data[1]);
        *_data = data;
        return result;
    }

    const unsigned char tag_type = *++data;
    switch(tag_type) {
        case 0xd9:
            // puts(get_jpeg_tag_name(*data));
            data++;                   // skip tag type
            *_data = data;
            return result;
        case 0xd8:
            // puts(get_jpeg_tag_name(*data));
            data++;                   // skip tag type
            break;
        case 0xdd:
            // puts(get_jpeg_tag_name(*data));
            data++;                   // skip tag type
            ADD_OFFSET_CHECK_BOUNDS(data, 4)
            break;
        case 0xe1:
        case 0xe0:
        case 0xe2:
        case 0xe3:
        case 0xe4:
        case 0xe5:
        case 0xe6:
        case 0xe7:
        case 0xe8:
        case 0xe9:
        case 0xea:
        case 0xeb:
        case 0xec:
        case 0xed:
        case 0xee:
        case 0xef:
        case 0xc4:
        case 0xdb:
        case 0xc0:
        {
            // printf("%s, len: %ld\n", get_jpeg_tag_name(*data), get_jpeg_tag_size(data + 1));
            data++;                   // skip tag type
            const uint32_t tag_size = get_jpeg_tag_size(data);
            if (tag_type == 0xe1
                && data + 2 + sizeof(EXIF_START_MARKER) < file_data + file_size
                && !memcmp(EXIF_START_MARKER, data + 2, sizeof(EXIF_START_MARKER))) {
                if (!parse_exif (data + 2 + sizeof(EXIF_START_MARKER), tag_size - 2 - sizeof(EXIF_START_MARKER), orientation)) {
                    *_data = data;
                    return result;
                }
            }
            ADD_OFFSET_CHECK_BOUNDS(data, tag_size)
            break;
        }
        case 0xda:
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
        case 0xd4:
        case 0xd5:
        case 0xd6:
        case 0xd7:
        {
            // puts(get_jpeg_tag_name(*data));
            int  end_tag_found = 0;
            while((size_t)(data - file_data) < file_size) {
                if (data[0] == 0xff && data[1] != 0x00) {
                    end_tag_found = 1;
                    break;
                }
                data++;
            }
            if (!end_tag_found) {
                puts("            error: premature end of file");
                *_data = data;
              return result;
            }
            break;
        }
        default:
            printf("unknown tag FF%02X\n", data[0]);
            *_data = data;
            return result;
    }

    result = 1;

    *_data = data;
    return result;
}

#undef ADD_OFFSET_CHECK_BOUNDS

const unsigned char *parse_jpeg(const unsigned char *data, uint32_t *orientation) {
    while(parse_jpeg_tag(&data, orientation));
    return data;
}

// now we have some helper functions for reading the Samsung trailer

static const unsigned char *read_samsung_block_name(const unsigned char *data,
                                                    const uint16_t type,
                                                    const uint32_t size,
                                                    uint32_t *name_len) {
    const uint32_t _dummy = read_ui16(data);
    const uint32_t _type = read_ui16(data + 2);
    if (_dummy != 0 || _type != type)
        return NULL;

    *name_len = read_ui32(data + 4);

    if (*name_len + 8 > size)
        return NULL;

    return data + 8;
}

int parse_samsung_trailer(const unsigned char *data, const size_t len,
                          const unsigned char **cv, size_t *cv_size,
                          const unsigned char **dm, size_t *dm_size,
                          size_t *dm_width, size_t *dm_height, image_type_t *dm_type,
                          uint32_t *orientation) {
    const unsigned char *color_view = NULL, *depth_map = NULL;
    uint32_t color_view_size = 0, depth_map_size = 0;
    uint32_t depth_map_width = 0, depth_map_height = 0;

    const unsigned char *iter = data + len - 4;
    if (memcmp(iter, "SEFT", 4)) {
        // puts("trailer doesn't end with \"SEFT\"");
        return 0;
    }

    iter -= 4;

    const uint32_t block_len = read_ui32(iter);

    iter -= block_len;

    if (memcmp(iter, "SEFH", 4)) {
        // puts("can't find \"SEFH\"");
        return 0;
    }

    const unsigned char *directory = iter;

    iter += 8;

    const uint32_t count = read_ui32(iter);

    if (12 + 12 * count > block_len) {
        // puts("invalid count in trailer data");
        return 0;
    }

    for(uint32_t i = 0; i < count; i++) {
        iter = directory + 12 + 12 * i;
        const uint16_t type = read_ui16(iter + 2);
        const uint32_t offset = read_ui32(iter + 4);
        const uint32_t size = read_ui32(iter + 8);

        // we are only interested in certain types
        if (type != 0x01 && type != 0x0ab1 && type != 0x0ab3)
            continue;

        // reading the block name does some sanity check on the block data.
        // comparing it to what we expect to see isn't really needed, but it might help with broken files
        uint32_t name_len;
        const unsigned char *block_name = read_samsung_block_name(directory - offset, type, size, &name_len);

        if (!block_name) {
            // puts("error reading block name");
            return 0;
        }

        switch(type) {
            case 0x01:
                // DualShot_1 or DualShot_2 -- we only want number 1
                if (memcmp("DualShot_1", block_name, strlen("DualShot_1")))
                  continue;
                color_view = directory - offset + 8 + name_len;
                color_view_size = size - (8 + name_len);
                break;
            case 0x0ab1:
                // DualShot_DepthMap_1
                if (memcmp("DualShot_DepthMap_1", block_name, strlen("DualShot_DepthMap_1")))
                  continue;
                depth_map = directory - offset + 8 + name_len;
                depth_map_size = size - (8 + name_len);
                break;
            case 0x0ab3:
            {
                // DualShot_Extra_Info
                if (memcmp("DualShot_Extra_Info", block_name, strlen("DualShot_Extra_Info")))
                  continue;
                // read width/height of depth map
                const unsigned char *info_data = directory - offset + 8 + name_len;
                unsigned int w_offset, h_offset;
                const uint16_t version = read_ui16(info_data);
                if (version == 1) {
                    w_offset = 9;
                    h_offset = 10;
                }
                else if (version == 3) {
                    w_offset = 12;
                    h_offset = 13;
                }
                else if (version == 4) {
                    w_offset = 17;
                    h_offset = 18;
                }
                else if (version == 5) {
                    w_offset = 20;
                    h_offset = 19;
                }
                else if (version == 6) {
                    w_offset = 22;
                    h_offset = 21;
                }
                else {
                    // printf("unknown/unsupported version: %u\n", version);
                    // printf("depth map size: %u\n", depth_map_size);
                    // puts("DualShot_Extra_Info:");
                    // print_hex(file_data, directory - offset, size);
                    continue;
                }
                depth_map_width = read_ui32(info_data + w_offset * sizeof(uint32_t));
                depth_map_height = read_ui32(info_data + h_offset * sizeof(uint32_t));
                break;
            }
            default:
                continue;
        }
    }

    if (!color_view || !depth_map || color_view_size == 0 || depth_map_size == 0
        || depth_map_width == 0 || depth_map_height == 0
        || depth_map_width * depth_map_height != depth_map_size)
        return 0;

    *cv = color_view;
    *cv_size = color_view_size;
    *dm = depth_map;
    *dm_size = depth_map_size;
    *dm_width = depth_map_width;
    *dm_height = depth_map_height;
    *dm_type = TYPE_RAW;

    return 1;
}

// read the trailer of Huawei files and extract the images
int parse_huawei_trailer(const unsigned char *data, const size_t size,
                         const unsigned char **cv, size_t *cv_size,
                         const unsigned char **dm, size_t *dm_size,
                         size_t *dm_width, size_t *dm_height, image_type_t *dm_type,
                         uint32_t *orientation) {
    const size_t dm_header_size = 64;

    // check if file ends with "DepthEn\0"
    static const char HUAWEI_END_MARKER[] = {'D', 'e', 'p', 't', 'h', 'E', 'n', '\0'};

    const unsigned char *iter = data + size - sizeof(HUAWEI_END_MARKER);

    if (iter < data) {
        // fprintf(stderr, "filesize too small\n");
        return 0;
    }

    if (memcmp(iter, HUAWEI_END_MARKER, sizeof(HUAWEI_END_MARKER))) {
        // fprintf(stderr, "trailer doesn't end with \"DepthEn\\0\"\n");
        return 0;
    }

    // The 4 bytes before that are the relative jump to the start of the depth map structure
    iter -= 4;
    if (iter < data) {
        // fprintf(stderr, "filesize too small\n");
        return 0;
    }
    const uint32_t dm_block_size = read_ui32(iter);

    // go to the start of the data structure where the depth map is in
    iter -= dm_block_size;
    if (iter < data) {
        // fprintf(stderr, "filesize too small\n");
        return 0;
    }
    const unsigned char *dm_start = iter;
    const unsigned char *dm_data_start = dm_start + dm_header_size;

    print_hex(file_data, dm_start, dm_header_size);

    switch(dm_start[3]) {
        case 0x10: *orientation = 1; break;
        case 0x11: *orientation = 8; break;
        case 0x12: *orientation = 3; break;
        case 0x13: *orientation = 6; break;
        default: break;
    }

    const uint16_t width = read_ui16(dm_start + 12);
    const uint16_t height = read_ui16(dm_start + 14);

    if (width * height > dm_block_size) {
        // fprintf(stderr, "broken file\n");
        return 0;
    }

    // now we need the original color image
    iter = dm_start - 12;
    if (iter < data) {
        // fprintf(stderr, "filesize too small\n");
        return 0;
    }

    if (memcmp(iter + 4, "edof", 4) || read_ui32(iter + 8) != 0) {
        // fprintf(stderr, "unexpected file content\n");
        return 0;
    }

    const uint32_t jpeg_size = read_ui32(iter);

    iter -= jpeg_size;
    if (iter < data) {
        // fprintf(stderr, "filesize too small\n");
        return 0;
    }

    const unsigned char *jpeg_data_start = iter;

    *cv = jpeg_data_start;
    *cv_size = jpeg_size;
    *dm = dm_data_start;
    *dm_size = width * height;
    *dm_width = width;
    *dm_height = height;
    *dm_type = TYPE_RAW;

    return 1;
}

// read color images and depth maps from Apple files
int parse_apple_trailer(const unsigned char *data, 
                        const unsigned char **cv, size_t *cv_size,
                        const unsigned char **dm, size_t *dm_size, image_type_t *dm_type,
                        const unsigned char **mask, size_t *mask_size, image_type_t *mask_type) {

    // Apple just concatenates three JPEGs. First the color view, then a tiny depth map
    // and then a mostly black and white dm.

    if (!(data[0] == 0xff && data[1] == 0xd8))
        return 0;

    uint32_t _orientation;
    const unsigned char *trailer = parse_jpeg(data, &_orientation);

    *cv = file_data;
    *cv_size = data - file_data;
    *dm = data;
    *dm_size = trailer - data;
    *dm_type = TYPE_JPEG;
    size_t first_two_photo_sizes = *dm_size + *cv_size;
    *mask = trailer;
    *mask_size = file_size - *cv_size - *dm_size;
    *mask_type = TYPE_JPEG;

    unsigned char *trailer2;

    // other times it has more masks which displaces the location of the depth photo, so
    // if there are still photos left
    if (trailer + 2 < file_data + file_size) {
        trailer2 =  (unsigned char*)parse_jpeg(trailer, &_orientation);
        if (trailer2 == 0) {
            if (*mask_size > 0 && *dm_size > *mask_size) {
                *dm_size = file_size - *cv_size - *dm_size;
                *dm = trailer;
            }
            return 1;
        }
        *mask_size = trailer2 - trailer;
    }
    if (*mask_size > 0 && *dm_size > *mask_size) {
        *dm_size = file_size - *cv_size - *dm_size;
        *dm = trailer;
    }
    size_t noSize = -1;
    const unsigned char **extras[8] = {mask,mask,mask,mask,mask,mask,mask,mask};
    const unsigned char *trailers[8];
    trailers[0] = trailer;
    *mask_size = noSize;
    size_t extras_size[8] = {*mask_size,noSize,noSize,noSize,noSize,noSize,noSize,noSize};
    const unsigned char **placeholder_dm = dm;
    size_t *placeholder_dm_size = dm_size;
    int how_many_photos = 0;

    // if there are still photos left
    for (int i = 0 ; i < 5; i++) {
        // if the place at the start of the last photo accessed is less than the entire file size
        // there is data left so lets keep going!
        if (trailers[i] + 2 < file_data + file_size) {

            trailers[i+1] =  (unsigned char*)parse_jpeg(trailers[i], &_orientation);
            if (trailers[i+1] == 0) {
                return 1;
            }

            if (extras_size[i] == noSize) {
                extras_size[i] = trailers[i+1]-trailers[i];
            }
            if (i == 0) {
                *mask_size = trailers[i+1]-trailers[i];
            }
            *extras[i+1] = trailers[i+1];

            // if there no extra data left then this is the final size
            if (!(trailers[i+1] + 2 < file_data + file_size)) {
                size_t allExtraPhotoSizes = 0;
                for (int j = 0; j < i+1 ; j++) {
                    allExtraPhotoSizes += extras_size[j];
                }
                extras_size[i+1] = file_size - first_two_photo_sizes - *mask_size - allExtraPhotoSizes;
            }

            //for the selfies always the 6th photo (4th here bc the first two were already taken out) 
            // (except if the 6th photo is garbage)
            if (i == 4 && extras_size[i] > 0.) {
                placeholder_dm_size = &extras_size[i];
                placeholder_dm = &trailers[i];
            }
        }
        else{
            how_many_photos = i;
            break;
        }
        how_many_photos = i;
    }
    if (how_many_photos >= 4) {
        *dm_size = *placeholder_dm_size;
        *dm = *placeholder_dm ;
    }

    return 1;
}

int extract_depth(const char *filename, 
                  const unsigned char **cv, size_t *cv_size,
                  const unsigned char **dm, size_t *dm_size, image_type_t *dm_type) {

    int data_found = 0;
    uint32_t orientation = 0;
    size_t  dm_width = 0, dm_height = 0;

    // read the file
    if (!open_jpeg(filename)) {
        close_jpeg();
        return data_found;
    }

    // check that it is a JPEG
    if (file_size < 2) {
        // fprintf(stderr, "file too small\n");
        close_jpeg();
        return data_found;
    }
    if (file_data[0] != 0xff || file_data[1] != 0xd8) {
        // fprintf(stderr, "not a JPEG\n");
        close_jpeg();
        return data_found;
    }

    // read the JPEG to get the orientation from Exif
    // http://jpegclub.org/exif_orientation.html
    // printf("Parsing JPEG structure\n\n");
    const unsigned char *trailer = parse_jpeg(file_data, &orientation);

    const unsigned char *extra = NULL;
    size_t extra_size = 0;
    image_type_t extra_type = TYPE_NONE;

    // look for the Samsung data
    if (parse_samsung_trailer(file_data, file_size, cv, cv_size, dm, dm_size,
                                &dm_width, &dm_height, dm_type, &orientation)) {
        // printf("\nSamsung trailer depth data founded\n");
        data_found = 1;
    } 
    else if (parse_huawei_trailer(file_data, file_size, cv, cv_size, dm, dm_size,
                                  &dm_width, &dm_height, dm_type, &orientation)) {
        // printf("\nHuawei trailer detph data founded\n");
        data_found = 1;
    }
    else if (parse_apple_trailer( trailer, cv, cv_size, dm, dm_size,
                                  dm_type, &extra, &extra_size, &extra_type)) {
        // printf("\nApple depth data founded\n");
        data_found = 1;
    }

    // TODO: LG seems to work similar to Apple. they also concatenate JPEGs for color views,
    //       but then have a depth map in raw format it seems.

    // printf("Image orientation is %u\n", orientation);

    // cleanup
    close_jpeg();
    return data_found;
}

}