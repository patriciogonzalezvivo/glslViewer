#ifndef LO_TYPES_H
#define LO_TYPES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef HAVE_POLL
#include <poll.h>
#endif

#if defined(WIN32) || defined(_MSC_VER)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#define closesocket close
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef _MSC_VER
typedef SSIZE_T ssize_t;
#endif

#ifndef UINT_PTR
  #ifdef HAVE_UINTPTR_T
    #include <stdint.h>
    #define UINT_PTR uintptr_t
  #else
    #define UINT_PTR unsigned long
  #endif
#endif

#ifdef ENABLE_THREADS
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif
#endif

#include "lo/lo_osc_types.h"

#define LO_HOST_SIZE 1024

/** \brief Bitflags for optional protocol features, set by
 *         lo_address_set_flags(). */
typedef enum {
    LO_SLIP=0x01,     /*!< SLIP decoding */
    LO_NODELAY=0x02,  /*!< Set the TCP_NODELAY socket option. */
} lo_proto_flags;

/** \brief Bitflags for optional server features. */
typedef enum {
    LO_SERVER_COERCE=0x01,     /*!< whether or not to coerce args
                                * during dispatch */
    LO_SERVER_ENQUEUE=0x02,    /*!< whether or not to enqueue early
                                * messages */
} lo_server_flags;

#define LO_SERVER_DEFAULT_FLAGS (LO_SERVER_COERCE | LO_SERVER_ENQUEUE)

typedef void (*lo_err_handler) (int num, const char *msg,
                                const char *path);

struct _lo_method;

typedef struct _lo_inaddr {
    union {
        struct in_addr addr;
        struct in6_addr addr6;
    } a;
    size_t size;
    char *iface;
} *lo_inaddr;

typedef struct _lo_address {
    char *host;
    int socket;
    int ownsocket;
    char *port;
    int protocol;
    lo_proto_flags flags;
    struct addrinfo *ai;
    struct addrinfo *ai_first;
    int errnum;
    const char *errstr;
    int ttl;
    struct _lo_inaddr addr;
    struct _lo_server *source_server;
    const char *source_path; /* does not need to be freed since it
                              * will always point to stack memory in
                              * dispatch_method() */
} *lo_address;

typedef struct _lo_blob {
    uint32_t size;
    char *data;
} *lo_blob;

typedef struct _lo_message {
    char *types;
    size_t typelen;
    size_t typesize;
    void *data;
    size_t datalen;
    size_t datasize;
    lo_address source;
    lo_arg **argv;
    /* timestamp from bundle (LO_TT_IMMEDIATE for unbundled messages) */
    lo_timetag ts;
    int refcount;
} *lo_message;

typedef int (*lo_method_handler) (const char *path, const char *types,
                                  lo_arg ** argv, int argc,
                                  struct _lo_message * msg,
                                  void *user_data);

typedef int (*lo_bundle_start_handler) (lo_timetag time, void *user_data);
typedef int (*lo_bundle_end_handler) (void *user_data);

typedef struct _lo_method {
    const char *path;
    const char *typespec;
    lo_method_handler handler;
    char *user_data;
    struct _lo_method *next;
} *lo_method;

struct socket_context {
    char *buffer;
    size_t buffer_size;
    unsigned int buffer_msg_offset;
    unsigned int buffer_read_offset;
    int is_slip;                        //<! 1 if slip mode, 0 otherwise, -1 for unknown
    int slip_state;                     //<! state variable for slip decoding
};

#ifdef HAVE_POLL
    typedef struct pollfd lo_server_fd_type;
#else
    typedef struct { int fd; } lo_server_fd_type;
#endif

typedef struct _lo_server {
    struct addrinfo *ai;
    lo_method first;
    lo_err_handler err_h;
    int port;
    char *hostname;
    char *path;
    int protocol;
    lo_server_flags flags;
    void *queued;
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int sockets_len;
    int sockets_alloc;
    lo_server_fd_type *sockets;

    // Some extra data needed per open socket.  Note that we don't put
    // it in the socket struct, because that layout is needed for
    // passing in the list of sockets to poll().
    struct socket_context *contexts;

    struct _lo_address *sources;
    int sources_len;
    lo_bundle_start_handler bundle_start_handler;
    lo_bundle_end_handler bundle_end_handler;
    void *bundle_handler_user_data;
    struct _lo_inaddr addr_if;
    void *error_user_data;
    int max_msg_size;
} *lo_server;

#ifdef ENABLE_THREADS
struct _lo_server_thread;
typedef int (*lo_server_thread_init_callback)(struct _lo_server_thread *s,
                                              void *user_data);
typedef void (*lo_server_thread_cleanup_callback)(struct _lo_server_thread *s,
                                                  void *user_data);
typedef struct _lo_server_thread {
    lo_server s;
#ifdef HAVE_LIBPTHREAD
    pthread_t thread;
#else
#ifdef HAVE_WIN32_THREADS
    HANDLE thread;
#endif
#endif
    volatile int active;
    volatile int done;
    lo_server_thread_init_callback cb_init;
    lo_server_thread_cleanup_callback cb_cleanup;
    void *user_data;
} *lo_server_thread;
#else
typedef void *lo_server_thread;
#endif

typedef struct _lo_bundle *lo_bundle;

typedef struct _lo_element {
    lo_element_type type;
    union {
        lo_bundle bundle;
        struct {
            lo_message msg;
            const char *path;
        } message;
    } content;
} lo_element;

struct _lo_bundle {
    size_t size;
    size_t len;
    lo_timetag ts;
    lo_element *elmnts;
    int refcount;
};

typedef struct _lo_strlist {
    char *str;
    struct _lo_strlist *next;
} lo_strlist;

typedef union {
    int32_t i;
    float f;
    char c;
    uint32_t nl;
} lo_pcast32;

typedef union {
    int64_t i;
    double f;
    uint64_t nl;
} lo_pcast64;

extern struct lo_cs {
    int udp;
    int tcp;
} lo_client_sockets;

#endif
