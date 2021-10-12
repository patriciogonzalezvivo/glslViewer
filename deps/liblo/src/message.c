/*
 *  Copyright (C) 2014 Steve Harris et al. (see AUTHORS)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(WIN32) || defined(_MSC_VER)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include "lo_types_internal.h"
#include "lo_internal.h"
#include "lo/lo.h"

#define LO_DEF_TYPE_SIZE 8
#define LO_DEF_DATA_SIZE 8

static char lo_numerical_types[] = {
    LO_INT32,
    LO_FLOAT,
    LO_INT64,
    LO_DOUBLE,
    '\0'
};

static char lo_string_types[] = {
    LO_STRING,
    LO_SYMBOL,
    '\0'
};

static int lo_message_add_typechar(lo_message m, char t);
static void *lo_message_add_data(lo_message m, size_t s);
void lo_arg_pp_internal(lo_type type, void *data, int bigendian);

// Used for calculating new sizes when expanding message data buffers.
// Note that log(x)/0.69315 = log2(x): this simply finds the next
// highest power of 2.
#if 1
#define lo_pow2_over(a,b) \
    a = ((b > a) ? (a << ((int)((log(((double)b/(double)a))/0.69315)+1))) : a);
#else
#define lo_pow2_over(a,b) \
    while (b > a) {a *= 2;}
#endif

lo_message lo_message_new()
{
    lo_message m = (lo_message) malloc(sizeof(struct _lo_message));
    if (!m) {
        return m;
    }

    m->types = (char*) calloc(LO_DEF_TYPE_SIZE, sizeof(char));
    m->types[0] = ',';
    m->types[1] = '\0';
    m->typelen = 1;
    m->typesize = LO_DEF_TYPE_SIZE;
    m->data = NULL;
    m->datalen = 0;
    m->datasize = 0;
    m->source = NULL;
    m->argv = NULL;
    m->ts = LO_TT_IMMEDIATE;
    m->refcount = 0;

    return m;
}

void lo_message_incref(lo_message m)
{
    m->refcount ++;
}

lo_message lo_message_clone(lo_message m)
{
	lo_message c;

    if (!m) {
	return NULL;
    }

    c = (lo_message) malloc(sizeof(struct _lo_message));
    if (!c) {
	return NULL;
    }

    c->types = (char*) calloc(m->typesize, sizeof(char));
    strcpy (c->types, m->types);
    c->typelen = m->typelen;
    c->typesize = m->typesize;
    c->data = calloc(m->datasize, sizeof(uint8_t));
    memcpy(c->data, m->data, m->datalen);
    c->datalen = m->datalen;
    c->datasize = m->datasize;
    c->source = NULL;
    c->argv = NULL;
    c->ts = LO_TT_IMMEDIATE;
    c->refcount = 0;

    return c;
}

void lo_message_free(lo_message m)
{
    if (m && (--m->refcount) <= 0)
    {
        free(m->types);
        free(m->data);
        free(m->argv);
        free(m);
    }
}

/* Don't call lo_message_add_varargs_internal directly, use
 * lo_message_add_varargs, a macro wrapping this function with
 * appropriate values for file and line */

