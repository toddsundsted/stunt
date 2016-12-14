/******************************************************************************
  Copyright (c) 1996 Xerox Corporation.  All rights reserved.
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

#include "sosemanuk.h"
#include "structures.h"

extern enum error become_integer(const Var&, int *, int);

extern int do_equals(const Var&, const Var&);
extern int compare_integers(int, int);
extern Var compare_numbers(const Var&, const Var&);

extern Var do_add(const Var&, const Var&);
extern Var do_subtract(const Var&, const Var&);
extern Var do_multiply(const Var&, const Var&);
extern Var do_divide(const Var&, const Var&);
extern Var do_modulus(const Var&, const Var&);
extern Var do_power(const Var&, const Var&);

extern sosemanuk_key_context key_context;
extern sosemanuk_run_context run_context;
