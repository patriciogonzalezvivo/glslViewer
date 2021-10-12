/*
 * oscsend - Send OpenSound Control message.
 *
 * Copyright (C) 2008 Kentaro Fukuchi <kentaro@fukuchi.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * TODO:
 * - support binary blob.
 * - support TimeTag.
 * - receive replies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <config.h>
#include <limits.h>
#include <lo/lo.h>

void usage(void)
{
    printf("oscsend version %s\n"
           "Copyright (C) 2008 Kentaro Fukuchi\n\n"
           "Usage: oscsend hostname port address types values...\n"
           "or     oscsend url address types values...\n"
           "Send OpenSound Control message via UDP.\n\n"
           "Description\n"
           "hostname: specifies the remote host's name.\n"
           "port    : specifies the port number to connect to the remote host.\n"
           "url     : specifies the destination parameters using a liblo URL.\n"
           "          e.g. UDP        \"osc.udp://localhost:9000\"\n"
           "               Multicast  \"osc.udp://224.0.1.9:9000\"\n"
           "               TCP        \"osc.tcp://localhost:9000\"\n\n"
           "address : the OSC address where the message to be sent.\n"
           "types   : specifies the types of the following values.\n",
           VERSION);
    printf("          %c - 32bit integer\n", LO_INT32);
    printf("          %c - 64bit integer\n", LO_INT64);
    printf("          %c - 32bit floating point number\n", LO_FLOAT);
    printf("          %c - 64bit (double) floating point number\n",
           LO_DOUBLE);
    printf("          %c - string\n", LO_STRING);
    printf("          %c - symbol\n", LO_SYMBOL);
    printf("          %c - char\n", LO_CHAR);
    printf("          %c - 4 byte midi packet (8 digits hexadecimal)\n",
           LO_MIDI);
    printf("          %c - TRUE (no value required)\n", LO_TRUE);
    printf("          %c - FALSE (no value required)\n", LO_FALSE);
    printf("          %c - NIL (no value required)\n", LO_NIL);
    printf("          %c - INFINITUM (no value required)\n", LO_INFINITUM);
    printf("values  : space separated values.\n\n"
           "Example\n"
           "$ oscsend localhost 7777 /sample/address %c%c%c%c 1 3.14 hello\n",
           LO_INT32, LO_TRUE, LO_FLOAT, LO_STRING);
}

lo_message create_message(char **argv)
{
    /* Note:
     * argv[0] <- types
     * argv[1..] <- values
     */
    int i, argi;
    lo_message message;
    const char *types, *arg;
    int values;

    message = lo_message_new();
    if (argv[0] == NULL) {
        /* empty message is allowed. */
        values = 0;
    } else {
        types = argv[0];
        values = strlen(types);
    }

    argi = 1;
    arg = NULL;
    for (i = 0; i < values; i++) {
		switch(types[i]) {
		case LO_INT32:
		case LO_FLOAT:
		case LO_STRING:
		case LO_BLOB:
		case LO_INT64:
		case LO_TIMETAG:
		case LO_DOUBLE:
		case LO_SYMBOL:
		case LO_CHAR:
		case LO_MIDI:
			arg = argv[argi];
			if (arg == NULL) {
				fprintf(stderr, "Value #%d is not given.\n", i + 1);
				goto EXIT;
			}
			break;
		default:
			break;
		}
        switch (types[i]) {
        case LO_INT32:
            {
                char *endp;
                int64_t v;

                v = strtol(arg, &endp, 10);
                if (*endp != '\0') {
                    fprintf(stderr, "An invalid value was given: '%s'\n",
                            arg);
                    goto EXIT;
                }
                if ((v == LONG_MAX || v == LONG_MIN) && errno == ERANGE) {
                    fprintf(stderr, "Value out of range: '%s'\n", arg);
                    goto EXIT;
                }
                if (v > INT_MAX || v < INT_MIN) {
                    fprintf(stderr, "Value out of range: '%s'\n", arg);
                    goto EXIT;
                }
                lo_message_add_int32(message, (int32_t) v);
                argi++;
                break;
            }
        case LO_INT64:
            {
                char *endp;
                int64_t v;

                v = strtoll(arg, &endp, 10);
                if (*endp != '\0') {
                    fprintf(stderr, "An invalid value was given: '%s'\n",
                            arg);
                    goto EXIT;
                }
                if ((v == LONG_MAX || v == LONG_MIN) && errno == ERANGE) {
                    fprintf(stderr, "Value out of range: '%s'\n", arg);
                    goto EXIT;
                }
                lo_message_add_int64(message, v);
                argi++;
                break;
            }
        case LO_FLOAT:
            {
                char *endp;
                float v;

#ifdef __USE_ISOC99
                v = strtof(arg, &endp);
#else
                v = (float) strtod(arg, &endp);
#endif                          /* __USE_ISOC99 */
                if (*endp != '\0') {
                    fprintf(stderr, "An invalid value was given: '%s'\n",
                            arg);
                    goto EXIT;
                }
                lo_message_add_float(message, v);
                argi++;
                break;
            }
        case LO_DOUBLE:
            {
                char *endp;
                double v;

                v = strtod(arg, &endp);
                if (*endp != '\0') {
                    perror(NULL);
                    fprintf(stderr, "An invalid value was given: '%s'\n",
                            arg);
                    goto EXIT;
                }
                lo_message_add_double(message, v);
                argi++;
                break;
            }
        case LO_STRING:
            lo_message_add_string(message, arg);
            argi++;
            break;
        case LO_SYMBOL:
            lo_message_add_symbol(message, arg);
            argi++;
            break;
        case LO_CHAR:
            lo_message_add_char(message, arg[0]);
            argi++;
            break;
        case LO_MIDI:
            {
                unsigned int midi;
                uint8_t packet[4];
                int ret;

                ret = sscanf(arg, "%08x", &midi);
                if (ret != 1) {
                    fprintf(stderr,
                            "An invalid hexadecimal value was given: '%s'\n",
                            arg);
                    goto EXIT;
                }
                packet[0] = (midi >> 24) & 0xff;
                packet[1] = (midi >> 16) & 0xff;
                packet[2] = (midi >> 8) & 0xff;
                packet[3] = midi & 0xff;
                lo_message_add_midi(message, packet);
                argi++;
                break;
            }
        case LO_TRUE:
            lo_message_add_true(message);
            break;
        case LO_FALSE:
            lo_message_add_false(message);
            break;
        case LO_NIL:
            lo_message_add_nil(message);
            break;
        case LO_INFINITUM:
            lo_message_add_infinitum(message);
            break;
        default:
            fprintf(stderr, "Type '%c' is not supported or invalid.\n",
                    types[i]);
            goto EXIT;
            break;
        }
    }

    return message;
  EXIT:
    lo_message_free(message);
    return NULL;
}