int lo_message_add_varargs_internal(lo_message msg, const char *types,
                                    va_list ap, const char *file, int line)
{
    int count = 0;
    int ret = 0;

    while (types && *types) {
        count++;

        switch (*types++) {

        case LO_INT32:{
                int32_t i = va_arg(ap, int32_t);
                lo_message_add_int32(msg, i);
                break;
            }

        case LO_FLOAT:{
                float f = (float) va_arg(ap, double);
                lo_message_add_float(msg, f);
                break;
            }

        case LO_STRING:{
                char *s = va_arg(ap, char *);
#ifndef USE_ANSI_C
                if (s == (char*) LO_MARKER_A) {
                    fprintf(stderr,
                            "liblo error: lo_send or lo_message_add called with "
                            "invalid string pointer for arg %d, probably arg mismatch\n"
                            "at %s:%d, exiting.\n", count, file, line);
                }
#endif
                lo_message_add_string(msg, s);
                break;
            }

        case LO_BLOB:{
                lo_blob b = va_arg(ap, lo_blob);
                lo_message_add_blob(msg, b);
                break;
            }

        case LO_INT64:{
                int64_t i64 = va_arg(ap, int64_t);
                lo_message_add_int64(msg, i64);
                break;
            }

        case LO_TIMETAG:{
                lo_timetag tt = va_arg(ap, lo_timetag);
                lo_message_add_timetag(msg, tt);
                break;
            }

        case LO_DOUBLE:{
                double d = va_arg(ap, double);
                lo_message_add_double(msg, d);
                break;
            }

        case LO_SYMBOL:{
                char *s = va_arg(ap, char *);
#ifndef USE_ANSI_C
                if (s == (char*) LO_MARKER_A) {
                    fprintf(stderr,
                            "liblo error: lo_send or lo_message_add called with "
                            "invalid symbol pointer for arg %d, probably arg mismatch\n"
                            "at %s:%d, exiting.\n", count, file, line);
                    va_end(ap);
                    return -2;
                }
#endif
                lo_message_add_symbol(msg, s);
                break;
            }

        case LO_CHAR:{
                char c = va_arg(ap, int);
                lo_message_add_char(msg, c);
                break;
            }

        case LO_MIDI:{
                uint8_t *m = va_arg(ap, uint8_t *);
                lo_message_add_midi(msg, m);
                break;
            }

        case LO_TRUE:
            lo_message_add_true(msg);
            break;

        case LO_FALSE:
            lo_message_add_false(msg);
            break;

        case LO_NIL:
            lo_message_add_nil(msg);
            break;

        case LO_INFINITUM:
            lo_message_add_infinitum(msg);
            break;

        case '$':
            if (*types == '$') {
                // type strings ending in '$$' indicate not to perform
                // LO_MARKER checking
                va_end(ap);
                return 0;
            }
            // fall through to unknown type

        default:{
                ret = -1;       // unknown type
                fprintf(stderr,
                        "liblo warning: unknown type '%c' at %s:%d\n",
                        *(types - 1), file, line);
                break;
            }
        }
    }
#ifndef USE_ANSI_C
    void *i = va_arg(ap, void *);
    if (((UINT_PTR)i & 0xFFFFFFFFUL)
	!= ((UINT_PTR)LO_MARKER_A & 0xFFFFFFFFUL))
    {
        ret = -2;               // bad format/args
        fprintf(stderr,
                "liblo error: lo_send, lo_message_add, or lo_message_add_varargs called with "
                "mismatching types and data at\n%s:%d, exiting.\n", file,
                line);
        va_end(ap);
        return ret;
    }
    i = va_arg(ap, void *);
    if (((UINT_PTR)i & 0xFFFFFFFFUL)
	!= ((UINT_PTR)LO_MARKER_B & 0xFFFFFFFFUL))
    {
        ret = -2;               // bad format/args
        fprintf(stderr,
                "liblo error: lo_send, lo_message_add, or lo_message_add_varargs called with "
                "mismatching types and data at\n%s:%d, exiting.\n", file,
                line);
    }
#endif
    va_end(ap);

    return ret;
}

#if defined(USE_ANSI_C) || defined(DLL_EXPORT)
int lo_message_add(lo_message msg, const char *types, ...)
{
    va_list ap;
    const char *file = "";
    const int line = 0;
    va_start(ap, types);
    return lo_message_add_varargs_internal(msg, types, ap, file, line);
}
#endif

/* Don't call lo_message_add_internal directly, use lo_message_add,
 * a macro wrapping this function with appropriate values for file and line */

int lo_message_add_internal(lo_message msg, const char *file,
                            const int line, const char *types, ...)
{
    va_list ap;
    va_start(ap, types);
    return lo_message_add_varargs_internal(msg, types, ap, file, line);
}

int lo_message_add_int32(lo_message m, int32_t a)
{
    lo_pcast32 b;
    int32_t *nptr = (int32_t*) lo_message_add_data(m, sizeof(a));
    if (!nptr)
        return -1;
    b.i = a;

    if (lo_message_add_typechar(m, LO_INT32))
        return -1;
    *nptr = b.nl;
    return 0;
}

int lo_message_add_float(lo_message m, float a)
{
    lo_pcast32 b;
    int32_t *nptr = (int32_t*) lo_message_add_data(m, sizeof(a));
    if (!nptr)
        return -1;
    b.f = a;

    if (lo_message_add_typechar(m, LO_FLOAT))
        return -1;
    *nptr = b.nl;
    return 0;
}

