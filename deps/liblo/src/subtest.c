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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "lo/lo.h"

static int subtest_count = 0;

int subtest_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, lo_message data, void *user_data);

int main(int argc, char *argv[])
{
    lo_server_thread st;
    lo_address t;
    int tries;

    printf("entered subtest\n");

    st = lo_server_thread_new(NULL, NULL);

    if (argc != 2) {
        fprintf(stderr, "Usage: subtest <uri>\n");

        return 1;
    }

    lo_server_thread_add_method(st, NULL, "i", subtest_handler, NULL);
    lo_server_thread_start(st);

    printf("subtest: creating new address `%s'\n", argv[1]);
    t = lo_address_new_from_url(argv[1]);
    lo_send(t, "/subtest", "i", 0xf00);

    tries = 400;
    while (subtest_count == 0 && (--tries > 0)) {
#if defined(WIN32) || defined(_MSC_VER)
        Sleep(10);
#else
        usleep(10000);
#endif
    }

    if (tries == 0) {
        printf("subtest: too many tries\n");
        exit(1);
    }

    return 0;
}

int subtest_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, lo_message data, void *user_data)
{
    int i;
    lo_address a = lo_message_get_source(data);
    static char *uri = NULL;

    printf("subtest: got reply (%s)\n", path);

    if (!uri) {
        uri = lo_address_get_url(a);
    } else {
        char *new_uri = lo_address_get_url(a);

        if (strcmp(uri, new_uri)) {
            printf("ERROR: %s != %s\n", uri, new_uri);

            exit(1);
        }
        free(new_uri);
    }

    lo_send(a, "/subtest-reply", "i", 0xbaa);

    if (lo_address_errno(a)) {
        fprintf(stderr, "subtest error %d: %s\n", lo_address_errno(a),
                lo_address_errstr(a));

        exit(1);
    }

    for (i = 0; i < 10; i++) {
        lo_send(a, "/subtest-reply", "i", 0xbaa + i);
#if defined(WIN32) || defined(_MSC_VER)
        Sleep(2);
#else
        usleep(2000);
#endif
    }

    subtest_count ++;

    return 0;
}

/* vi:set ts=8 sts=4 sw=4: */
