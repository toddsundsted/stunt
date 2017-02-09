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

#ifndef List_h
#define List_h 1

#include "structures.h"
#include "streams.h"

extern List new_list(int size);
extern void destroy_list(List& list);
extern List list_dup(const List& list);

extern List listappend(const List& list, const Var& value);
extern List listinsert(const List& list, const Var& value, int pos);
extern List listdelete(const List& list, int pos);
extern List listset(const List& list, const Var& value, int pos);
extern List listrangeset(const List& list, int from, int to, const List& value);
extern List listconcat(const List& first, const List& second);
extern List setadd(const List& list, const Var& value);
extern List setremove(const List& list, const Var& value);
extern List sublist(const List& list, int lower, int upper);
extern int listequal(const List& lhs, const List& rhs, int case_matters);

extern int list_sizeof(const List& list);

typedef int (*listfunc) (const Var& value, void *data, int first);
extern int listforeach(const List& list, listfunc func, void *data);

extern Str strrangeset(const Str& str, int from, int to, const Str& value);
extern Str substr(const Str& str, int lower, int upper);
extern Str strget(const Str& str, int i);

extern ref_ptr<const char> value2str(const Var&);
extern void unparse_value(Stream *, const Var&);

/*
 * Returns the length of the given list `l'.  Does *not* check to
 * ensure `l' is, in fact, a list.
 */
static inline int32_t
listlength(const Var& l)
{
    return l.v.list.expose()[0].v.num;
}

/*
 * Iterate over the values in the list `lst'.  Sets `val' to each
 * value, in turn.  `itr' and `cnt' must be int variables.  In the
 * body of the statement, they hold the current index and total count,
 * respectively.  Use the macro as follows (assuming you already have
 * a list in `items'):
 *   Var item;
 *   int i, c;
 *   FOR_EACH(item, items, i, c) {
 *       printf("%d of %d, item = %s\n", i, c, value_to_literal(item));
 *   }
 */
#define FOR_EACH(val, lst, idx, cnt)					\
for (idx = 1, cnt = static_cast<const List&>(lst).length();		\
     idx <= cnt && (val = static_cast<const List&>(lst)[idx], 1);	\
     idx++)

/*
 * Pop the first value off `stck' and put it in `tp'.
 */
#define POP_TOP(tp, stck)						\
tp = var_ref(stck[1]);							\
stck = listdelete(stck, 1);

#endif
