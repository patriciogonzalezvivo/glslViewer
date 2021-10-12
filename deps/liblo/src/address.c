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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if defined(WIN32) || defined(_MSC_VER)
#include <io.h>
#define snprintf _snprintf
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>
#endif
#endif

#include "lo_types_internal.h"
#include "lo_internal.h"
#include "lo/lo.h"
#include "lo/lo_throw.h"

static void lo_address_set_flags(lo_address t, int flags);

lo_address lo_address_new_with_proto(int proto, const char *host,
                                     const char *port)
{
    lo_address a;

    if (proto != LO_UDP && proto != LO_TCP && proto != LO_UNIX)
        return NULL;

    a = (lo_address) calloc(1, sizeof(struct _lo_address));
    if (a == NULL)
        return NULL;

    a->ai = NULL;
    a->ai_first = NULL;
    a->socket = -1;
    a->ownsocket = 1;
    a->protocol = proto;
    a->flags = (lo_proto_flags) 0;
    switch (proto) {
    default:
    case LO_UDP:
    case LO_TCP:
        if (host) {
            a->host = strdup(host);
        } else {
            a->host = strdup("localhost");
        }
        break;
    case LO_UNIX:
        a->host = strdup("localhost");
        break;
    }
    if (port) {
        a->port = strdup(port);
    } else {
        a->port = NULL;
    }

    a->ttl = -1;
    a->addr.size = 0;
    a->addr.iface = 0;
    a->source_server = 0;
    a->source_path = 0;

    return a;
}

lo_address lo_address_new(const char *host, const char *port)
{
    return lo_address_new_with_proto(LO_UDP, host, port);
}

lo_address lo_address_new_from_url(const char *url)
{
    lo_address a;
    int protocol;
    char *host, *port, *proto;

    if (!url || !*url) {
        return NULL;
    }

    protocol = lo_url_get_protocol_id(url);
    if (protocol == LO_UDP || protocol == LO_TCP) {
        host = lo_url_get_hostname(url);
        port = lo_url_get_port(url);
        a = lo_address_new_with_proto(protocol, host, port);
        if (host)
            free(host);
        if (port)
            free(port);
#if !defined(WIN32) && !defined(_MSC_VER)
    } else if (protocol == LO_UNIX) {
        port = lo_url_get_path(url);
        a = lo_address_new_with_proto(LO_UNIX, NULL, port);
        if (port)
            free(port);
#endif
    } else {
        proto = lo_url_get_protocol(url);
        fprintf(stderr,
                PACKAGE_NAME ": protocol '%s' not supported by this "
                "version\n", proto);
        if (proto)
            free(proto);

        return NULL;
    }

    return a;
}

static void lo_address_resolve_source(lo_address a)
{
    char hostname[LO_HOST_SIZE];
    char portname[32];
    int err;
    lo_server s = a->source_server;

    if (a->protocol == LO_UDP && s && s->addr_len > 0)
    {
        err = getnameinfo((struct sockaddr *) &s->addr, s->addr_len,
                          hostname, sizeof(hostname),
                          portname, sizeof(portname),
                          NI_NUMERICHOST | NI_NUMERICSERV);
        if (err) {
            switch (err) {
            case EAI_AGAIN:
                lo_throw(s, err, "Try again", a->source_path);
                break;
            case EAI_BADFLAGS:
                lo_throw(s, err, "Bad flags", a->source_path);
                break;
            case EAI_FAIL:
                lo_throw(s, err, "Failed", a->source_path);
                break;
            case EAI_FAMILY:
                lo_throw(s, err, "Cannot resolve address family",
                         a->source_path);
                break;
            case EAI_MEMORY:
                lo_throw(s, err, "Out of memory", a->source_path);
                break;
            case EAI_NONAME:
                lo_throw(s, err, "Cannot resolve", a->source_path);
                break;
#if !defined(WIN32) && !defined(_MSC_VER)
            case EAI_SYSTEM:
                lo_throw(s, err, strerror(err), a->source_path);
                break;
#endif
            default:
                lo_throw(s, err, "Unknown error", a->source_path);
                break;
            }

            return;
        }

        a->host = strdup(hostname);
        a->port = strdup(portname);
    } else {
        a->host = strdup("");
        a->port = strdup("");
    }
}

