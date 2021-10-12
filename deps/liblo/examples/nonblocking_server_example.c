/*
 *  nonblocking_server_example.c
 *
 *  This code demonstrates two methods of monitoring both an lo_server
 *  and other I/O from a single thread.
 *
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
#include <sys/types.h>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

#include "lo/lo.h"

int done = 0;

void error(int num, const char *m, const char *path);

int generic_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);

int foo_handler(const char *path, const char *types, lo_arg ** argv,
                int argc, void *data, void *user_data);

int quit_handler(const char *path, const char *types, lo_arg ** argv,
                 int argc, void *data, void *user_data);

void read_stdin(void);

int main()
{
    int lo_fd;
    fd_set rfds;
#ifndef WIN32
    struct timeval tv;
#endif
    int retval;

    /* start a new server on port 7770 */
    lo_server s = lo_server_new("7770", error);

    /* add method that will match any path and args */
    lo_server_add_method(s, NULL, NULL, generic_handler, NULL);

    /* add method that will match the path /foo/bar, with two numbers, coerced
     * to float and int */
    lo_server_add_method(s, "/foo/bar", "fi", foo_handler, NULL);

    /* add method that will match the path /quit with no args */
    lo_server_add_method(s, "/quit", "", quit_handler, NULL);

    /* get the file descriptor of the server socket, if supported */
    lo_fd = lo_server_get_socket_fd(s);

    if (lo_fd > 0) {

        /* select() on lo_server fd is supported, so we'll use select()
         * to watch both stdin and the lo_server fd. */

        do {

            FD_ZERO(&rfds);
#ifndef WIN32
            FD_SET(0, &rfds);   /* stdin */
#endif
            FD_SET(lo_fd, &rfds);

            retval = select(lo_fd + 1, &rfds, NULL, NULL, NULL);        /* no timeout */

            if (retval == -1) {

                printf("select() error\n");
                exit(1);

            } else if (retval > 0) {

                if (FD_ISSET(0, &rfds)) {

                    read_stdin();

                }
                if (FD_ISSET(lo_fd, &rfds)) {

                    lo_server_recv_noblock(s, 0);

                }
            }

        } while (!done);

    } else {

        /* lo_server protocol does not support select(), so we'll watch
         * stdin while polling the lo_server. */
#ifdef WIN32
        printf
            ("non-blocking input from stdin not supported under Windows\n");
        exit(1);
#else
        do {

            FD_ZERO(&rfds);
            FD_SET(0, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 10000;

            retval = select(1, &rfds, NULL, NULL, &tv); /* timeout every 10ms */

            if (retval == -1) {

                printf("select() error\n");
                exit(1);

            } else if (retval > 0 && FD_ISSET(0, &rfds)) {

                read_stdin();

            }

            lo_server_recv_noblock(s, 0);

        } while (!done);
#endif
    }

    return 0;
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
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

int quit_handler(const char *path, const char *types, lo_arg ** argv,
                 int argc, void *data, void *user_data)
{
    done = 1;
    printf("quiting\n\n");

    return 0;
}

void read_stdin(void)
{
#ifdef WIN32
  return;
#else
    char buf[256];
    int len = read(0, buf, 256);
    if (len > 0) {
        printf("stdin: ");
        fwrite(buf, len, 1, stdout);
        printf("\n");
        fflush(stdout);
    }
#endif
}

/* vi:set ts=8 sts=4 sw=4: */
