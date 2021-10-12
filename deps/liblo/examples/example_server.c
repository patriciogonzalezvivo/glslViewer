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
#ifndef WIN32
# include <unistd.h>
#endif
#include "lo/lo.h"

int done = 0;

void error(int num, const char *m, const char *path);

int generic_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);

int foo_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data);

int blobtest_handler(const char *path, const char *types, lo_arg ** argv,
                     int argc, void *data, void *user_data);

int quit_handler(const char *path, const char *types, lo_arg ** argv,
                 int argc, void *data, void *user_data);

int main()
{
    /* start a new server on port 7770 */
    lo_server_thread st = lo_server_thread_new("7770", error);

    /* add method that will match any path and args */
    lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);

    /* add method that will match the path /foo/bar, with two numbers, coerced
     * to float and int */
    lo_server_thread_add_method(st, "/foo/bar", "fi", foo_handler, NULL);

    /* add method that will match the path /blobtest with one blob arg */
    lo_server_thread_add_method(st, "/blobtest", "b", blobtest_handler, NULL);

    /* add method that will match the path /quit with no args */
    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);

    lo_server_thread_start(st);

    while (!done) {
#ifdef WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }

    lo_server_thread_free(st);

    return 0;
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int generic_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)
{
    int i;

    printf("path: <%s>\n", path);
    for (i = 0; i < argc; i++) {
        printf("arg %d '%c' ", i, types[i]);
        lo_arg_pp((lo_type)types[i], argv[i]);
        printf("\n");
    }
    printf("\n");
    fflush(stdout);

    return 1;
}

int foo_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data)
{
    /* example showing pulling the argument values out of the argv array */
    printf("%s <- f:%f, i:%d\n\n", path, argv[0]->f, argv[1]->i);
    fflush(stdout);

    return 0;
}

int blobtest_handler(const char *path, const char *types, lo_arg ** argv,
                     int argc, void *data, void *user_data)
{
    /* example showing how to get data for a blob */
    int i, size = argv[0]->blob.size;
    char mydata[6];

    unsigned char *blobdata = (unsigned char*)lo_blob_dataptr((lo_blob)argv[0]);
    int blobsize = lo_blob_datasize((lo_blob)argv[0]);

    /* Alternatively:
     * blobdata = &argv[0]->blob.data;
     * blobsize = argv[0]->blob.size;
     */

    /* Don't trust network input! Blob can be anything, so check if
       each character is a letter A-Z. */
    for (i=0; i<6 && i<blobsize; i++)
        if (blobdata[i] >= 'A' && blobdata[i] <= 'Z')
            mydata[i] = blobdata[i];
        else
            mydata[i] = '.';
    mydata[5] = 0;

    printf("%s <- length:%d '%s'\n", path, size, mydata);
    fflush(stdout);

    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg ** argv,
                 int argc, void *data, void *user_data)
{
    done = 1;
    printf("quiting\n\n");
    fflush(stdout);

    return 0;
}

/* vi:set ts=8 sts=4 sw=4: */