const char *lo_address_get_hostname(lo_address a)
{
    if (!a) {
        return NULL;
    }

    if (!a->host)
        lo_address_resolve_source(a);

    return a->host;
}

int lo_address_get_protocol(lo_address a)
{
    if (!a) {
        return -1;
    }

    return a->protocol;
}

const char *lo_address_get_port(lo_address a)
{
    if (!a) {
        return NULL;
    }

    if (!a->host)
        lo_address_resolve_source(a);

    return a->port;
}

static const char *get_protocol_name(int proto)
{
    switch (proto) {
    case LO_UDP:
        return "udp";
    case LO_TCP:
        return "tcp";
#if !defined(WIN32) && !defined(_MSC_VER)
    case LO_UNIX:
        return "unix";
#endif
    }
    return NULL;
}


char *lo_address_get_url(lo_address a)
{
    char *buf;
    int ret = 0;
    int needquote = 0;
    const char *fmt;

    if (!a->host)
        lo_address_resolve_source(a);

    if (!a->host)
		return NULL;

    needquote = strchr(a->host, ':') ? 1 : 0;

    if (needquote) {
        fmt = "osc.%s://[%s]:%s/";
    } else {
        fmt = "osc.%s://%s:%s/";
    }
#ifndef _MSC_VER
    ret = snprintf(NULL, 0, fmt,
                   get_protocol_name(a->protocol), a->host, a->port);
#endif
    if (ret <= 0) {
        /* this libc is not C99 compliant, guess a size */
        ret = 1023;
    }
    buf = (char*) malloc((ret + 2) * sizeof(char));
    snprintf(buf, ret + 1, fmt,
             get_protocol_name(a->protocol), a->host, a->port);

    if (a->protocol==LO_UNIX) {
        buf[ret-1] = 0;
    }

    return buf;
}

void lo_address_free(lo_address a)
{
    if (a) {
        if (a->socket != -1 && a->ownsocket) {
#ifdef SHUT_WR
            shutdown(a->socket, SHUT_WR);
#endif
            closesocket(a->socket);
        }
        lo_address_free_mem(a);
        free(a);
    }
}

void lo_address_free_mem(lo_address a)
{
    if (a) {
        if (a->host)
            free(a->host);
        if (a->port)
            free(a->port);
        if (a->ai_first)
            freeaddrinfo(a->ai_first);
        if (a->addr.iface)
            free(a->addr.iface);

        memset(a, 0, sizeof(struct _lo_address));
        a->socket = -1;
    }
}

int lo_address_errno(lo_address a)
{
    return a->errnum;
}

const char *lo_address_errstr(lo_address a)
{
    char *msg;

    if (a->errstr) {
        return a->errstr;
    }

    if (a->errnum == 0)
        return "Success";

    msg = strerror(a->errnum);
    if (msg) {
        return msg;
    } else {
        return "unknown error";
    }

    return "unknown error";
}

char *lo_url_get_protocol(const char *url)
{
    char *protocol, *ret;

    if (!url) {
        return NULL;
    }

    protocol = (char*) malloc(strlen(url));

    if (sscanf(url, "osc://%s", protocol)) {
        fprintf(stderr,
                PACKAGE_NAME " warning: no protocol specified in URL, "
                "assuming UDP.\n");
        ret = strdup("udp");
    } else if (sscanf(url, "osc.%[^:/[]", protocol)) {
        ret = strdup(protocol);
    } else {
        ret = NULL;
    }

    free(protocol);

    return ret;
}

int lo_url_get_protocol_id(const char *url)
{
    if (!url) {
        return -1;
    }

    if (!strncmp(url, "osc:", 4)) {
        fprintf(stderr,
                PACKAGE_NAME " warning: no protocol specified in URL, "
                "assuming UDP.\n");
        return LO_UDP;          // should be LO_DEFAULT?
    } else if (!strncmp(url, "osc.udp:", 8)) {
        return LO_UDP;
    } else if (!strncmp(url, "osc.tcp:", 8)) {
        return LO_TCP;
    } else if (!strncmp(url, "osc.unix:", 9)) {
        return LO_UNIX;
    }
    return -1;
}

