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

#include <stdio.h>

#include "lo_types_internal.h"
#include "lo/lo.h"

void lo_method_pp(lo_method m)
{
    lo_method_pp_prefix(m, "");
}

void lo_method_pp_prefix(lo_method m, const char *p)
{
    printf("%spath:      %s\n", p, m->path);
    printf("%stypespec:  %s\n", p, m->typespec);
    printf("%shandler:   %p\n", p, m->handler);
    printf("%suser-data: %p\n", p, m->user_data);
}

/* vi:set ts=8 sts=4 sw=4: */
