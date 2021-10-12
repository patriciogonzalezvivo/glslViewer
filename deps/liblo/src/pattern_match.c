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

/* This code was originally forked from: */

/* Open SoundControl kit in C++                                              */
/* Copyright (C) 2002-2004 libOSC++ contributors. See AUTHORS                */
/*                                                                           */
/* This library is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU Lesser General Public                */
/* License as published by the Free Software Foundation; either              */
/* version 2.1 of the License, or (at your option) any later version.        */
/*                                                                           */
/* This library is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         */
/* Lesser General Public License for more details.                           */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public          */
/* License along with this library; if not, write to the Free Software       */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                           */
/* For questions regarding this program contact                              */
/* Daniel Holth <dholth@fastmail.fm> or visit                                */
/* http://wiretap.stetson.edu/                                               */

/* In the sprit of the public domain, my modifications to this file are also */
/* dedicated to the public domain. Daniel Holth, Oct. 2004                   */

/* ChangeLog:
 * 
 * 2004-10-29 Import, convert to C++, begin OSC syntax changes. -dwh
 *              OSC syntax changes are now working, needs more testing.
 *
 */

// Original header and syntax: 

/*
 * robust glob pattern matcher
 * ozan s. yigit/dec 1994
 * public domain
 *
 * glob patterns:
 *  *   matches zero or more characters
 *  ?   matches any single character
 *  [set]   matches any character in the set
 *  [^set]  matches any character NOT in the set
 *      where a set is a group of characters or ranges. a range
 *      is written as two characters seperated with a hyphen: a-z denotes
 *      all characters between a to z inclusive.
 *  [-set]  set matches a literal hypen and any character in the set
 *  []set]  matches a literal close bracket and any character in the set
 *
 *  char    matches itself except where char is '*' or '?' or '['
 *  \char   matches char, including any pattern character
 *
 * examples:
 *  a*c     ac abc abbc ...
 *  a?c     acc abc aXc ...
 *  a[a-z]c     aac abc acc ...
 *  a[-a-z]c    a-c aac abc ...
 *
 * $Log$
 * Revision 1.1  2004/11/19 23:00:57  theno23
 * Added lo_send_timestamped
 *
 * Revision 1.3  1995/09/14  23:24:23  oz
 * removed boring test/main code.
 *
 * Revision 1.2  94/12/11  10:38:15  oz
 * cset code fixed. it is now robust and interprets all
 * variations of cset [i think] correctly, including [z-a] etc.
 * 
 * Revision 1.1  94/12/08  12:45:23  oz
 * Initial revision
 */

#include "lo/lo.h"

#ifndef NEGATE
#define NEGATE  '!'
#endif

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

int lo_pattern_match(const char *str, const char *p)
{
    int negate;
    int match;
    char c;

    while (*p) {
        if (!*str && *p != '*')
            return false;

        switch (c = *p++) {

        case '*':
            while (*p == '*' && *p != '/')
                p++;

            if (!*p)
                return true;

//                if (*p != '?' && *p != '[' && *p != '\\')
            if (*p != '?' && *p != '[' && *p != '{')
                while (*str && *p != *str)
                    str++;

            while (*str) {
                if (lo_pattern_match(str, p))
                    return true;
                str++;
            }
            return false;

        case '?':
            if (*str)
                break;
            return false;
            /*
             * set specification is inclusive, that is [a-z] is a, z and
             * everything in between. this means [z-a] may be interpreted
             * as a set that contains z, a and nothing in between.
             */
        case '[':
            if (*p != NEGATE)
                negate = false;
            else {
                negate = true;
                p++;
            }

            match = false;

            while (!match && (c = *p++)) {
                if (!*p)
                    return false;
                if (*p == '-') {        /* c-c */
                    if (!*++p)
                        return false;
                    if (*p != ']') {
                        if (*str == c || *str == *p ||
                            (*str > c && *str < *p))
                            match = true;
                    } else {    /* c-] */
                        if (*str >= c)
                            match = true;
                        break;
                    }
                } else {        /* cc or c] */
                    if (c == *str)
                        match = true;
                    if (*p != ']') {
                        if (*p == *str)
                            match = true;
                    } else
                        break;
                }
            }

            if (negate == match)
                return false;
            /*
             * if there is a match, skip past the cset and continue on
             */
            while (*p && *p != ']')
                p++;
            if (!*p++)          /* oops! */
                return false;
            break;

            /*
             * {astring,bstring,cstring}
             */
        case '{':
            {
                // *p is now first character in the {brace list}
                const char *place = str;        // to backtrack
                const char *remainder = p;      // to forwardtrack

                // find the end of the brace list
                while (*remainder && *remainder != '}')
                    remainder++;
                if (!*remainder++)      /* oops! */
                    return false;

                c = *p++;

                while (c) {
                    if (c == ',') {
                        if (lo_pattern_match(str, remainder)) {
                            return true;
                        } else {
                            // backtrack on test string
                            str = place;
                            // continue testing,
                            // skip comma
                            if (!*p++)  // oops
                                return false;
                        }
                    } else if (c == '}') {
                        // continue normal pattern matching
                        if (!*p && !*str)
                            return true;
                        str--;  // str is incremented again below
                        break;
                    } else if (c == *str) {
                        str++;
                        if (!*str && *remainder)
                            return false;
                    } else {    // skip to next comma
                        str = place;
                        while (*p != ',' && *p != '}' && *p)
                            p++;
                        if (*p == ',')
                            p++;
                        else if (*p == '}') {
                            return false;
                        }
                    }
                    c = *p++;
                }
            }

            break;

            /* Not part of OSC pattern matching
               case '\\':
               if (*p)
               c = *p++;
             */

        default:
            if (c != *str)
                return false;
            break;

        }
        str++;
    }

    return !*str;
}
