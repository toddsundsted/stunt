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

#ifndef Storage_h
#define Storage_h 1

#include "my-string.h"

#include "options.h"

/* See "Concurrent Cycle Collection in Reference Counted Systems",
 * (Bacon and Rajan, 2001) for a description of the cycle collection
 * algorithm and the colors.
 */
typedef enum GC_Color {
    GC_GREEN,
    GC_YELLOW,
    GC_BLACK,
    GC_GRAY,
    GC_WHITE,
    GC_PURPLE,
    GC_PINK
} GC_Color;

typedef struct reference_overhead {
    unsigned int count:28;
    unsigned int buffered:1;
    GC_Color color:3;
} reference_overhead;

static inline int
addref(const void *ptr)
{
    return ++((reference_overhead *)ptr)[-1].count;
}

static inline int
delref(const void *ptr)
{
    return --((reference_overhead *)ptr)[-1].count;
}

static inline int
refcount(const void *ptr)
{
    return ((reference_overhead *)ptr)[-1].count;
}

static inline void
gc_set_buffered(const void *ptr)
{
    ((reference_overhead *)ptr)[-1].buffered = 1;
}

static inline void
gc_clear_buffered(const void *ptr)
{
    ((reference_overhead *)ptr)[-1].buffered = 0;
}

static inline int
gc_is_buffered(const void *ptr)
{
    return ((reference_overhead *)ptr)[-1].buffered;
}

static inline void
gc_set_color(const void *ptr, GC_Color color)
{
    ((reference_overhead *)ptr)[-1].color = color;
}

static inline GC_Color
gc_get_color(const void *ptr)
{
    return ((reference_overhead *)ptr)[-1].color;
}

typedef enum Memory_Type {
    M_NONE,

    M_STRING, M_LIST, M_TREE, M_TRAV, M_FLOAT, M_ANON,

    Sizeof_Memory_Type

} Memory_Type;

/* A simple wrapper for pointers with hidden allocations
 * (reference counts, memoized values, etc.)
 */

// forward declarations

template<typename T>
class ref_ptr;

template<typename T>
extern ref_ptr<T> mymalloc(size_t);

template<typename T>
extern ref_ptr<T> myrealloc(ref_ptr<T>, size_t);

template<typename T>
extern void myfree(ref_ptr<T>);

template<typename T>
class ref_ptr {

    T* ptr;

    explicit ref_ptr(T* t) : ptr(t) {}

 public:

    const T& operator * () const {
	return *ptr;
    }

    T& operator * () {
	return *ptr;
    }

    bool operator == (const ref_ptr<T> that) const { return this->ptr == that.ptr; }
    bool operator != (const ref_ptr<T> that) const { return this->ptr != that.ptr; }

    int inc_ref() const { return addref(ptr); }     // should not be const!
    int dec_ref() const { return delref(ptr); }     // should not be const!
    int ref_count() const { return refcount(ptr); }

    friend ref_ptr<T> mymalloc<>(size_t);
    friend ref_ptr<T> myrealloc<>(ref_ptr<T>, size_t);
    friend void myfree<>(ref_ptr<T>);
};

extern const char *str_dup(const char *);
extern const char *str_ref(const char *);

template<typename T>
extern ref_ptr<T> mymalloc(size_t);

template<>
extern ref_ptr<double> mymalloc(size_t);

template<typename T>
extern ref_ptr<T> myrealloc(ref_ptr<T>, size_t);

template<>
extern ref_ptr<double> myrealloc(ref_ptr<double>, size_t);

template<typename T>
extern void myfree(ref_ptr<T>);

template<>
extern void myfree(ref_ptr<double>);

extern void myfree(void *where, Memory_Type type);
extern void *mymalloc(unsigned size, Memory_Type type);
extern void *myrealloc(void *where, unsigned size, Memory_Type type);

static inline void		/* XXX was extern, fix for non-gcc compilers */
free_str(const char *s)
{
    if (delref(s) == 0)
	myfree((void *) s, M_STRING);
}

#ifdef MEMO_STRLEN
/*
 * Using the same mechanism as ref_count.h uses to hide Value ref counts,
 * keep a memozied strlen in the storage with the string.
 */
#define memo_strlen(X)		((void)0, (((int *)(X))[-2]))
#else
#define memo_strlen(X)		strlen(X)

#endif /* MEMO_STRLEN */

#endif				/* Storage_h */