char *lo_url_get_hostname(const char *url)
{
    char *hostname = (char*) malloc(strlen(url));

    if (sscanf(url, "osc://%[^[:/]", hostname)) {
        return hostname;
    }
    if (sscanf(url, "osc.%*[^:/]://[%[^]/]]", hostname)) {
        return hostname;
    }
    if (sscanf(url, "osc.%*[^:/]://%[^[:/]", hostname)) {
        return hostname;
    }

    /* doesnt look like an OSC URL */
    free(hostname);

    return NULL;
}

char *lo_url_get_port(const char *url)
{
    char *port = (char*) malloc(strlen(url));

    if (sscanf(url, "osc://%*[^:]:%[0-9]", port)) {
        return port;
    }
    if (sscanf(url, "osc.%*[^:]://%*[^:]:%[0-9]", port)) {
        return port;
    }
    if (sscanf(url, "osc://[%*[^]]]:%[0-9]", port)) {
        return port;
    }
    if (sscanf(url, "osc.%*[^:]://[%*[^]]]:%[0-9]", port)) {
        return port;
    }
    if (sscanf(url, "osc://:%[0-9]", port)) {
        return port;
    }
    if (sscanf(url, "osc.%*[^:]://:%[0-9]", port)) {
        return port;
    }

    /* doesnt look like an OSC URL with port number */
    free(port);

    return NULL;
}

char *lo_url_get_path(const char *url)
{
    char *path = (char*) malloc(strlen(url));

    if (sscanf(url, "osc://%*[^:]:%*[0-9]%s", path)) {
        return path;
    }
    if (sscanf(url, "osc.%*[^:]://%*[^:]:%*[0-9]%s", path) == 1) {
        return path;
    }
    if (sscanf(url, "osc.unix://%*[^/]%s", path)) {
        int i = strlen(path)-1;
        if (path[i]=='/') // remove trailing slash
            path[i] = 0;
        return path;
    }
    if (sscanf(url, "osc.%*[^:]://%s", path)) {
        int i = strlen(path)-1;
        if (path[i]=='/') // remove trailing slash
            path[i] = 0;
        return path;
    }

    /* doesnt look like an OSC URL with port number and path */
    free(path);

    return NULL;
}

void lo_address_set_ttl(lo_address t, int ttl)
{
    if (t->protocol == LO_UDP)
        t->ttl = ttl;
}

int lo_address_get_ttl(lo_address t)
{
    return t->ttl;
}

int lo_address_set_tcp_nodelay(lo_address t, int enable)
{
    int r = (t->flags & LO_NODELAY) != 0;
    lo_address_set_flags(t, enable
                         ? t->flags | LO_NODELAY
                         : t->flags & ~LO_NODELAY);
    return r;
}

int lo_address_set_stream_slip(lo_address t, int enable)
{
    int r = (t->flags & LO_SLIP) != 0;
    lo_address_set_flags(t, enable
                         ? t->flags | LO_SLIP
                         : t->flags & ~LO_SLIP);
    return r;
}

static
void lo_address_set_flags(lo_address t, int flags)
{
    if (((t->flags & LO_NODELAY) != (flags & LO_NODELAY))
        && t->socket > 0)
    {
        int option = (t->flags & LO_NODELAY)!=0;
        setsockopt(t->socket, IPPROTO_TCP, TCP_NODELAY,
                   (const char*)&option, sizeof(option));
    }

    t->flags = (lo_proto_flags) flags;
}

#ifdef ENABLE_IPV6
static int is_dotted_ipv4_address (const char* address)
{
    int a[4];
    return sscanf(address, "%u.%u.%u.%u", &a[0], &a[1], &a[2], &a[3]);
}
#endif

void lo_address_copy(lo_address to, lo_address from)
{
    /* Initialize all members that are not auto-initialized when the
     * lo_address is used. (e.g. resolving addrinfo) */
    memset(to, 0, sizeof(struct _lo_address));
    to->socket = from->socket;
    if (from->host) {
        free(to->host);
        to->host = strdup(from->host);
    }
    if (from->port) {
        free(to->port);
        to->port = strdup(from->port);
    }
    to->protocol = from->protocol;
    to->ttl = from->ttl;
    to->addr = from->addr;
    if (from->addr.iface)
        to->addr.iface = strdup(from->addr.iface);
}

