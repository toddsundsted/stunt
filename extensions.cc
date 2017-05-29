/******************************************************************************
  Copyright (c) 1995, 1996 Xerox Corporation.  All rights reserved.
  Portions of this code were written by Stephen White, aka ghond.
  Use and copying of this software and preparation of derivative works based
  upon this software are permitted.  Any distribution of this software or
  derivative works must comply with all applicable United States export
  control laws.  This software is made available AS IS, and Xerox Corporation
  makes no warranty about the software, its performance or its conformity to
  any specification.  Any person obtaining a copy of this software is requested
  to send their name and post office or electronic mail address to:
    Pavel Curtis
    Xerox PARC
    3333 Coyote Hill Rd.
    Palo Alto, CA 94304
    Pavel@Xerox.Com
 *****************************************************************************/

/* Extensions to the MOO server

 * This module contains some examples of how to extend the MOO server using
 * some of the interfaces exported by various other modules.  The examples are
 * all commented out, since they're really not all that useful in general; they
 * were written primarily to test out the interfaces they use.
 *
 * The uncommented parts of this module provide a skeleton for any module that
 * implements new MOO built-in functions.  Feel free to replace the
 * commented-out bits with your own extensions; in future releases, you can
 * just replace the distributed version of this file (which will never contain
 * any actually useful code) with your own edited version as a simple way to
 * link in your extensions.
 */

#include "my-stdlib.h"
#include "my-sys-time.h"

#include "functions.h"
#include "numbers.h"
#include "server.h"
#include "storage.h"
#include "utils.h"

/* us_time - return a float of time() with microseconds */
static package
bf_us_time(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
    struct timezone *tz = 0;

    gettimeofday(tv,tz);

    r = new_float( ( (double)tv->tv_sec) + ( (double)tv->tv_usec / 1000000 ) );

    free(tv);
    free_var(arglist);
    return make_var_pack(r);
}

/* Panic - panic the server with the given message. */
static package
bf_panic(Var arglist, Byte next, void *vdata, Objid progr)
{
    const char *msg;

    if (!is_wizard(progr))
        return make_error_pack(E_PERM);

    if (arglist.v.list[0].v.num) {
        msg = str_dup(arglist.v.list[1].v.str);
    } else {
        msg = "Intentional Server Panic";
    }

    free_var(arglist);
    panic(msg);

    return make_error_pack(E_NONE);
}

static package
bf_chr (Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    char str[2];

    if (!is_wizard(progr))
        return make_error_pack(E_PERM);

    switch (arglist.v.list[1].type) {
        case TYPE_INT:
            if ((arglist.v.list[1].v.num < 1) || (arglist.v.list[1].v.num > 255)) {
                free_var(arglist);
                return make_error_pack(E_INVARG);
            }

            str[0] = (char) arglist.v.list[1].v.num;
            str[1] = '\0';
            r.type = TYPE_STR;
            r.v.str = str_dup(str);
            break;
        case TYPE_STR:
            if (!(r.v.num = (int) arglist.v.list[1].v.str[0])) {
                free_var(arglist);
                return make_error_pack(E_INVARG);
            }

            r.type = TYPE_INT;
            break;
        default:
            free_var(arglist);
            return make_error_pack(E_TYPE);
            break;
    }

    free_var(arglist);
    return make_var_pack(r);
}


void
register_extensions()
{
    register_function("us_time", 0, 0, bf_us_time);
    register_function("panic", 0, 1, bf_panic, TYPE_STR);
    register_function("chr", 1, 1, bf_chr, TYPE_ANY);
}
