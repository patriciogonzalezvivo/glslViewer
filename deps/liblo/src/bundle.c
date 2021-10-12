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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lo_types_internal.h"
#include "lo/lo.h"

lo_bundle lo_bundle_new(lo_timetag tt)
{
    lo_bundle b = (lo_bundle) calloc(1, sizeof(struct _lo_bundle));

    b->size = 4;
    b->len = 0;
    b->ts = tt;
    b->elmnts = (lo_element*) calloc(b->size, sizeof(lo_element));
    b->refcount = 0;

    return b;
}

void lo_bundle_incref(lo_bundle b)
{
    b->refcount ++;
}

static
void lo_bundle_decref(lo_bundle b)
{
    b->refcount --;
}

static lo_bundle *push_to_list(lo_bundle *list, lo_bundle ptr, size_t *len, size_t *size)
{
    if (*len >= *size) {
	*size *= 2;
	list = (lo_bundle*) realloc(list, *size * sizeof(lo_bundle));
    }

    list[*len] = ptr;
    (*len)++;

    return list;
}

static lo_bundle *pop_from_list(lo_bundle *list, size_t *len, size_t *size)
{
    (*len)--;
    return list;
}

static int is_in_list(lo_bundle *list, lo_bundle ptr, size_t *len)
{
    size_t i;

    for (i = 0; i < *len; i++)
	if (list[i] == ptr)
	    return -1;
    
    return 0;
}

static lo_bundle *walk_tree(lo_bundle *B, lo_bundle b, size_t *len, size_t *size, int *ret)
{
    size_t i;
    int res;

    // check whether bundle is part of parents list
    if (is_in_list(B, b, len)) {
	*ret = -1;
	return B;
    }

    // push bundle to parents list
    B = push_to_list(B, b, len, size);

    res = 0;
    for (i = 0; i < b->len; i++) {
	if (b->elmnts[i].type == LO_ELEMENT_BUNDLE) {
	    B = walk_tree(B, b->elmnts[i].content.bundle, len, size, &res);
	    if (res)
		break;
	}
    }

    // pop bundle from parents list
    B = pop_from_list(B, len, size);

    *ret = res;
    return B;
}

static int lo_bundle_circular(lo_bundle b)
{
    size_t len = 0;
    size_t size = 4;
    int res;
    lo_bundle *B = (lo_bundle*) calloc(size, sizeof(lo_bundle));

    B = walk_tree(B, b, &len, &size, &res);

    if (B)
	free(B);

    return res;
}

static int lo_bundle_add_element(lo_bundle b, int type, const char *path, void *elmnt)
{
    if (b->len >= b->size) {
	b->size *= 2;
	b->elmnts = (lo_element*) realloc(b->elmnts,
                                      b->size * sizeof(lo_element));
	if (!b->elmnts)
	    return -1;
    }

    b->elmnts[b->len].type = (lo_element_type) type;

    switch (type) {
	case LO_ELEMENT_MESSAGE: {
	    lo_message msg = (lo_message) elmnt;
        lo_message_incref(msg);
	    b->elmnts[b->len].content.message.msg = msg;
	    b->elmnts[b->len].content.message.path = strdup(path);
	    (b->len)++;
	    break;
	}
	case LO_ELEMENT_BUNDLE: {
	    lo_bundle bndl = (lo_bundle) elmnt;
        lo_bundle_incref(bndl);
	    b->elmnts[b->len].content.bundle = bndl;
	    (b->len)++;

	    // do not add bundle if a circular reference is found
	    if (lo_bundle_circular(b))
        {
            // note that this is a special case where we _know_ that
            // decrement should not result in a free, therefore we
            // avoid it explicitly.  otherwise double-free might
            // happen if calling code still has a reference to the
            // added bundle, since it's possible that refcount==0.
            lo_bundle_decref(bndl);
            (b->len)--;
            return -1;
	    }
	    break;
	}
    }

    return 0;
}

int lo_bundle_add_message(lo_bundle b, const char *path, lo_message m)
{
    if (!m)
        return 0;

    return lo_bundle_add_element (b, LO_ELEMENT_MESSAGE, path, m);
}

int lo_bundle_add_bundle(lo_bundle b, lo_bundle n)
{
    if (!n)
	return 0;
    
    return lo_bundle_add_element (b, LO_ELEMENT_BUNDLE, NULL, n);
}

lo_element_type lo_bundle_get_type(lo_bundle b, int index)
{
    if (index < (int)b->len) {
        return b->elmnts[index].type;
    }
    return (lo_element_type) 0;
}

lo_bundle lo_bundle_get_bundle(lo_bundle b, int index)
{
    if ( (index < (int)b->len) && (b->elmnts[index].type == LO_ELEMENT_BUNDLE) ) {
        return b->elmnts[index].content.bundle;
    }
    return 0;
}

lo_message lo_bundle_get_message(lo_bundle b, int index,
                                 const char **path)
{
    if ( (index < (int)b->len) && (b->elmnts[index].type == LO_ELEMENT_MESSAGE) ) {
        if (path)
            *path = b->elmnts[index].content.message.path;
        return b->elmnts[index].content.message.msg;
    }
    return 0;
}

