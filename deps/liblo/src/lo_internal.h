#ifndef LO_INTERNAL_H
#define LO_INTERNAL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <lo/lo_osc_types.h>

/**
 * \brief Validate raw OSC string data. Where applicable, data should be
 * in network byte order.
 *
 * This function is used internally to parse and validate raw OSC data.
 *
 * Returns length of string or < 0 if data is invalid.
 *
 * \param data      A pointer to the data.
 * \param size      The size of data in bytes (total bytes remaining).
 */
ssize_t lo_validate_string(void *data, ssize_t size);

/**
 * \brief Validate raw OSC blob data. Where applicable, data should be
 * in network byte order.
 *
 * This function is used internally to parse and validate raw OSC data.
 *
 * Returns length of blob or < 0 if data is invalid.
 *
 * \param data      A pointer to the data.
 * \param size      The size of data in bytes (total bytes remaining).
 */
ssize_t lo_validate_blob(void *data, ssize_t size);

/**
 * \brief Validate raw OSC bundle data. Where applicable, data should be
 * in network byte order.
 *
 * This function is used internally to parse and validate raw OSC data.
 *
 * Returns length of bundle or < 0 if data is invalid.
 *
 * \param data      A pointer to the data.
 * \param size      The size of data in bytes (total bytes remaining).
 */
ssize_t lo_validate_bundle(void *data, ssize_t size);

/**
 * \brief Validate raw OSC argument data. Where applicable, data should be
 * in network byte order.
 *
 * This function is used internally to parse and validate raw OSC data.
 *
 * Returns length of argument data or < 0 if data is invalid.
 *
 * \param type      The OSC type of the data item (eg. LO_FLOAT).
 * \param data      A pointer to the data.
 * \param size      The size of data in bytes (total bytes remaining).
 */
ssize_t lo_validate_arg(lo_type type, void *data, ssize_t size);

int lo_address_resolve(lo_address a);

/**
 * \internal \brief Look up a given interface by name or by IP and
 * store the found information in a lo_inaddr.  Usually either iface
 * or ip will be zero, but not both.
 *
 * \param t Location to store interface information.
 * \param fam Family, either AF_INET or AF_INET6.
 * \param iface The interface to look for by name.
 * \param ip The IP to find an interface for.
 */
int lo_inaddr_find_iface(lo_inaddr t, int fam,
                         const char *iface, const char *ip);

/** \internal \brief Add a socket to this server's list of sockets.
 *  \param s The lo_server
 *  \param socket The socket number to add.
 *  \return The index number of the added socket, or -1 on failure.
 */
int lo_server_add_socket(lo_server s, int socket, lo_address a,
                                struct sockaddr_storage *addr,
                                socklen_t addr_len);

/** \internal \brief Delete a socket from this server's list of sockets.
 *  \param s The lo_server
 *  \param index The index of the socket to delete, -1 if socket is provided.
 *  \param socket The socket number to delete, -1 if index is provided.
 *  \return The index number of the added socket.
 */
void lo_server_del_socket(lo_server s, int index, int socket);

/** \internal \brief Copy a lo_address into pre-allocated memory. */
void lo_address_copy(lo_address to, lo_address from);

/** \internal \brief Initialize a pre-allocated lo_address from a
 * sockaddr. */
void lo_address_init_with_sockaddr(lo_address a,
                                   void *sa, size_t sa_len,
                                   int sock, int prot);

/** \internal \brief Free memory owned by an address, without freeing
 * actual lo_address structure. */
void lo_address_free_mem(lo_address a);

#endif