void lo_address_init_with_sockaddr(lo_address a,
                                   void *sa, size_t sa_len,
                                   int sock, int prot)
{
    int err = 0;
    assert(a != NULL);
    lo_address_free_mem(a);
    a->host = (char*) malloc(INET_ADDRSTRLEN);
    a->port = (char*) malloc(8);

    err = getnameinfo((struct sockaddr *)sa, sa_len,
                      a->host, INET_ADDRSTRLEN, a->port, 8,
                      NI_NUMERICHOST | NI_NUMERICSERV);

    if (err) {
        free(a->host);
        free(a->port);
        a->host = a->port = 0;
    }

    a->socket = sock;
    a->protocol = prot;
}

int lo_address_resolve(lo_address a)
{
    int ret;

    if (a->protocol == LO_UDP || a->protocol == LO_TCP) {
        struct addrinfo *ai;
        struct addrinfo hints;
        const char* host = lo_address_get_hostname(a);
#ifdef ENABLE_IPV6
        char hosttmp[7+16+1]; // room for ipv6 prefix + a dotted quad
#endif

        memset(&hints, 0, sizeof(hints));
#ifdef ENABLE_IPV6
        hints.ai_family = PF_UNSPEC;

        if (is_dotted_ipv4_address(host)) {
            host = hosttmp;
            strcpy(hosttmp, "::FFFF:");
            strncpy(hosttmp + 7, lo_address_get_hostname(a), 16);
        }
#else
        hints.ai_family = PF_INET;
#endif
        hints.ai_socktype =
            a->protocol == LO_UDP ? SOCK_DGRAM : SOCK_STREAM;

        if ((ret = getaddrinfo(host, lo_address_get_port(a), &hints, &ai))) {
            a->errnum = ret;
            a->errstr = gai_strerror(ret);
            a->ai = NULL;
            a->ai_first = NULL;
            return -1;
        }
        
        a->ai = ai;
        a->ai_first = ai;
    }

    return 0;
}

#if defined(WIN32) || defined(_MSC_VER) || defined(HAVE_GETIFADDRS)

int lo_address_set_iface(lo_address t, const char *iface, const char *ip)
{
    int fam;
    if (!t->ai) {
        lo_address_resolve(t);
        if (!t->ai)
            return 2;  // Need the address family to continue
    }
    fam = t->ai->ai_family;

    return lo_inaddr_find_iface(&t->addr, fam, iface, ip);
}

