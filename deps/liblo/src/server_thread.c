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
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif
#include <sys/types.h>

#if defined(WIN32) || defined(_MSC_VER)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

#include "lo_types_internal.h"
#include "lo/lo.h"
#include "lo/lo_throw.h"

#ifdef HAVE_WIN32_THREADS
static unsigned __stdcall thread_func(void *data);
#else
static void* thread_func(void *data);
#endif

lo_server_thread lo_server_thread_new(const char *port,
                                      lo_err_handler err_h)
{
    return lo_server_thread_new_with_proto(port, LO_DEFAULT, err_h);
}

static lo_server_thread alloc_server_thread(lo_server s)
{
    lo_server_thread st;

    if (!s)
        return NULL;

    st = (lo_server_thread) malloc(sizeof(struct _lo_server_thread));

    st->s = s;
    st->active = 0;
    st->done = 0;
    st->cb_init = NULL;
    st->cb_cleanup = NULL;
    st->user_data = NULL;
    return st;
}

lo_server_thread lo_server_thread_new_multicast(const char *group,
                                                const char *port,
                                                lo_err_handler err_h)
{
    lo_server_thread st = alloc_server_thread(
        lo_server_new_multicast(group, port, err_h));
    return st;
}

lo_server_thread lo_server_thread_new_multicast_iface(const char *group, const char *port,
						      const char *iface, const char *ip,
						      lo_err_handler err_h)
{
    lo_server_thread st = alloc_server_thread(
        lo_server_new_multicast_iface(group, port, iface, ip, err_h));
    return st;
}

lo_server_thread lo_server_thread_new_with_proto(const char *port,
                                                 int proto,
                                                 lo_err_handler err_h)
{
    lo_server_thread st = alloc_server_thread(
        lo_server_new_with_proto(port, proto, err_h));
    return st;
}

lo_server_thread lo_server_thread_new_from_url(const char *url,
                                               lo_err_handler err_h)
{
    lo_server_thread st = alloc_server_thread(
        lo_server_new_from_url(url, err_h));
    return st;
}

void lo_server_thread_set_error_context(lo_server_thread st,
                                        void *user_data)
{
    lo_server_set_error_context(st->s, user_data);
}

void lo_server_thread_free(lo_server_thread st)
{
    if (st) {
        if (st->active) {
            lo_server_thread_stop(st);
        }
        lo_server_free(st->s);
    }
    free(st);
}

lo_method lo_server_thread_add_method(lo_server_thread st,
                                      const char *path,
                                      const char *typespec,
                                      lo_method_handler h,
                                      const void *user_data)
{
    return lo_server_add_method(st->s, path, typespec, h, user_data);
}

void lo_server_thread_del_method(lo_server_thread st, const char *path,
                                 const char *typespec)
{
    lo_server_del_method(st->s, path, typespec);
}

int lo_server_thread_del_lo_method(lo_server_thread st, lo_method m)
{
    return lo_server_del_lo_method(st->s, m);
}

void lo_server_thread_set_callbacks(lo_server_thread st,
                                    lo_server_thread_init_callback init,
                                    lo_server_thread_cleanup_callback cleanup,
                                    void *user_data)
{
    st->cb_init = init;
    st->cb_cleanup = cleanup;
    st->user_data = user_data;
}

int lo_server_thread_start(lo_server_thread st)
{
    if (!st->active) {
        st->active = 1;
        st->done = 0;

        // Create the server thread
#ifdef HAVE_LIBPTHREAD
        int result;
        result=pthread_create(&(st->thread), NULL, &thread_func, st);
        if (result)
        {
            fprintf (stderr,
                "Failed to create thread: pthread_create(), %s",
                strerror (result));
            return -result;
        }
#else
#ifdef HAVE_WIN32_THREADS
        st->thread = (HANDLE)_beginthreadex (NULL, 0, &thread_func, st, 0, NULL);

        if (st->thread == NULL)
        {
            fprintf (stderr,
                "Failed to create thread: Win _beginthreadex(), %s",
                strerror (errno));
            return -1;
        }
#else
#error "No threading implementation available."
#endif
#endif
    }
    return 0;
}

int lo_server_thread_stop(lo_server_thread st)
{
    int result;

    if (st->active) {
        // Signal thread to stop
        st->active = 0;

#ifdef HAVE_LIBPTHREAD
        // pthread_join waits for thread to terminate
        // and then releases the thread's resources
        result = pthread_join(st->thread, NULL);

        if (result) {
            fprintf(stderr, "Failed to stop thread: pthread_join(), %s",
                    strerror(result));
            return -result;
        }
#else
#ifdef HAVE_WIN32_THREADS
        result = WaitForSingleObject (st->thread, INFINITE);
        CloseHandle (st->thread);
        st->thread = NULL;

        if (result != 0)
        {
            fprintf (stderr, "Failed to join thread: waitForSingleObject(), %d",
                result);
            return -1;
        }
#else
#error "No threading implementation available."
#endif
#endif
    }

    return 0;
}

int lo_server_thread_get_port(lo_server_thread st)
{
    return lo_server_get_port(st->s);
}

char *lo_server_thread_get_url(lo_server_thread st)
{
    return lo_server_get_url(st->s);
}

lo_server lo_server_thread_get_server(lo_server_thread st)
{
    return st->s;
}

int lo_server_thread_events_pending(lo_server_thread st)
{
    return lo_server_events_pending(st->s);
}

#ifdef HAVE_WIN32_THREADS
static unsigned __stdcall thread_func(void *data)
#else
static void* thread_func(void *data)
#endif
{
    lo_server_thread st = (lo_server_thread) data;

    if (st->cb_init) {
        if ( (st->cb_init)(st, st->user_data) )
        {
            st->done = 1;
#ifdef HAVE_LIBPTHREAD
            pthread_exit (NULL);
#else
#ifdef HAVE_WIN32_THREADS
            _endthread ();
#else
#error "No threading implementation selected."
#endif
#endif
            return 0;
        }
    }

    while (st->active) {
        lo_server_recv_noblock(st->s, 10);
    }
    st->done = 1;

    if (st->cb_cleanup) {
        (st->cb_cleanup)(st, st->user_data);
    }

#ifdef HAVE_LIBPTHREAD
    pthread_exit(NULL);
#else
#ifdef HAVE_WIN32_THREADS
    _endthread ();
#else
#error "No threading implementation selected."
#endif
#endif
    return 0;
}

void lo_server_thread_pp(lo_server_thread st)
{
    lo_server_pp(st->s);
}

/* vi:set ts=8 sts=4 sw=4: */
