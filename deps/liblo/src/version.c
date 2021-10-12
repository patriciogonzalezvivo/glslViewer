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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lo_types_internal.h"
#include "lo_internal.h"
#include "lo/lo.h"
#include "lo/lo_throw.h"

void lo_version(char *verstr, int verstr_size,
                int *major, int *minor, char *extra, int extra_size,
                int *lt_major, int *lt_minor, int *lt_bug)
{
    int _maj, _min, _ltmaj, _ltmin, _ltbug;
    int _lt[] = LO_SO_VERSION;
    char ex[256];

    int i = sscanf(PACKAGE_VERSION, "%d.%d%s", &_maj, &_min, ex);

    if (extra && extra_size > 0)
        extra[0] = 0;

    if (major) *major = 0;
    if (minor) *minor = 0;

    if (i==2 || i==3) {
        if (major) *major = _maj;
        if (minor) *minor = _min;
        if (extra && i==3) strncpy(extra, ex, extra_size);
    }
#ifdef DEBUG
    else {
        fprintf(stderr, "[liblo] Internal error processing "
                "version string (%s, line %d)\n", __FILE__, __LINE__);
        fprintf(stderr, "[liblo] Expected sscanf to return 3, got %d\n", i);
        abort();
    }
#endif

    if (verstr) strncpy(verstr, PACKAGE_VERSION, verstr_size);

    _ltmaj = _lt[0] - _lt[2];
    _ltmin = _lt[2];
    _ltbug = _lt[1];

    if (lt_major) *lt_major = _ltmaj;
    if (lt_minor) *lt_minor = _ltmin;
    if (lt_bug)   *lt_bug   = _ltbug;
}
