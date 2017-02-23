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

/**
 * A simple wrapper for pointers with hidden allocations (reference
 * counts, memoized values...).
 */

class wrapper {

 protected:

    void* ptr;

    explicit wrapper(void* ptr) : ptr(ptr) {}

    explicit wrapper() : ptr(nullptr) {}

 public:

    // `inc_ref()', `dec_ref()', `set_buffered()', `clear_buffered()'
    // and `set_color()' should not be `const' in the following
    // definitions.

    int inc_ref() const { return ++((reference_overhead*)ptr)[-1].count; }
    int dec_ref() const { return --((reference_overhead*)ptr)[-1].count; }
    int ref_count() const { return ((reference_overhead*)ptr)[-1].count; }

    void set_buffered() const { ((reference_overhead*)ptr)[-1].buffered = 1; }
    void clear_buffered() const { ((reference_overhead*)ptr)[-1].buffered = 0; }
    bool is_buffered() const { return ((reference_overhead*)ptr)[-1].buffered; }

    void set_color(GC_Color color) const { ((reference_overhead*)ptr)[-1].color = color; }
    GC_Color color() const { return ((reference_overhead*)ptr)[-1].color; }

    operator bool () const { return ptr != nullptr; }
};

/**
 * Type-safe accessor class. Construction is intentionally restricted
 * and, once instantiated, access to the underlying pointer is
 * restricted/explicit.
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
class ref_ptr : public wrapper {

 private:

    explicit ref_ptr(T* t) : wrapper((void*)t) {}

 public:

    static const ref_ptr<T> empty;

    explicit ref_ptr() : wrapper() {}

    const T& operator * () const {
	return *(T*)ptr;
    }

    T& operator * () {
	return *(T*)ptr;
    }

    const T* operator -> () const {
      return (T*)ptr;
    }

    T* operator -> () {
	return (T*)ptr;
    }

    const T* expose() const {
	return (T*)ptr;
    }

    T* expose() {
	return (T*)ptr;
    }

    bool operator == (const ref_ptr<T>& that) const { return this->ptr == that.ptr; }
    bool operator != (const ref_ptr<T>& that) const { return this->ptr != that.ptr; }

    friend ref_ptr<T> mymalloc<>(size_t);
    friend ref_ptr<T> myrealloc<>(ref_ptr<T>, size_t);
    friend void myfree<>(ref_ptr<T>);
};

template<typename T>
const ref_ptr<T> ref_ptr<T>::empty;

template<typename T>
extern ref_ptr<T> mymalloc(size_t);

template<>
ref_ptr<double> mymalloc(size_t);

template<typename T>
extern ref_ptr<T> myrealloc(ref_ptr<T>, size_t);

template<>
ref_ptr<double> myrealloc(ref_ptr<double>, size_t);

template<typename T>
extern void myfree(ref_ptr<T>);

template<>
void myfree(ref_ptr<double>);

template<>
void myfree(ref_ptr<const char>);

extern ref_ptr<const char> str_ref(const ref_ptr<const char>&);
extern ref_ptr<const char> str_dup(const char*);

static inline void
free_str(ref_ptr<const char>& s)
{
    if (s.dec_ref() == 0)
	myfree(s);
}

static inline int
memo_strlen(ref_ptr<const char> s)
{
#ifdef MEMO_STRLEN
    return ((int*)s.expose())[-2];
#else
    return strlen(s.expose());
#endif
}

#endif				/* Storage_h */