int lo_message_add_string(lo_message m, const char *a)
{
    const int size = lo_strsize(a);
    char *nptr = (char*) lo_message_add_data(m, size);
    if (!nptr)
        return -1;

    if (lo_message_add_typechar(m, LO_STRING))
        return -1;
    strncpy(nptr, a, size);
    return 0;
}

int lo_message_add_blob(lo_message m, lo_blob a)
{
    const uint32_t size = lo_blobsize(a);
    const uint32_t dsize = lo_blob_datasize(a);
    char *nptr = (char*) lo_message_add_data(m, size);
    if (!nptr)
        return -1;

    if (lo_message_add_typechar(m, LO_BLOB))
        return -1;
    memset(nptr + size - 4, 0, 4);

    memcpy(nptr, &dsize, sizeof(dsize));
    memcpy(nptr + sizeof(int32_t), lo_blob_dataptr(a),
           lo_blob_datasize(a));
    return 0;
}

int lo_message_add_int64(lo_message m, int64_t a)
{
    lo_pcast64 b;
    uint64_t *nptr = (uint64_t*) lo_message_add_data(m, sizeof(a));
    if (!nptr)
        return -1;
    b.i = a;

    if (lo_message_add_typechar(m, LO_INT64))
        return -1;
    *nptr = b.nl;
    return 0;
}

int lo_message_add_timetag(lo_message m, lo_timetag a)
{
    uint32_t *nptr = (uint32_t*) lo_message_add_data(m, sizeof(a));
    if (!nptr)
        return -1;

    if (lo_message_add_typechar(m, LO_TIMETAG))
        return -1;
    *nptr = a.sec;
    nptr++;
    *nptr = a.frac;
    return 0;
}

int lo_message_add_double(lo_message m, double a)
{
    lo_pcast64 b;
    uint64_t *nptr = (uint64_t*) lo_message_add_data(m, sizeof(a));
    if (!nptr)
        return -1;
    b.f = a;

    if (lo_message_add_typechar(m, LO_DOUBLE))
        return -1;
    *nptr = b.nl;
    return 0;
}

int lo_message_add_symbol(lo_message m, const char *a)
{
    const int size = lo_strsize(a);
    char *nptr = (char*) lo_message_add_data(m, size);
    if (!nptr)
        return -1;

    if (lo_message_add_typechar(m, LO_SYMBOL))
        return -1;
    strncpy(nptr, a, size);
    return 0;
}

int lo_message_add_char(lo_message m, char a)
{
    lo_pcast32 b;
    int32_t *nptr = (int32_t*) lo_message_add_data(m, sizeof(int32_t));
    if (!nptr)
        return -1;

    b.i = 0; // zero the 32 bits before writing the char
    b.c = a;

    if (lo_message_add_typechar(m, LO_CHAR))
        return -1;
    *nptr = b.nl;
    return 0;
}

int lo_message_add_midi(lo_message m, uint8_t a[4])
{
    char *nptr = (char*) lo_message_add_data(m, 4);
    if (!nptr)
        return -1;

    if (lo_message_add_typechar(m, LO_MIDI))
        return -1;

    memcpy(nptr, a, 4 * sizeof(uint8_t));
    return 0;
}

int lo_message_add_true(lo_message m)
{
    return lo_message_add_typechar(m, LO_TRUE);
}

int lo_message_add_false(lo_message m)
{
    return lo_message_add_typechar(m, LO_FALSE);
}

int lo_message_add_nil(lo_message m)
{
    return lo_message_add_typechar(m, LO_NIL);
}

int lo_message_add_infinitum(lo_message m)
{
    return lo_message_add_typechar(m, LO_INFINITUM);
}

static int lo_message_add_typechar(lo_message m, char t)
{
    if (m->typelen + 1 >= m->typesize) {
        int new_typesize = m->typesize * 2;
        char *new_types = 0;
        if (!new_typesize)
            new_typesize = LO_DEF_TYPE_SIZE;
        new_types = (char*) realloc(m->types, new_typesize);
        if (!new_types)
            return -1;
        m->types = new_types;
        m->typesize = new_typesize;
    }
    m->types[m->typelen] = t;
    m->typelen++;
    m->types[m->typelen] = '\0';
    if (m->argv) {
        free(m->argv);
        m->argv = NULL;
    }
    return 0;
}

