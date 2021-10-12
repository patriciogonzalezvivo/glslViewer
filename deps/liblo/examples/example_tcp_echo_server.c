/*
 *  Copyright (C) 2012 Steve Harris, Stephen Sinclair
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

/* Run this program in one terminal with no arguments,
 *   $ ./example_tcp_echo_server
 * and in another terminal with argument 1,
 *   $ ./example_tcp_echo_server 1
 * Now watch the /test message ping back and forth a few times!
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
#include <signal.h>
#include "lo/lo.h"

int done = 0;
int count = 0;

void error(int num, const char *m, const char *path);

int echo_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);

int quit_handler(const char *path, const char *types, lo_arg ** argv,
                 int argc, void *data, void *user_data);

void ctrlc(int sig)
{
    done = 1;
}

int main(int argc, char *argv[])
{
    const char *port = "7770";
    int do_send = 0;

    if (argc > 1 && argv[1][0]=='1') {
        do_send = 1;
        port = "7771";
    }

    /* start a new server on port 7770 */
    lo_server_thread st = lo_server_thread_new_with_proto(port, LO_TCP, error);
    if (!st) {
        printf("Could not create server thread.\n");
        exit(1);
    }

    lo_server s = lo_server_thread_get_server(st);

    /* add method that will match the path /quit with no args */
    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);

    /* add method that will match any path and args */
    lo_server_thread_add_method(st, NULL, NULL, echo_handler, s);

    lo_server_thread_start(st);

    signal(SIGINT,ctrlc);

    printf("Listening on TCP port %s\n", port);

    lo_address a = 0;
    if (do_send) {
        a = lo_address_new_with_proto(LO_TCP, "localhost", "7770");
        if (!a) {
            printf("Error creating destination address.\n");
            exit(1);
        }
        int r = lo_send_from(a, s, LO_TT_IMMEDIATE, "/test", "ifs",
                             1, 2.0f, "3");
        if (r < 0)
            printf("Error sending initial message.\n");
        if (a) lo_address_free(a);
        a = 0;
    }

    while (!done) {
#ifdef WIN32
        Sleep(1);
#else
        sleep(1);
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

/* catch any incoming messages, display them, and send them
 * back. */
int echo_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)
{
    int i;
    lo_message m = (lo_message)data;
    lo_address a = lo_message_get_source(m);
    lo_server s = (lo_server)user_data;
    const char *host = lo_address_get_hostname(a);
    const char *port = lo_address_get_port(a);

    count ++;

#ifdef WIN32
    Sleep(1);
#else
    sleep(1);
#endif

    printf("path: <%s>\n", path);
    for (i = 0; i < argc; i++) {
        printf("arg %d '%c' ", i, types[i]);
        lo_arg_pp((lo_type)types[i], argv[i]);
        printf("\n");
    }

    if (!a) {
        printf("Couldn't get message source, quitting.\n");
        done = 1;
        return 0;
    }

    int r = lo_send_message_from(a, s, path, m);
    if (r < 0)
        printf("Error sending back message, socket may have closed.\n");
    else
        printf("Sent message back to %s:%s.\n", host, port);
    
    if (count >= 3) {
        printf("Got enough messages, quitting.\n");
        done = 1;
    }

    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg ** argv,
                 int argc, void *data, void *user_data)
{
    done = 1;
    printf("quitting\n\n");
    fflush(stdout);

    return 0;
}
