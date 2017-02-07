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

#include "my-stdlib.h"

#include "config.h"
#include "list.h"
#include "options.h"
#include "server.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

static inline int
refcount_overhead(Memory_Type type)
{
    /* These are the only allocation types that are addref()'d.
     * As long as we're living on the wild side, avoid getting the
     * refcount slot for allocations that won't need it.
     */
    switch (type) {
    /* deal with systems with picky alignment issues */
    case M_FLOAT:
	return MAX(sizeof(int), sizeof(double *));
    case M_LIST:
#ifdef MEMO_VALUE_BYTES
	return MAX(sizeof(int), sizeof(Var *)) * 2;
#else
	return MAX(sizeof(int), sizeof(Var *));
#endif /* MEMO_VALUE_BYTES */
    case M_TREE:
#ifdef MEMO_VALUE_BYTES
	return MAX(sizeof(int), sizeof(rbtree *)) * 2;
#else
	return MAX(sizeof(int), sizeof(rbtree *));
#endif /* MEMO_VALUE_BYTES */
    case M_TRAV:
	return MAX(sizeof(int), sizeof(rbtrav *));
    case M_STRING:
#ifdef MEMO_STRLEN
	return sizeof(int) * 2;
#else
	return sizeof(int);
#endif /* MEMO_STRLEN */
    case M_ANON:
	return MAX(sizeof(int), sizeof(struct Object *));
    default:
	return 0;
    }
}

template<typename T> ref_ptr<T>
mymalloc(size_t size)
{
    return ref_ptr<T>((T*)mymalloc(size, M_NONE));
}

template<> ref_ptr<double>
mymalloc(size_t size)
{
    return ref_ptr<double>((double*)mymalloc(size, M_FLOAT));
}

template<> ref_ptr<rbtree>
mymalloc(size_t size)
{
    return ref_ptr<rbtree>((rbtree*)mymalloc(size, M_TREE));
}

template<> ref_ptr<rbtrav>
mymalloc(size_t size)
{
    return ref_ptr<rbtrav>((rbtrav*)mymalloc(size, M_TRAV));
}

template<> ref_ptr<const char>
mymalloc(size_t size)
{
    return ref_ptr<const char>((const char*)mymalloc(size, M_STRING));
}

void *
mymalloc(unsigned size, Memory_Type type)
{
    char *memptr;
    char msg[100];
    int offs;

    if (size == 0)		/* For queasy systems */
	size = 1;

    offs = refcount_overhead(type);
    memptr = (char*)malloc(offs + size);
    if (!memptr) {
	sprintf(msg, "memory allocation (size %u) failed!", size);
	panic(msg);
    }

    if (offs) {
	memptr += offs;
	((reference_overhead*)memptr)[-1].count = 1;
#ifdef ENABLE_GC
	((reference_overhead*)memptr)[-1].buffered = 0;
	((reference_overhead*)memptr)[-1].color = (type == M_ANON) ? GC_BLACK : GC_GREEN;
#endif /* ENABLE_GC */
#ifdef MEMO_STRLEN
	if (type == M_STRING)
	    ((int*)memptr)[-2] = size - 1;
#endif /* MEMO_STRLEN */
#ifdef MEMO_VALUE_BYTES
	if (type == M_LIST)
	    ((int*)memptr)[-2] = 0;
	if (type == M_TREE)
	    ((int*)memptr)[-2] = 0;
#endif /* MEMO_VALUE_BYTES */
    }
    return memptr;
}

ref_ptr<const char>
str_ref(const ref_ptr<const char>& s)
{
    s.inc_ref();
    return s;
}

ref_ptr<const char>
str_dup(const char* s)
{
    if (s == nullptr || *s == '\0') {
	static ref_ptr<const char> e;
	if (!e) {
	    e = mymalloc<const char>(1);
	    char* t = const_cast<char*>(e.expose());
	    *t = '\0';
	}
	e.inc_ref();
	return e;
    } else {
	ref_ptr<const char> r = mymalloc<const char>(strlen(s) + 1);
	char *t = const_cast<char*>(r.expose());
	strcpy(t, s);
	return r;
    }
}

template<typename T> ref_ptr<T>
myrealloc(ref_ptr<T> ptr, size_t size)
{
    return ref_ptr<T>((T*)myrealloc(ptr, size, M_NONE));
}

template<> ref_ptr<double>
myrealloc(ref_ptr<double> ptr, size_t size)
{
    return ref_ptr<double>((double*)myrealloc(ptr.ptr, size, M_FLOAT));
}

template<> ref_ptr<rbtree>
myrealloc(ref_ptr<rbtree> ptr, size_t size)
{
    return ref_ptr<rbtree>((rbtree*)myrealloc(ptr.ptr, size, M_TREE));
}

template<> ref_ptr<rbtrav>
myrealloc(ref_ptr<rbtrav> ptr, size_t size)
{
    return ref_ptr<rbtrav>((rbtrav*)myrealloc(ptr.ptr, size, M_TRAV));
}

template<> ref_ptr<const char>
myrealloc(ref_ptr<const char> ptr, size_t size)
{
    return ref_ptr<const char>((const char*)myrealloc((void*)ptr.ptr, size, M_STRING));
}

void *
myrealloc(void *ptr, unsigned size, Memory_Type type)
{
    int offs = refcount_overhead(type);
    static char msg[100];

    ptr = realloc((char *) ptr - offs, size + offs);
    if (!ptr) {
	sprintf(msg, "memory re-allocation (size %u) failed!", size);
	panic(msg);
    }

    return (char *) ptr + offs;
}

template<typename T> void
myfree(ref_ptr<T> ptr)
{
    myfree((void*)ptr.ptr, M_NONE);
}

template<> void
myfree<double>(ref_ptr<double> ptr)
{
    myfree(ptr.ptr, M_FLOAT);
}

template<> void
myfree<rbtree>(ref_ptr<rbtree> ptr)
{
    myfree(ptr.ptr, M_TREE);
}

template<> void
myfree<rbtrav>(ref_ptr<rbtrav> ptr)
{
    myfree(ptr.ptr, M_TRAV);
}

template<> void
myfree<const char>(ref_ptr<const char> ptr)
{
    myfree((void*)ptr.ptr, M_STRING);
}

void
myfree(void *ptr, Memory_Type type)
{
    free((char *) ptr - refcount_overhead(type));
}