static void *lo_message_add_data(lo_message m, size_t s)
{
    uint32_t old_dlen = m->datalen;
    int new_datasize = m->datasize;
    int new_datalen = m->datalen + s;
    void *new_data = 0;

    if (!new_datasize)
        new_datasize = LO_DEF_DATA_SIZE;

    lo_pow2_over(new_datasize, new_datalen);
    new_data = realloc(m->data, new_datasize);
    if (!new_data)
        return 0;

    m->datalen = new_datalen;
    m->datasize = new_datasize;
    m->data = new_data;

    if (m->argv) {
        free(m->argv);
        m->argv = NULL;
    }

    return (void *) ((char*) m->data + old_dlen);
}

int lo_strsize(const char *s)
{
    return 4 * (strlen(s) / 4 + 1);
}

size_t lo_arg_size(lo_type type, void *data)
{
    switch (type) {
    case LO_TRUE:
    case LO_FALSE:
    case LO_NIL:
    case LO_INFINITUM:
        return 0;

    case LO_INT32:
    case LO_FLOAT:
    case LO_MIDI:
    case LO_CHAR:
        return 4;

    case LO_INT64:
    case LO_TIMETAG:
    case LO_DOUBLE:
        return 8;

    case LO_STRING:
    case LO_SYMBOL:
        return lo_strsize((char*) data);

    case LO_BLOB:
        return lo_blobsize((lo_blob) data);

    default:
        fprintf(stderr,
                "liblo warning: unhandled OSC type '%c' at %s:%d\n", type,
                __FILE__, __LINE__);
        return 0;
    }

    return 0;
}

char *lo_get_path(void *data, ssize_t size)
{
    ssize_t result = lo_validate_string(data, size);
    return (result >= 4) ? (char*) data : NULL;
}

ssize_t lo_validate_string(void *data, ssize_t size)
{
    ssize_t i = 0, len = 0;
    char *pos = (char*) data;

    if (size < 0) {
        return -LO_ESIZE;       // invalid size
    }
    for (i = 0; i < size; ++i) {
        if (pos[i] == '\0') {
            len = 4 * (i / 4 + 1);
            break;
        }
    }
    if (0 == len) {
        return -LO_ETERM;       // string not terminated
    }
    if (len > size) {
        return -LO_ESIZE;       // would overflow buffer
    }
    for (; i < len; ++i) {
        if (pos[i] != '\0') {
            return -LO_EPAD;    // non-zero char found in pad area
        }
    }
    return len;
}


ssize_t lo_validate_blob(void *data, ssize_t size)
{
    ssize_t i, end, len;
    uint32_t dsize;
    char *pos = (char*) data;

    if (size < 0) {
        return -LO_ESIZE;       // invalid size
    }
    dsize = lo_otoh32(*(uint32_t *) data);
    // described size must fit within the buffer
    if (dsize > size) {      // avoid int overflow in next step
        return -LO_ESIZE;
    }
    end = sizeof(uint32_t) + dsize;     // end of data
    len = 4 * ((end + 3) / 4);  // full padded size
    if (len > size) {
        return -LO_ESIZE;       // would overflow buffer
    }
    for (i = end; i < len; ++i) {
        if (pos[i] != '\0') {
            return -LO_EPAD;    // non-zero char found in pad area
        }
    }
    return len;
}


ssize_t lo_validate_bundle(void *data, ssize_t size)
{
    ssize_t len = 0, remain = size;
    char *pos = (char*) data;
    ssize_t elem_len;

    len = lo_validate_string(data, size);
    if (len < 0) {
        return -LO_ESIZE;       // invalid size
    }
    if (0 != strcmp((const char*) data, "#bundle")) {
        return -LO_EINVALIDBUND;        // not a bundle
    }
    pos += len;
    remain -= len;

    // time tag
    if (remain < 8) {
        return -LO_ESIZE;
    }
    pos += 8;
    remain -= 8;

    while (remain >= 4) {
        elem_len = lo_otoh32(*((uint32_t *) pos));
        pos += 4;
        remain -= 4;
        if (elem_len > remain) {
            return -LO_ESIZE;
        }
        pos += elem_len;
        remain -= elem_len;
    }
    if (0 != remain) {
        return -LO_ESIZE;
    }
    return size;
}