lo_timetag lo_bundle_get_timestamp(lo_bundle b)
{
    return b->ts;
}

unsigned int lo_bundle_count(lo_bundle b)
{
    return b->len;
}

size_t lo_bundle_length(lo_bundle b)
{
    size_t size = 16;           /* "#bundle" and the timetag */
    size_t i;

    if (!b) {
        return 0;
    }

    size += b->len * 4;         /* sizes */
    for (i = 0; i < b->len; i++)
	switch (b->elmnts[i].type) {
	    case LO_ELEMENT_BUNDLE:
		size += lo_bundle_length(b->elmnts[i].content.bundle);
		break;
	    case LO_ELEMENT_MESSAGE:
		size += lo_message_length(b->elmnts[i].content.message.msg, b->elmnts[i].content.message.path);
		break;
	}

    return size;
}

void *lo_bundle_serialise(lo_bundle b, void *to, size_t * size)
{
    size_t s, skip;
    int32_t *bes;
    size_t i;
    char *pos;
    lo_pcast32 be;

    if (!b) {
        if (size)
            *size = 0;
        return NULL;
    }

    s = lo_bundle_length(b);
    if (size) {
        *size = s;
    }

    if (!to) {
        to = calloc(1, s);
    }

    pos = (char*) to;
    strcpy(pos, "#bundle");
    pos += 8;

    be.nl = lo_htoo32(b->ts.sec);
    memcpy(pos, &be, 4);
    pos += 4;
    be.nl = lo_htoo32(b->ts.frac);
    memcpy(pos, &be, 4);
    pos += 4;

    for (i = 0; i < b->len; i++) {
	switch (b->elmnts[i].type) {
	    case LO_ELEMENT_MESSAGE:
		lo_message_serialise(b->elmnts[i].content.message.msg, b->elmnts[i].content.message.path, pos + 4, &skip);
		break;
	    case LO_ELEMENT_BUNDLE:
		lo_bundle_serialise(b->elmnts[i].content.bundle, pos+4, &skip);
		break;
	}

	bes = (int32_t *) (void *)pos;
	*bes = lo_htoo32(skip);
	pos += skip + 4;

	if (pos > (char*) to + s) {
            fprintf(stderr, "liblo: data integrity error at message %lu\n",
                    (long unsigned int)i);

	    return NULL;
	}
    }
    if (pos != (char*) to + s) {
        fprintf(stderr, "liblo: data integrity error\n");
        if (to) {
            free(to);
        }
        return NULL;
    }

    return to;
}

void lo_bundle_free(lo_bundle b)
{
    if (!b) {
        return;
    }

    lo_bundle_decref(b);
    if (b->refcount > 0)
        return;

    free(b->elmnts);
    free(b);
}

static void collect_element(lo_element *elmnt)
{
    switch (elmnt->type) {
	case LO_ELEMENT_MESSAGE: {
	    lo_message msg = elmnt->content.message.msg;
		lo_message_free(msg);
        free((char*)elmnt->content.message.path);
	    break;
	}

	case LO_ELEMENT_BUNDLE: {
	    lo_bundle bndl = elmnt->content.bundle;
        lo_bundle_free_recursive(bndl);
	    break;
	}
    }
}

void lo_bundle_free_messages(lo_bundle b)
{
    lo_bundle_free_recursive(b);
}

void lo_bundle_free_recursive(lo_bundle b)
{
    size_t i;

    if (!b)
        return;

    lo_bundle_decref(b);
    if (b->refcount > 0)
        return;

    for (i = 0; i < b->len; i++)
	collect_element(&b->elmnts[i]);

    free(b->elmnts);
    free(b);
}

static void offset_pp(int offset, int *state)
{
    int i;

    for (i=0; i < offset; i++) {
	if (!state[i])
	    printf("│        ");
	else // last
	    printf("         ");
    }

    if (!state[offset])
	printf("├─");
    else // last
	printf("└─");
}

static int *lo_bundle_pp_internal(lo_bundle b, unsigned int offset,
                                  int *state, size_t *len)
{
    size_t i;
   
    if (offset+2 > *len) {
	*len *= 2;
	state = (int*) realloc(state, *len*sizeof(int));
    }

    offset_pp(offset, state);
    printf("bundle─┬─(%08x.%08x)\n", b->ts.sec, b->ts.frac);
    for (i = 0; i < b->len; i++) {
	state[offset+1] = i != b->len-1 ? 0 : 1;

	switch(b->elmnts[i].type) {
	    case LO_ELEMENT_MESSAGE:
		offset_pp(offset+1, state);
		printf("%s ", b->elmnts[i].content.message.path);
		lo_message_pp(b->elmnts[i].content.message.msg);
		break;
	    case LO_ELEMENT_BUNDLE:
		state = lo_bundle_pp_internal(b->elmnts[i].content.bundle, offset+1, state, len);
		break;
	}
    }

    return state;
}

void lo_bundle_pp(lo_bundle b)
{
    size_t len;
    int *state;

    if (!b)
        return;

    len = 4;
    state = (int*) calloc(len, sizeof(int));

    state[0] = 1;
    state = lo_bundle_pp_internal(b, 0, state, &len);
    free (state);
}

/* vi:set ts=8 sts=4 sw=4: */
