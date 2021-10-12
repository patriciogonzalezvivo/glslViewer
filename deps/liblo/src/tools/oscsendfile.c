/*
 * oscsend - Send OpenSound Control message.
 *
 * Copyright (C) 2016 Joseph Malloch <joseph.malloch@gmail.com>
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <config.h>
#include <limits.h>
#include <unistd.h>
#include <lo/lo.h>

static FILE* input_file = 0;
static double multiplier = 1.0/((double)(1LL<<32));

void usage(void)
{
    printf("oscsendfile version %s\n"
           "Copyright (C) 2016 Joseph Malloch\n"
           "  adapted from oscsend.c (C) 2008 Kentaro Fukuchi\n\n"
           "Usage: oscsend hostname port file <speed>\n"
           "or     oscsend url file <speed>\n"
           "Send OpenSound Control messages from a file via UDP.\n\n"
           "Description\n"
           "hostname: specifies the remote host's name.\n"
           "port    : specifies the port number to connect to the remote host.\n"
           "url     : specifies the destination parameters using a liblo URL.\n"
           "          e.g. UDP        \"osc.udp://localhost:9000\"\n"
           "               Multicast  \"osc.udp://224.0.1.9:9000\"\n"
           "               TCP        \"osc.tcp://localhost:9000\"\n"
           "speed   : specifies a speed multiplier.\n",
           VERSION);
           printf("Example\n"
           "$ oscsendfile localhost 7777 myfile.txt 2.5\n");
}

lo_message create_message(char **argv)
{
    /* Note:
     * argv[0] <- types
     * argv[1..] <- values
     */
    int i, argi;
    lo_message message;
    const char *types;
    char *arg;
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
        case LO_SYMBOL:
            if (arg[strlen(arg)-1] == '"')
                arg[strlen(arg)-1] = '\0';
            if (arg[0] == '"')
                lo_message_add_string(message, arg+1);
            else
                lo_message_add_string(message, arg);
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

void timetag_add(lo_timetag *tt, lo_timetag addend)
{
    tt->sec += addend.sec;
    tt->frac += addend.frac;
    if (tt->frac < addend.frac) // overflow
        tt->sec++;
}

void timetag_subtract(lo_timetag *tt, lo_timetag subtrahend)
{
    tt->sec -= subtrahend.sec;
    if (tt->frac < subtrahend.frac) // overflow
        --tt->sec;
    tt->frac -= subtrahend.frac;
}

double timetag_diff(const lo_timetag a, const lo_timetag b)
{
    return ((double)a.sec - (double)b.sec +
            ((double)a.frac - (double)b.frac) * multiplier);
}

double timetag_double(const lo_timetag tt)
{
    return (double)tt.sec + (double)tt.frac * multiplier;
}

void timetag_multiply(lo_timetag *tt, double d)
{
    d *= timetag_double(*tt);
    tt->sec = floor(d);
    d -= tt->sec;
    tt->frac = (uint32_t) (d * (double)(1LL<<32));
}

int send_file(lo_address target, double speed) {
    double speedmul = 1.0 / speed;
    const char *delim = " \r\n";
    char *args[64];
    char str[1024], *p, *s, *path;
    lo_timetag tt_start = {0, 0}, tt_start_file, tt_now;
    lo_timetag tt1 = {0, 0}, tt2 = {0, 0}, *tt_last = &tt1, *tt_this = &tt2;
    int tt_init = 0, ret;
    lo_message m;
    lo_bundle b = 0;

    lo_timetag_now(&tt_start);

    while (fgets(str, 1024, input_file)) {
        path = 0;
        s = strtok_r(str, delim, &p);
        m = lo_message_new();
        if (!m)
            return 1;

        if (s) {
            // check if first arg is timetag
            if (s[0] != '/') {
                // first argument is timetag
                char *ttstr = strtok(s, ".");
                if (ttstr) {
                    tt_this->sec = strtoul(ttstr, NULL, 16);
                    if (!tt_init)
                        tt_start_file.sec = tt_this->sec;
                }
                ttstr = strtok(0, ".");
                if (ttstr) {
                    tt_this->frac = strtoul(ttstr, NULL, 16);
                    if (!tt_init)
                        tt_start_file.frac = tt_this->frac;
                }
                if (1) {
                    timetag_subtract(tt_this, tt_start_file);
                    timetag_multiply(tt_this, speedmul);
                    timetag_add(tt_this, tt_start);
                }
                else {
                    if (!tt_init)
                        timetag_subtract(&tt_start, tt_start_file);
                    timetag_add(tt_this, tt_start);
                }
                tt_init = 1;
                s = strtok_r(0, delim, &p);
            }
            else {
                tt_this->sec = 0;
                tt_this->frac = 1;
            }
        }
        if (s)
            path = s;
        else
            continue;

        s = strtok_r(0, delim, &p);
        if (s)
            args[0] = s;    // types

        int i = 0;
        while ((s = strtok_r(0, delim, &p))) {
            args[++i] = s;
        }

        m = create_message(args);
        if (!m) {
            fprintf(stderr, "Failed to create OSC message.\n");
            return 1;
        }

        if (b && memcmp(tt_this, tt_last, sizeof(lo_timetag))==0) {
            lo_bundle_add_message(b, path, m);
        }
        else {
            // wait for timestamp?
            lo_timetag_now(&tt_now);
            double wait_time = timetag_diff(*tt_last, tt_now);
            if (wait_time > 0.) {
                usleep(wait_time * 1000000);
            }
            if (b) {
                ret = lo_send_bundle(target, b);
            }
            b = lo_bundle_new(*tt_this);
            lo_bundle_add_message(b, path, m);

            lo_timetag *tmp = tt_last;
            tt_last = tt_this;
            tt_this = tmp;
        }

        if (ret == -1)
            return ret;
    }

    if (b) {
        // wait for timestamp?
        lo_timetag_now(&tt_now);
        double wait_time = timetag_diff(*tt_last, tt_now);
        if (wait_time > 0.) {
            usleep(wait_time * 1000000);
        }
        lo_send_bundle(target, b);
    }

    return 0;
}

int main(int argc, char **argv)
{
    lo_address target;
    int ret, i=1;

    if (argc < 2) {
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
        lo_address_set_ttl(target, 1);
        i += 2;
    }

    if (argv[i] == NULL) {
        fprintf(stderr, "No %s given.\n", argc == i+1 ? "filename" : "path");
        exit(1);
    }

    // next arg should be filename
    input_file = fopen(argv[i], "r");
    if (!input_file) {
        fprintf(stderr, "Failed to open file `%s' for reading.\n", argv[i]);
        exit(1);
    }

    double speed = 1.0;
    if (argc > i+1) {
        // optional speed argument
        speed = atof(argv[i+1]);
    }
    ret = send_file(target, speed);

    if (ret == -1) {
        fprintf(stderr, "An error occured: %s\n",
                lo_address_errstr(target));
        exit(1);
    }

    return 0;
}