ssize_t lo_validate_arg(lo_type type, void *data, ssize_t size)
{
    if (size < 0) {
        return -1;
    }
    switch (type) {
    case LO_TRUE:
    case LO_FALSE:
    case LO_NIL:
    case LO_INFINITUM:
        return 0;

    case LO_INT32:
    case LO_FLOAT:
    case LO_MIDI:
    case LO_CHAR:
        return size >= 4 ? 4 : -LO_ESIZE;

    case LO_INT64:
    case LO_TIMETAG:
    case LO_DOUBLE:
        return size >= 8 ? 8 : -LO_ESIZE;

    case LO_STRING:
    case LO_SYMBOL:
        return lo_validate_string((char*) data, size);

    case LO_BLOB:
        return lo_validate_blob((lo_blob) data, size);

    default:
        return -LO_EINVALIDTYPE;
    }
    return -LO_INT_ERR;
}

/* convert endianness of arg pointed to by data from network to host */
void lo_arg_host_endian(lo_type type, void *data)
{
    switch (type) {
    case LO_INT32:
    case LO_FLOAT:
    case LO_BLOB:
    case LO_CHAR:
        *(int32_t *) data = lo_otoh32(*(int32_t *) data);
        break;

    case LO_TIMETAG:
        *(int32_t *) data = lo_otoh32(*(int32_t *) data);
        data = ((int32_t *) data) + 1;
        *(int32_t *) data = lo_otoh32(*(int32_t *) data);
        break;

    case LO_INT64:
    case LO_DOUBLE:
        *(int64_t *) data = lo_otoh64(*(int64_t *) data);
        break;

    case LO_STRING:
    case LO_SYMBOL:
    case LO_MIDI:
    case LO_TRUE:
    case LO_FALSE:
    case LO_NIL:
    case LO_INFINITUM:
        /* these are fine */
        break;

    default:
        fprintf(stderr,
                "liblo warning: unhandled OSC type '%c' at %s:%d\n", type,
                __FILE__, __LINE__);
        break;
    }
}

/* convert endianness of arg pointed to by data from host to network */
void lo_arg_network_endian(lo_type type, void *data)
{
    switch (type) {
    case LO_INT32:
    case LO_FLOAT:
    case LO_BLOB:
    case LO_CHAR:
        *(int32_t *) data = lo_htoo32(*(int32_t *) data);
        break;

    case LO_TIMETAG:
        *(uint32_t *) data = lo_htoo32(*(uint32_t *) data);
        data = ((uint32_t *) data) + 1;
        *(uint32_t *) data = lo_htoo32(*(uint32_t *) data);
        break;

    case LO_INT64:
    case LO_DOUBLE:
        *(int64_t *) data = lo_htoo64(*(int64_t *) data);
        break;

    case LO_STRING:
    case LO_SYMBOL:
    case LO_MIDI:
    case LO_TRUE:
    case LO_FALSE:
    case LO_NIL:
    case LO_INFINITUM:
        /* these are fine */
        break;

    default:
        fprintf(stderr,
                "liblo warning: unhandled OSC type '%c' at %s:%d\n", type,
                __FILE__, __LINE__);
        break;
    }
}

lo_address lo_message_get_source(lo_message m)
{
    return m->source;
}

lo_timetag lo_message_get_timestamp(lo_message m)
{
    return m->ts;
}

size_t lo_message_length(lo_message m, const char *path)
{
    return lo_strsize(path) + lo_strsize(m->types) + m->datalen;
}

int lo_message_get_argc(lo_message m)
{
    return m->typelen - 1;
}

lo_arg **lo_message_get_argv(lo_message m)
{
    int i, argc;
    char *types, *ptr;
    lo_arg **argv;

    if (NULL != m->argv) {
        return m->argv;
    }

    argc = m->typelen - 1;
    types = m->types + 1;
    ptr = (char*) m->data;

    argv = (lo_arg**) calloc(argc, sizeof(lo_arg *));
    for (i = 0; i < argc; ++i) {
        size_t len = lo_arg_size((lo_type) types[i], ptr);
        argv[i] = len ? (lo_arg *) ptr : NULL;
        ptr += len;
    }
    m->argv = argv;
    return argv;
}

char *lo_message_get_types(lo_message m)
{
    return m->types + 1;
}

void *lo_message_serialise(lo_message m, const char *path, void *to,
                           size_t * size)
{
    int i, argc;
    char *types, *ptr;
    size_t s = lo_message_length(m, path);

    if (size) {
        *size = s;
    }

    if (!to) {
        to = calloc(1, s);
    }
    memset((char*) to + lo_strsize(path) - 4, 0, 4);   // ensure zero-padding
    strcpy((char*) to, path);
    memset((char*) to + lo_strsize(path) + lo_strsize(m->types) - 4, 0,
           4);
    strcpy((char*) to + lo_strsize(path), m->types);

    types = m->types + 1;
    ptr = (char*) to + lo_strsize(path) + lo_strsize(m->types);
    memcpy(ptr, m->data, m->datalen);

    argc = m->typelen - 1;
    for (i = 0; i < argc; ++i) {
        size_t len = lo_arg_size((lo_type) types[i], ptr);
        lo_arg_network_endian((lo_type) types[i], ptr);
        ptr += len;
    }
    return to;
}


