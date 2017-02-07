/******************************************************************************
  Copyright (c) 1992, 1995, 1996 Xerox Corporation.  All rights reserved.
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

#ifndef Utils_h
#define Utils_h 1

#include "my-stdio.h"

#include "config.h"
#include "execute.h"
#include "list.h"
#include "map.h"
#include "storage.h"
#include "streams.h"

#undef MAX
#undef MIN
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define Arraysize(x) (sizeof(x) / sizeof(*x))

extern int mystrcasecmp(const char *, const char *);
extern int mystrncasecmp(const char *, const char *, int);

extern int verbcasecmp(const char *verb, const char *word);

extern unsigned str_hash(const char *);

extern void complex_free_var(const Var&);
extern Var complex_var_ref(const Var&);
extern Var complex_var_dup(const Var&);
extern int var_refcount(const Var&);

extern void aux_free(const Var&);

static inline void
free_var(const Var& var)
{
    if (var.is_complex())
	complex_free_var(var);
}

static inline Var
var_ref(const Var& var)
{
    return var.is_complex() ? complex_var_ref(var) : var;
}

static inline List
var_ref(const List& list)
{
    addref(list.v.list);
    return list;
}

static inline Map
var_ref(const Map& map)
{
    map.v.tree.inc_ref();
    return map;
}

static inline Iter
var_ref(const Iter& iter)
{
    iter.v.trav.inc_ref();
    return iter;
}

static inline Var
var_dup(const Var& var)
{
    return var.is_complex() ? complex_var_dup(var) : var;
}

static inline List
var_dup(const List& list)
{
    return list_dup(list);
}

static inline Map
var_dup(const Map& map)
{
    return map_dup(map);
}

extern int is_true(const Var& v);
extern int compare(const Var& lhs, const Var& rhs, int case_matters);
extern int equality(const Var& lhs, const Var& rhs, int case_matters);

extern void stream_add_strsub(Stream *, const char *, const char *, const char *, int);
extern int strindex(const char *, int, const char *, int, int);
extern int strrindex(const char *, int, const char *, int, int);

extern const char *strtr(const char *, int, const char *, int, const char *, int, int);

extern Var get_system_property(const char *);
extern Objid get_system_object(const char *);

extern int value_bytes(const Var&);

extern void stream_add_raw_bytes_to_clean(Stream *, const char *buffer, int buflen);
extern const char *raw_bytes_to_clean(const char *buffer, int buflen);
extern const char *clean_to_raw_bytes(const char *binary, int *rawlen);

extern void stream_add_raw_bytes_to_binary(Stream *, const char *buffer, int buflen);
/**
 * Converts an array of 8-bit binary characters into a null-terminated
 * string in MOO binary string format. `buflen` specifies the length
 * of the array. The returned string is re-used on subsequent calls,
 * so is should be copied if not used immediately.
 */
extern const char *raw_bytes_to_binary(const char *buffer, int buflen);
/**
 * Converts a null-terminated string in MOO binary string format into
 * an array of 8-bit binary characters. Returns the length of the
 * array in `rawlen`. The returned array is re-used on subsequent
 * calls, so is should be copied if not used immediately.
 */
extern const char *binary_to_raw_bytes(const char *binary, int *rawlen);

extern Var anonymizing_var_ref(const Var& v, Objid progr);
				/* To be used in places where you
				 * need to copy (via `var_ref()') a
				 * value and also need to ensure that
				 * _if_ it's an anonymous object it
				 * remains anonymous for
				 * non-wizards/owners.
				 */

#endif