int lo_inaddr_find_iface(lo_inaddr t, int fam,
                         const char *iface, const char *ip)
{
#if defined(WIN32) || defined(_MSC_VER)
    ULONG size;
    int tries;
    PIP_ADAPTER_ADDRESSES paa, aa;
    DWORD rc;
    int found;
#endif

	union {
        struct in_addr addr;
#ifdef ENABLE_IPV6
        struct in6_addr addr6;
#endif
    } a;

    if (ip) {
#ifdef HAVE_INET_PTON
        int rc = inet_pton(fam, ip, &a);
        if (rc!=1)
            return (rc<0) ? 3 : 4;
#else
        if (fam!=AF_INET6)
            *((unsigned long*)&a.addr) = inet_addr(ip);
#endif
    }

#if defined(WIN32) || defined(_MSC_VER)

    /* Start with recommended 15k buffer for GetAdaptersAddresses. */
    size = 15*1024/2;
    tries = 3;
    paa = malloc(size*2);
    rc = ERROR_SUCCESS-1;
    while (rc!=ERROR_SUCCESS && paa && tries-- > 0) {
        size *= 2;
        paa = realloc(paa, size);
        rc = GetAdaptersAddresses(fam, 0, 0, paa, &size);
    }
    if (rc!=ERROR_SUCCESS)
        return 2;

    aa = paa;
    found=0;
    while (aa && rc==ERROR_SUCCESS) {
        if (iface) {
            if (strcmp(iface, aa->AdapterName)==0)
                found = 1;
            else {
				WCHAR ifaceW[256];
				MultiByteToWideChar(CP_ACP, 0, iface, strlen(iface),
									ifaceW, 256);
				if (lstrcmpW(ifaceW, aa->FriendlyName)==0)
					found = 1;
			}
        }
        if (ip) {
            PIP_ADAPTER_UNICAST_ADDRESS pua = aa->FirstUnicastAddress;
            while (pua && !found) {
                if (fam==AF_INET) {
                    struct sockaddr_in *s =
                        (struct sockaddr_in*)pua->Address.lpSockaddr;
                    if (fam == s->sin_family
                        && memcmp(&a.addr, &s->sin_addr,
                                  sizeof(struct in_addr))==0) {
                        memcpy(&t->a.addr, &s->sin_addr,
                               sizeof(struct in_addr));
                        found = 1;
                    }
                }
#ifdef ENABLE_IPV6
                else if (fam==AF_INET6) {
                    struct sockaddr_in6 *s =
                        (struct sockaddr_in6*)pua->Address.lpSockaddr;
                    if (fam == s->sin6_family
                        && memcmp(&a.addr6, &s->sin6_addr,
                                  sizeof(struct in6_addr))==0) {
                        memcpy(&t->a.addr6, &s->sin6_addr,
                               sizeof(struct in6_addr));
                        found = 1;
                    }
                }
#endif
                pua = pua->Next;
            }
        }

        if (aa && found) {
            t->iface = strdup(aa->AdapterName);
            if (!ip && aa->FirstUnicastAddress) {
                PIP_ADAPTER_UNICAST_ADDRESS pua = aa->FirstUnicastAddress;
                while (pua) {
                    struct sockaddr_in *s =
                        (struct sockaddr_in*)pua->Address.lpSockaddr;
                    if (s->sin_family==fam) {
                        if (fam==AF_INET) {
                            memcpy(&t->a.addr, &s->sin_addr,
                                   sizeof(struct in_addr));
                            break;
                        }
#ifdef ENABLE_IPV6
                        else if (fam==AF_INET6) {
                            struct sockaddr_in6 *s6 =
                                (struct sockaddr_in6*)pua->Address.lpSockaddr;
                            memcpy(&t->a.addr6, &s6->sin6_addr,
                                   sizeof(struct in6_addr));
                            break;
                        }
#endif
                    }
                    pua = pua->Next;
                }
            }
            break;
        }

        aa = aa->Next;
    }

    if (paa) free(paa);

    return !found;

#else // !WIN32

    struct ifaddrs *ifa, *ifa_list;
    if (getifaddrs(&ifa_list)==-1)
        return 5;
    ifa = ifa_list;

    int found = 0;
    while (ifa) {
        if (!ifa->ifa_addr) {
            ifa = ifa->ifa_next;
            continue;
        }
        if (ip) {
            if (ifa->ifa_addr->sa_family == AF_INET && fam == AF_INET)
            {
                if (memcmp(&((struct sockaddr_in*)ifa->ifa_addr)->sin_addr,
                           &a.addr, sizeof(struct in_addr))==0) {
                    found = 1;
                    t->size = sizeof(struct in_addr);
                    memcpy(&t->a, &a, t->size);
                    break;
                }
            }
#ifdef ENABLE_IPV6
            else if (ifa->ifa_addr->sa_family == AF_INET6 && fam == AF_INET6)
            {
                if (memcmp(&((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr,
                           &a.addr6, sizeof(struct in6_addr))==0) {
                    found = 1;
                    t->size = sizeof(struct in6_addr);
                    memcpy(&t->a, &a, t->size);
                    break;
                }
            }
#endif
        }
        if (iface) {
            if (ifa->ifa_addr->sa_family == fam
                && strcmp(ifa->ifa_name, iface)==0)
            {
                if (fam==AF_INET) {
                    found = 1;
                    t->size = sizeof(struct in_addr);
                    memcpy(&t->a, &((struct sockaddr_in*)
                                    ifa->ifa_addr)->sin_addr,
                           t->size);
                    break;
                }
#ifdef ENABLE_IPV6
                else if (fam==AF_INET6) {
                    found = 1;
                    t->size = sizeof(struct in6_addr);
                    memcpy(&t->a, &((struct sockaddr_in6*)
                                    ifa->ifa_addr)->sin6_addr,
                           t->size);
                    break;
                }
#endif
            }
        }
        ifa = ifa->ifa_next;
    }

    if (found && ifa->ifa_name) {
        if (t->iface) free(t->iface);
        t->iface = strdup(ifa->ifa_name);
    }

    freeifaddrs(ifa_list);
    return !found;
#endif
}

const char* lo_address_get_iface(lo_address t)
{
    if (t)
        return t->addr.iface;
    return 0;
}

#endif // HAVE_GETIFADDRS

/* vi:set ts=8 sts=4 sw=4: */