lo_message lo_message_deserialise(void *data, size_t size, int *result)
{
    lo_message msg = NULL;
    char *types = NULL, *ptr = NULL;
    int i = 0, argc = 0, remain = size, res = 0, len;

    if (remain <= 0) {
        res = LO_ESIZE;
        goto fail;
    }

    msg = (lo_message) malloc(sizeof(struct _lo_message));
    if (!msg) {
        res = LO_EALLOC;
        goto fail;
    }

    msg->types = NULL;
    msg->typelen = 0;
    msg->typesize = 0;
    msg->data = NULL;
    msg->datalen = 0;
    msg->datasize = 0;
    msg->source = NULL;
    msg->argv = NULL;
    msg->ts = LO_TT_IMMEDIATE;
    msg->refcount = 0;

    // path
    len = lo_validate_string(data, remain);
    if (len < 0) {
        res = LO_EINVALIDPATH;  // invalid path string
        goto fail;
    }
    remain -= len;

    // types
    if (remain <= 0) {
        res = LO_ENOTYPE;       // no type tag string
        goto fail;
    }
    types = (char*) data + len;
    len = lo_validate_string(types, remain);
    if (len < 0) {
        res = LO_EINVALIDTYPE;  // invalid type tag string
        goto fail;
    }
    if (types[0] != ',') {
        res = LO_EBADTYPE;      // type tag string missing initial comma
        goto fail;
    }
    remain -= len;

    msg->typelen = strlen(types);
    msg->typesize = len;
    msg->types = (char*) malloc(msg->typesize);
    if (NULL == msg->types) {
        res = LO_EALLOC;
        goto fail;
    }
    memcpy(msg->types, types, msg->typesize);

    // args
    msg->data = malloc(remain);
    if (NULL == msg->data) {
        res = LO_EALLOC;
        goto fail;
    }
    memcpy(msg->data, types + len, remain);
    msg->datalen = msg->datasize = remain;
    ptr = (char*) msg->data;

    ++types;
    argc = msg->typelen - 1;
    if (argc) {
        msg->argv = (lo_arg **) calloc(argc, sizeof(lo_arg *));
        if (NULL == msg->argv) {
            res = LO_EALLOC;
            goto fail;
        }
    }

    for (i = 0; remain >= 0 && i < argc; ++i) {
        len = lo_validate_arg((lo_type) types[i], ptr, remain);
        if (len < 0) {
            res = LO_EINVALIDARG;       // invalid argument
            goto fail;
        }
        lo_arg_host_endian((lo_type) types[i], ptr);
        msg->argv[i] = len ? (lo_arg *) ptr : NULL;
        remain -= len;
        ptr += len;
    }
    if (0 != remain || i != argc) {
        res = LO_ESIZE;         // size/argument mismatch
        goto fail;
    }

    if (result) {
        *result = res;
    }
    return msg;

  fail:
    if (msg) {
        lo_message_free(msg);
    }
    if (result) {
        *result = res;
    }
    return NULL;
}

void lo_message_pp(lo_message m)
{
    void *d = m->data;
    void *end = (char*) m->data + m->datalen;
    int i;

    printf("%s ", m->types);
    for (i = 1; m->types[i]; i++) {
        if (i > 1) {
            printf(" ");
        }

        lo_arg_pp_internal((lo_type) m->types[i], d, 0);
        d = (char*) d + lo_arg_size((lo_type) m->types[i], d);
    }
    putchar('\n');
    if (d != end) {
        fprintf(stderr,
                "liblo warning: type and data do not match (off by %ld) in message %p\n",
                labs((char*) d - (char*) end), m);
    }
}

void lo_arg_pp(lo_type type, void *data)
{
    lo_arg_pp_internal(type, data, 0);
}