int main(int argc, char **argv)
{
    lo_address target;
    lo_message message;
    int ret, i=1;

    if (argc < 3) {
        usage();
        exit(1);
    }

    if (argv[i] == NULL) {
        fprintf(stderr, "No hostname is given.\n");
        exit(1);
    }

    if (strstr(argv[i], "://")!=0) {
        target = lo_address_new_from_url(argv[i]);
        if (target == NULL) {
            fprintf(stderr, "Failed to open %s\n", argv[i]);
            exit(1);
        }
        i++;
    }
    else if (argv[i+1] == NULL) {
        fprintf(stderr, "No port number is given.\n");
        exit(1);
    }
    else {
        target = lo_address_new(argv[i], argv[i+1]);
        if (target == NULL) {
            fprintf(stderr, "Failed to open %s:%s\n", argv[i], argv[i+1]);
            exit(1);
        }
        i += 2;
    }

    if (argv[i] == NULL) {
        fprintf(stderr, "No path is given.\n");
        exit(1);
    }

    lo_address_set_ttl(target, 1);

    message = create_message(&argv[i+1]);
    if (message == NULL) {
        fprintf(stderr, "Failed to create OSC message.\n");
        exit(1);
    }

    ret = lo_send_message(target, argv[i], message);
    if (ret == -1) {
        fprintf(stderr, "An error occured: %s\n",
                lo_address_errstr(target));
        exit(1);
    }

    return 0;
}
