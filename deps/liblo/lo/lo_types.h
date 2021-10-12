/*
 *  Copyright (C) 2014 Steve Harris et al. (see AUTHORS)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  $Id$
 */

#ifndef LO_TYPES_H
#define LO_TYPES_H

/**
 * \file lo_types.h The liblo headerfile defining types used by this API.
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(_MSC_VER)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

#include "lo/lo_osc_types.h"

#define LO_DISABLE 0  //!< Disable a boolean option.
#define LO_ENABLE 1   //!< Enable a boolean option.

/**
 * \brief A reference to an OSC service.
 *
 * Created by calls to lo_address_new() or lo_address_new_from_url().
 */
typedef void *lo_address;

/**
 * \brief A object to store an opaque binary data object.
 *
 * Can be passed over OSC using the 'b' type. Created by calls to lo_blob_new().
 */
typedef void *lo_blob;

/**
 * \brief A low-level object used to represent messages passed over OSC.
 *
 * Created by calls to lo_message_new(), arguments can be added with calls to
 * lo_message_add_*().
 */
typedef void *lo_message;

/**
 * \brief A low-level object used to represent bundles of messages passed over
 * OSC.
 *
 * Created by calls to lo_bundle_new(), messages can be added with calls to
 * lo_bundle_add_message().
 */
typedef void *lo_bundle;

/**
 * \brief An object representing an method on a server.
 *
 * Returned by calls to lo_server_thread_add_method() and
 * lo_server_add_method().
 */
typedef void *lo_method;

/**
 * \brief An object representing an instance of an OSC server.
 *
 * Created by calls to lo_server_new(). If you wish to have the server
 * operate in a background thread, use lo_server_thread instead.
 */
typedef void *lo_server;

/**
 * \brief An object representing a thread containing an OSC server.
 *
 * Created by calls to lo_server_thread_new().
 */
typedef void *lo_server_thread;

/**
 * \brief A callback function to receive notification of an error in a server or
 * server thread.
 *
 * On callback the parameters will be set to the following values:
 *
 * \param num An error number that can be used to identify this condition.
 * \param msg An error message describing the condidtion.
 * \param where A string describing the place the error occured - typically
 * either a function call or method path.
 */
typedef void (*lo_err_handler)(int num, const char *msg, const char *where);

/**
 * \brief A callback function to receive notification of matching message
 * arriving in the server or server thread.
 *
 * The return value tells the method dispatcher whether this handler
 * has dealt with the message correctly: a return value of 0 indicates
 * that it has been handled, and it should not attempt to pass it on
 * to any other handlers, non-0 means that it has not been handled and
 * the dispatcher will attempt to find more handlers that match the
 * path and types of the incoming message.
 *
 * On callback the paramters will be set to the following values:
 *
 * \param path That path that the incoming message was sent to
 * \param types If you specided types in your method creation call then this
 * will match those and the incoming types will have been coerced to match,
 * otherwise it will be the types of the arguments of the incoming message.
 * \param argv An array of lo_arg types containing the values, e.g. if the
 * first argument of the incoming message is of type 'f' then the value will be
 * found in argv[0]->f.
 * \param argc The number of arguments received.
 * \param msg A structure containing the original raw message as received. No
 * type coercion will have occured and the data will be in OSC byte order
 * (bigendian).
 * \param user_data This contains the user_data value passed in the call to
 * lo_server_thread_add_method.
 */
typedef int (*lo_method_handler)(const char *path, const char *types,
				 lo_arg **argv, int argc, lo_message msg,
				 void *user_data);

/**
 * \brief A callback function to receive notification of a bundle being
 * dispatched by the server or server thread.
 *
 * This callback allows applications to be aware of incoming bundles
 * and preserve ordering and atomicity of messages in bundles.
 *
 * If installed with lo_server_add_bundle_handlers, this callback will be
 * called with \a time set to the time tag of the bundle, and \a user_data
 * set to the user_data parameter passed to lo_server_add_bundle_handlers.
 *
 * Note that bundles may be nested, in which case calls to the bundle start
 * and end handlers will also be nested.  The application can keep track of
 * nested bundles in a stack-like manner by treating the start handler as
 * "push" and the end handler as "pop".  For example, a bundle containing two
 * bundles would fire 6 callbacks: begin, begin, end, begin, end, end.
 */
typedef int (*lo_bundle_start_handler)(lo_timetag time, void *user_data);

/**
 * \brief A callback function to receive notification of a bundle dispatch
 * being completed by the server or server thread.
 *
 * If installed with lo_server_add_bundle_handlers, this callback will be
 * called after all the messages of a bundle have been dispatched with
 * \a user_data set to the user_data parameter passed to
 * lo_server_add_bundle_handlers.
 */
typedef int (*lo_bundle_end_handler)(void *user_data);

/**
 * \brief A callback function to perform initialization when the
 * server thread is started.
 *
 * If installed with lo_server_thread_set_callbacks, this callback
 * will be called in the server thread, just before the server starts
 * listening for events. \a user_data is set to the user_data
 * parameter passed to lo_server_thread_add_functions.
 *
 * If the return value is non-zero, the thread start will be aborted.
 */
typedef int (*lo_server_thread_init_callback)(lo_server_thread s,
                                              void *user_data);

/**
 * \brief A callback function to perform cleanup when the server
 * thread is started.
 *
 * If installed with lo_server_thread_set_callbacks, this callback
 * will be called in the server thread, after the server have stopped
 * listening and processing events, before it quits. \a user_data is
 * set to the user_data parameter passed to
 * lo_server_thread_add_functions.
 */
typedef void (*lo_server_thread_cleanup_callback)(lo_server_thread s,
                                                  void *user_data);

#ifdef __cplusplus
}
#endif

#endif