void lo_arg_pp_internal(lo_type type, void *data, int bigendian)
{
    lo_pcast32 val32;
    lo_pcast64 val64 = {0};
    lo_timetag valtt = { 0, 1 };
    int size;
    int i;

    size = lo_arg_size(type, data);
    if (size == 4 || type == LO_BLOB) {
        if (bigendian) {
            val32.nl = lo_otoh32(*(int32_t *) data);
        } else {
            val32.nl = *(int32_t *) data;
        }
    } else if (type == LO_TIMETAG) {
        valtt.sec =
            bigendian ? lo_otoh32(*(uint32_t *) data) : *(uint32_t *) data;
        data = (uint32_t *) data + 1;
        valtt.frac =
            bigendian ? lo_otoh32(*(uint32_t *) data) : *(uint32_t *) data;
    } else if (size == 8) {
        if (bigendian) {
            val64.nl = lo_otoh64(*(int64_t *) data);
        } else {
            val64.nl = *(int64_t *) data;
        }
    }

    switch (type) {
    case LO_INT32:
        printf("%d", val32.i);
        break;

    case LO_FLOAT:
        printf("%f", val32.f);
        break;

    case LO_STRING:
        printf("\"%s\"", (char*) data);
        break;

    case LO_BLOB:
        printf("[");
        if (val32.i > 12) {
            printf("%d byte blob", val32.i);
        } else {
            printf("%db ", val32.i);
            for (i = 0; i < val32.i; i++) {
                printf("%#02x", (unsigned int)*((unsigned char *) (data) + 4 + i));
                if (i + 1 < val32.i)
                    printf(" ");
            }
        }
        printf("]");
        break;

    case LO_INT64:
        printf("%" PRINTF_LL "d", (long long int) val64.i);
        break;

    case LO_TIMETAG:
        printf("%08x.%08x", valtt.sec, valtt.frac);
        break;

    case LO_DOUBLE:
        printf("%f", val64.f);
        break;

    case LO_SYMBOL:
        printf("'%s", (char*) data);
        break;

    case LO_CHAR:
        printf("'%c'", (char) val32.c);
        break;

    case LO_MIDI:
        printf("MIDI [");
        for (i = 0; i < 4; i++) {
            printf("0x%02x", *((uint8_t *) (data) + i));
            if (i + 1 < 4)
                printf(" ");
        }
        printf("]");
        break;

    case LO_TRUE:
        printf("#T");
        break;

    case LO_FALSE:
        printf("#F");
        break;

    case LO_NIL:
        printf("Nil");
        break;

    case LO_INFINITUM:
        printf("Infinitum");
        break;

    default:
        fprintf(stderr, "liblo warning: unhandled type: %c\n", type);
        break;
    }
}

int lo_is_numerical_type(lo_type a)
{
    return strchr(lo_numerical_types, a) != 0;
}

int lo_is_string_type(lo_type a)
{
    return strchr(lo_string_types, a) != 0;
}

int lo_coerce(lo_type type_to, lo_arg * to, lo_type type_from,
              lo_arg * from)
{
    if (type_to == type_from) {
        memcpy(to, from, lo_arg_size(type_from, from));

        return 1;
    }

    if (lo_is_string_type(type_to) && lo_is_string_type(type_from)) {
        strcpy((char*) to, (char*) from);

        return 1;
    }

    if (lo_is_numerical_type(type_to) && lo_is_numerical_type(type_from)) {
        switch (type_to) {
        case LO_INT32:
            to->i = (uint32_t) lo_hires_val(type_from, from);
            break;

        case LO_INT64:
            to->i64 = (uint64_t) lo_hires_val(type_from, from);
            break;

        case LO_FLOAT:
            to->f = (float) lo_hires_val(type_from, from);
            break;

        case LO_DOUBLE:
            to->d = (double) lo_hires_val(type_from, from);
            break;

        default:
            fprintf(stderr, "liblo: bad coercion: %c -> %c\n", type_from,
                    type_to);
            return 0;
        }
        return 1;
    }

    return 0;
}

lo_hires lo_hires_val(lo_type type, lo_arg * p)
{
    switch (type) {
    case LO_INT32:
        return p->i;
    case LO_INT64:
        return p->h;
    case LO_FLOAT:
        return p->f;
    case LO_DOUBLE:
        return p->d;
    default:
        fprintf(stderr,
                "liblo: hires val requested of non numerical type '%c' at %s:%d\n",
                type, __FILE__, __LINE__);
        break;
    }

    return 0.0l;
}



/* vi:set ts=8 sts=4 sw=4: */
