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

#include <assert.h>
#include <stdlib.h>

#include "my-ctype.h"
#include "my-string.h"

#include "bf_register.h"
#include "collection.h"
#include "config.h"
#include "functions.h"
#include "list.h"
#include "log.h"
#include "map.h"
#include "options.h"
#include "pattern.h"
#include "streams.h"
#include "storage.h"
#include "structures.h"
#include "str_intern.h"
#include "unparse.h"
#include "utils.h"
#include "server.h"

List
new_list(int size)
{
    List list;
    Var *ptr;

    if (size == 0) {
	static List emptylist;

	if (emptylist.v.list == NULL) {
	    if ((ptr = (Var *)mymalloc(1 * sizeof(Var), M_LIST)) == NULL)
		panic("EMPTY_LIST: mymalloc failed");

	    emptylist.type = TYPE_LIST;
	    emptylist.v.list = ptr;
	    emptylist.v.list[0] = Var::new_int(0);
	}

#ifdef ENABLE_GC
	assert(gc_get_color(emptylist.v.list) == GC_GREEN);
#endif

	addref(emptylist.v.list);

	return emptylist;
    }

    if ((ptr = (Var *)mymalloc((size + 1) * sizeof(Var), M_LIST)) == NULL)
	panic("EMPTY_LIST: mymalloc failed");

    list.type = TYPE_LIST;
    list.v.list = ptr;
    list.v.list[0] = Var::new_int(size);

#ifdef ENABLE_GC
    gc_set_color(list.v.list, GC_YELLOW);
#endif

    return list;
}

/* called from utils.c */
void
destroy_list(List& list)
{
    int i;
    Var *pv;

    for (i = list.length(), pv = list.v.list + 1; i > 0; i--, pv++)
	free_var(*pv);

    /* Since this list could possibly be the root of a cycle, final
     * destruction is handled in the garbage collector if garbage
     * collection is enabled.
     */
#ifndef ENABLE_GC
    myfree(list.v.list, M_LIST);
#endif
}

/* called from utils.c */
List
list_dup(const List& list)
{
    int i, n = list.length();
    List _new = new_list(n);

    for (i = 1; i <= n; i++)
	_new.v.list[i] = var_ref(list.v.list[i]);

    gc_set_color(_new.v.list, gc_get_color(list.v.list));

    return _new;
}

int
listforeach(const List& list, listfunc func, void *data)
{				/* does NOT consume `list' */
    int i, n;
    int first = 1;
    int ret;

    for (i = 1, n = list.length(); i <= n; i++) {
	if ((ret = (*func)(list[i], data, first)))
	    return ret;
	first = 0;
    }

    return 0;
}

List
setadd(const List& list, const Var& value)
{
    if (ismember(value, list, 0)) {
	free_var(value);
	return list;
    }
    return listappend(list, value);
}

List
setremove(const List& list, const Var& value)
{
    int i;
    if ((i = ismember(value, list, 0)) != 0) {
	return listdelete(list, i);
    } else {
	return list;
    }
}

List
listset(const List& list, const Var& value, int pos)
{				/* consumes `list', `value' */
    List _new;

    if (var_refcount(list) > 1) {
	_new = var_dup(list);
	free_var(list);
    } else {
	_new = list;
    }

#ifdef MEMO_VALUE_BYTES
    /* reset the memoized size */
    ((int *)(_new.v.list))[-2] = 0;
#endif

    free_var(_new.v.list[pos]);
    _new.v.list[pos] = value;

#ifdef ENABLE_GC
    gc_set_color(_new.v.list, GC_YELLOW);
#endif

    return _new;
}

static List
doinsert(const List& list, const Var& value, int pos)
{
    int size = list.length() + 1;

    if (var_refcount(list) == 1 && pos == size) {
	List _new = list;

	_new.v.list = (Var *) myrealloc(_new.v.list, (size + 1) * sizeof(Var), M_LIST);
#ifdef MEMO_VALUE_BYTES
	/* reset the memoized size */
	((int *)(_new.v.list))[-2] = 0;
#endif
	_new.v.list[0].v.num = size;
	_new.v.list[pos] = value;

#ifdef ENABLE_GC
	gc_set_color(_new.v.list, GC_YELLOW);
#endif

	return _new;

    } else {
	List _new = new_list(size);
	int i;

	for (i = 1; i < pos; i++)
	    _new.v.list[i] = var_ref(list.v.list[i]);
	_new.v.list[pos] = value;
	for (i = pos; i <= list.length(); i++)
	    _new.v.list[i + 1] = var_ref(list.v.list[i]);

	free_var(list);

#ifdef ENABLE_GC
	gc_set_color(_new.v.list, GC_YELLOW);
#endif

	return _new;
    }
}

List
listinsert(const List& list, const Var& value, int pos)
{
    if (pos <= 0)
	pos = 1;
    else if (pos > list.length())
	pos = list.length() + 1;
    return doinsert(list, value, pos);
}

List
listappend(const List& list, const Var& value)
{
    return doinsert(list, value, list.length() + 1);
}

List
listdelete(const List& list, int pos)
{
    List _new;
    int size = list.length() - 1;
    int i;

    _new = new_list(size);
    for (i = 1; i < pos; i++) {
	_new.v.list[i] = var_ref(list.v.list[i]);
    }
    for (i = pos + 1; i <= list.length(); i++)
	_new.v.list[i - 1] = var_ref(list.v.list[i]);

    free_var(list);

#ifdef ENABLE_GC
    if (size > 0)		/* only non-empty lists */
	gc_set_color(_new.v.list, GC_YELLOW);
#endif

    return _new;
}

List
listconcat(const List& first, const List& second)
{
    int lfirst = first.length();
    int lsecond = second.length();
    List _new;
    int i;

    _new = new_list(lsecond + lfirst);
    for (i = 1; i <= lfirst; i++)
	_new.v.list[i] = var_ref(first.v.list[i]);
    for (i = 1; i <= lsecond; i++)
	_new.v.list[i + lfirst] = var_ref(second.v.list[i]);

    free_var(first);
    free_var(second);

#ifdef ENABLE_GC
    if (lsecond + lfirst > 0)	/* only non-empty lists */
	gc_set_color(_new.v.list, GC_YELLOW);
#endif

    return _new;
}

List
listrangeset(const List& base, int from, int to, const List& value)
{
    /* base and value are free'd */
    int index, offset = 0;
    int base_len = base.length();
    int val_len = value.length();
    int lenleft = (from > 1) ? from - 1 : 0;
    int lenmiddle = val_len;
    int lenright = (base_len > to) ? base_len - to : 0;
    int newsize = lenleft + lenmiddle + lenright;

    List ans = new_list(newsize);
    for (index = 1; index <= lenleft; index++)
	ans.v.list[++offset] = var_ref(base.v.list[index]);
    for (index = 1; index <= lenmiddle; index++)
	ans.v.list[++offset] = var_ref(value.v.list[index]);
    for (index = 1; index <= lenright; index++)
	ans.v.list[++offset] = var_ref(base.v.list[to + index]);

    free_var(base);
    free_var(value);

#ifdef ENABLE_GC
    if (newsize > 0)	/* only non-empty lists */
	gc_set_color(ans.v.list, GC_YELLOW);
#endif

    return ans;
}

List
sublist(const List& list, int lower, int upper)
{
    if (lower > upper) {
	free_var(list);
	return new_list(0);
    } else {
	int i;
	List r = new_list(upper - lower + 1);
	for (i = lower; i <= upper; i++)
	    r.v.list[i - lower + 1] = var_ref(list.v.list[i]);

	free_var(list);

#ifdef ENABLE_GC
	gc_set_color(r.v.list, GC_YELLOW);
#endif

	return r;
    }
}

int
listequal(const List& lhs, const List& rhs, int case_matters)
{
    if (lhs.v.list == rhs.v.list)
	return 1;

    if (lhs.length() != rhs.length())
	return 0;

    int i, c = lhs.length();
    for (i = 1; i <= c; i++) {
	if (!equality(lhs[i], rhs[i], case_matters))
	    return 0;
    }

    return 1;
}

static void
stream_add_tostr(Stream * s, const Var& v)
{
    switch (v.type) {
    case TYPE_INT:
	stream_printf(s, "%d", v.v.num);
	break;
    case TYPE_OBJ:
	stream_printf(s, "#%d", v.v.obj);
	break;
    case TYPE_STR:
	stream_add_string(s, v.v.str.expose());
	break;
    case TYPE_ERR:
	stream_add_string(s, unparse_error(v.v.err));
	break;
    case TYPE_FLOAT:
	stream_printf(s, "%g", *v.v.fnum);
	break;
    case TYPE_MAP:
	stream_add_string(s, "[map]");
	break;
    case TYPE_LIST:
	stream_add_string(s, "{list}");
	break;
    case TYPE_ANON:
	stream_add_string(s, "*anonymous*");
	break;
    default:
	panic("STREAM_ADD_TOSTR: Unknown Var type");
    }
}

ref_ptr<const char>
value2str(const Var& value)
{
    if (value.is_str()) {
	/* do this case separately to avoid two copies
	 * and to ensure that the stream never grows */
	return str_ref(value.v.str);
    }
    else {
	static Stream *s = 0;
	if (!s)
	    s = new_stream(32);
	stream_add_tostr(s, value);
	return str_dup(reset_stream(s));
    }
}

static int
print_map_to_stream(const Var& key, const Var& value, void *sptr, int first)
{
    Stream *s = (Stream *)sptr;

    if (!first) {
	stream_add_string(s, ", ");
    }

    unparse_value(s, key);
    stream_add_string(s, " -> ");
    unparse_value(s, value);

    return 0;
}

void
unparse_value(Stream* s, const Var& v)
{
    switch (v.type) {
    case TYPE_INT:
	stream_printf(s, "%d", v.v.num);
	break;
    case TYPE_OBJ:
	stream_printf(s, "#%d", v.v.obj);
	break;
    case TYPE_ERR:
	stream_add_string(s, error_name(v.v.err));
	break;
    case TYPE_FLOAT:
	stream_printf(s, "%g", *v.v.fnum);
	break;
    case TYPE_STR:
	{
	    const char* str = v.v.str.expose();

	    stream_add_char(s, '"');
	    while (*str) {
		switch (*str) {
		case '"':
		case '\\':
		    stream_add_char(s, '\\');
		    /* fall thru */
		default:
		    stream_add_char(s, *str++);
		}
	    }
	    stream_add_char(s, '"');
	}
	break;
    case TYPE_LIST:
	{
	    const char *sep = "";
	    int len, i;

	    stream_add_char(s, '{');
	    len = v.v.list[0].v.num;
	    for (i = 1; i <= len; i++) {
		stream_add_string(s, sep);
		sep = ", ";
		unparse_value(s, v.v.list[i]);
	    }
	    stream_add_char(s, '}');
	}
	break;
    case TYPE_MAP:
	{
	    stream_add_char(s, '[');
	    mapforeach(static_cast<const Map&>(v), print_map_to_stream, (void *)s);
	    stream_add_char(s, ']');
	}
	break;
    case TYPE_ANON:
	stream_add_string(s, "*anonymous*");
	break;
    default:
	errlog("UNPARSE_VALUE: Unknown Var type = %d\n", v.type);
	stream_add_string(s, ">>Unknown value<<");
    }
}

/* called from utils.c */
int
list_sizeof(const Var *list)
{
    int i, len, size;

#ifdef MEMO_VALUE_BYTES
    if ((size = (((int *)(list))[-2])))
	return size;
#endif

    size = sizeof(Var);	/* for the `length' element */
    len = list[0].v.num;
    for (i = 1; i <= len; i++) {
	size += value_bytes(list[i]);
    }

#ifdef MEMO_VALUE_BYTES
    (((int *)(list))[-2]) = size;
#endif

    return size;
}

Str
strrangeset(const Str& base, int from, int to, const Str& value)
{
    /* base and value are free'd */
    int index, offset = 0;
    int base_len = memo_strlen(base.v.str);
    int val_len = memo_strlen(value.v.str);
    int lenleft = (from > 1) ? from - 1 : 0;
    int lenmiddle = val_len;
    int lenright = (base_len > to) ? base_len - to : 0;
    int newsize = lenleft + lenmiddle + lenright;

    Str ans;
    ref_ptr<const char> tmp = mymalloc<const char>(newsize + 1);
    char* s = const_cast<char*>(tmp.expose());
    for (index = 0; index < lenleft; index++)
	s[offset++] = base.v.str.expose()[index];
    for (index = 0; index < lenmiddle; index++)
	s[offset++] = value.v.str.expose()[index];
    for (index = 0; index < lenright; index++)
	s[offset++] = base.v.str.expose()[index + to];
    s[offset] = '\0';
    ans.type = TYPE_STR;
    ans.v.str = tmp;

    free_var(base);
    free_var(value);

    return ans;
}

Str
substr(const Str& str, int lower, int upper)
{
    Str r;
    r.type = TYPE_STR;

    if (lower > upper) {
	r.v.str = str_intern("");
    } else {
	int loop, index = 0;
	ref_ptr<const char> tmp = mymalloc<const char>(upper - lower + 2);
	char* s = const_cast<char*>(tmp.expose());
	for (loop = lower - 1; loop < upper; loop++)
	    s[index++] = str.v.str.expose()[loop];
	s[index] = '\0';
	r.v.str = tmp;
    }

    free_var(str);

    return r;
}

Str
strget(const Str& str, int i)
{
    Str r;
    char buf[] = { str.v.str.expose()[i - 1], '\0' };

    r.type = TYPE_STR;
    r.v.str = str_dup(buf);

    return r;
}

/**** helpers for catching overly large allocations ****/

#define TRY_STREAM enable_stream_exceptions()
#define ENDTRY_STREAM disable_stream_exceptions()

static package
make_space_pack()
{
    if (server_flag_option_cached(SVO_MAX_CONCAT_CATCHABLE))
	return make_error_pack(E_QUOTA);
    else
	return make_abort_pack(ABORT_SECONDS);
}

/**** built in functions ****/

static package
bf_length(const List& arglist, Objid progr)
{
    Var r;
    switch (arglist[1].type) {
    case TYPE_LIST:
	r = Var::new_int(listlength(arglist[1]));
	break;
    case TYPE_MAP:
	r = Var::new_int(maplength(static_cast<const Map&>(arglist[1])));
	break;
    case TYPE_STR:
	r = Var::new_int(memo_strlen(arglist[1].v.str));
	break;
    default:
	free_var(arglist);
	return make_error_pack(E_TYPE);
	break;
    }

    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_setadd(const List& arglist, Objid progr)
{
    List r;
    Var lst = var_ref(arglist[1]);
    Var elt = var_ref(arglist[2]);

    free_var(arglist);

    assert(lst.is_list());
    r = setadd(static_cast<const List&>(lst), elt);

    if (value_bytes(r) <= server_int_option_cached(SVO_MAX_LIST_VALUE_BYTES))
	return make_var_pack(r);
    else {
	free_var(r);
	return make_space_pack();
    }
}

static package
bf_setremove(const List& arglist, Objid progr)
{
    List r;
    Var lst = var_ref(arglist[1]);
    Var elt = var_ref(arglist[2]);

    free_var(arglist);

    assert(lst.is_list());
    r = setremove(static_cast<const List&>(lst), elt);

    if (value_bytes(r) <= server_int_option_cached(SVO_MAX_LIST_VALUE_BYTES))
	return make_var_pack(r);
    else {
	free_var(r);
	return make_space_pack();
    }
}

static package
insert_or_append(const List& arglist, int append)
{
    int pos;
    Var lst = var_ref(arglist[1]);
    Var elt = var_ref(arglist[2]);

    if (arglist.length() == 2) {
	pos = append ? listlength(lst) + 1 : 1;
    } else {
	pos = arglist[3].v.num + append;
	if (pos <= 0)
	    pos = 1;
	else if (pos > listlength(lst) + 1)
	    pos = listlength(lst) + 1;
    }

    free_var(arglist);

    List r = doinsert(static_cast<const List&>(lst), elt, pos);

    if (value_bytes(r) <= server_int_option_cached(SVO_MAX_LIST_VALUE_BYTES))
	return make_var_pack(r);
    else {
	free_var(r);
	return make_space_pack();
    }
}

static package
bf_listappend(const List& arglist, Objid progr)
{
    return insert_or_append(arglist, 1);
}


static package
bf_listinsert(const List& arglist, Objid progr)
{
    return insert_or_append(arglist, 0);
}

static package
bf_listdelete(const List& arglist, Objid progr)
{
    if (arglist[2].v.num <= 0 ||
	arglist[2].v.num > listlength(arglist[1])) {
	free_var(arglist);
	return make_error_pack(E_RANGE);
    }

    Var lst = var_ref(arglist[1]);
    int pos = arglist[2].v.num;

    List r = listdelete(static_cast<const List&>(lst), pos);

    free_var(arglist);

    if (value_bytes(r) <= server_int_option_cached(SVO_MAX_LIST_VALUE_BYTES))
	return make_var_pack(r);
    else {
	free_var(r);
	return make_space_pack();
    }
}

static package
bf_listset(const List& arglist, Objid progr)
{
    Var lst = var_ref(arglist[1]);
    Var elt = var_ref(arglist[2]);
    int pos = arglist[3].v.num;

    free_var(arglist);

    if (pos <= 0 || pos > listlength(lst))
	return make_error_pack(E_RANGE);

    List r = listset(static_cast<const List&>(lst), elt, pos);

    if (value_bytes(r) <= server_int_option_cached(SVO_MAX_LIST_VALUE_BYTES))
	return make_var_pack(r);
    else {
	free_var(r);
	return make_space_pack();
    }
}

static package
bf_equal(const List& arglist, Objid progr)
{
    Var r = Var::new_int(equality(arglist[1], arglist[2], 1));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_strsub(const List& arglist, Objid progr)
{				/* (source, what, with [, case-matters]) */
    int case_matters = 0;
    Stream *s;
    package p;

    if (arglist.length() == 4)
	case_matters = is_true(arglist[4]);
    if (arglist[2].v.str.expose()[0] == '\0') {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    s = new_stream(100);
    TRY_STREAM;
    try {
	Var r;
	stream_add_strsub(s, arglist[1].v.str.expose(),
			  arglist[2].v.str.expose(),
			  arglist[3].v.str.expose(),
			  case_matters);
	r = Var::new_str(stream_contents(s));
	p = make_var_pack(r);
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;
    free_stream(s);
    free_var(arglist);
    return p;
}

static int
signum(int x)
{
    return x < 0 ? -1 : (x > 0 ? 1 : 0);
}

static package
bf_strcmp(const List& arglist, Objid progr)
{				/* (string1, string2) */
    Var r = Var::new_int(signum(strcmp(arglist[1].v.str.expose(), arglist[2].v.str.expose())));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_strtr(const List& arglist, Objid progr)
{				/* (subject, from, to [, case_matters]) */
    Var r;
    int case_matters = 0;

    if (listlength(arglist) > 3)
	case_matters = is_true(arglist[4]);
    r = Var::new_str(strtr(arglist[1].v.str.expose(), memo_strlen(arglist[1].v.str),
			   arglist[2].v.str.expose(), memo_strlen(arglist[2].v.str),
			   arglist[3].v.str.expose(), memo_strlen(arglist[3].v.str),
			   case_matters));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_index(const List& arglist, Objid progr)
{				/* (source, what [, case-matters [, offset]]) */
    Var r;
    int case_matters = 0;
    int offset = 0;

    if (listlength(arglist) > 2)
	case_matters = is_true(arglist[3]);
    if (listlength(arglist) > 3)
	offset = arglist[4].v.num;
    if (offset < 0) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    r = Var::new_int(strindex(arglist[1].v.str.expose() + offset,
			      memo_strlen(arglist[1].v.str) - offset,
			      arglist[2].v.str.expose(),
			      memo_strlen(arglist[2].v.str),
			      case_matters));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_rindex(const List& arglist, Objid progr)
{				/* (source, what [, case-matters [, offset]]) */
    Var r;

    int case_matters = 0;
    int offset = 0;

    if (listlength(arglist) > 2)
	case_matters = is_true(arglist[3]);
    if (listlength(arglist) > 3)
	offset = arglist[4].v.num;
    if (offset > 0) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    r = Var::new_int(strrindex(arglist[1].v.str.expose(),
			       memo_strlen(arglist[1].v.str) + offset,
			       arglist[2].v.str.expose(),
			       memo_strlen(arglist[2].v.str),
			       case_matters));

    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_tostr(const List& arglist, Objid progr)
{
    package p;
    Stream *s = new_stream(100);

    TRY_STREAM;
    try {
	Var r;
	int i;

	for (i = 1; i <= arglist.length(); i++) {
	    stream_add_tostr(s, arglist[i]);
	}
	r = Var::new_str(stream_contents(s));
	p = make_var_pack(r);
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;
    free_stream(s);
    free_var(arglist);
    return p;
}

static package
bf_toliteral(const List& arglist, Objid progr)
{
    package p;
    Stream *s = new_stream(100);

    TRY_STREAM;
    try {
	Var r;

	unparse_value(s, arglist[1]);
	r = Var::new_str(stream_contents(s));
	p = make_var_pack(r);
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;
    free_stream(s);
    free_var(arglist);
    return p;
}

struct pat_cache_entry {
    const char *string;
    int case_matters;
    Pattern pattern;
    struct pat_cache_entry *next;
};

static struct pat_cache_entry *pat_cache;
static struct pat_cache_entry pat_cache_entries[PATTERN_CACHE_SIZE];

static void
setup_pattern_cache()
{
    int i;

    for (i = 0; i < PATTERN_CACHE_SIZE; i++) {
	pat_cache_entries[i].string = 0;
	pat_cache_entries[i].pattern.ptr = 0;
    }

    for (i = 0; i < PATTERN_CACHE_SIZE - 1; i++)
	pat_cache_entries[i].next = &(pat_cache_entries[i + 1]);
    pat_cache_entries[PATTERN_CACHE_SIZE - 1].next = 0;

    pat_cache = &(pat_cache_entries[0]);
}

static Pattern
get_pattern(const char *string, int case_matters)
{
    struct pat_cache_entry *entry, **entry_ptr;

    entry = pat_cache;
    entry_ptr = &pat_cache;

    while (1) {
	if (entry->string && !strcmp(string, entry->string)
	    && case_matters == entry->case_matters) {
	    /* A cache hit; move this entry to the front of the cache. */
	    break;
	} else if (!entry->next) {
	    /* A cache miss; this is the last entry in the cache, so reuse that
	     * one for this pattern, moving it to the front of the cache iff
	     * the compilation succeeds.
	     */
	    if (entry->string) {
		free((void*)entry->string);
		free_pattern(entry->pattern);
	    }
	    entry->pattern = new_pattern(string, case_matters);
	    entry->case_matters = case_matters;
	    if (!entry->pattern.ptr)
		entry->string = 0;
	    else
		entry->string = strdup(string);
	    break;
	} else {
	    /* not done searching the cache... */
	    entry_ptr = &(entry->next);
	    entry = entry->next;
	}
    }

    *entry_ptr = entry->next;
    entry->next = pat_cache;
    pat_cache = entry;
    return entry->pattern;
}

Var
do_match(const List& arglist, int reverse)
{
    const char *subject, *pattern;
    int i;
    Pattern pat;
    Var ans;
    Match_Indices regs[10];

    subject = arglist[1].v.str.expose();
    pattern = arglist[2].v.str.expose();
    pat = get_pattern(pattern, (arglist.length() == 3
				&& is_true(arglist[3])));

    if (!pat.ptr) {
	ans = Var::new_err(E_INVARG);
    } else
	switch (match_pattern(pat, subject, regs, reverse)) {
	default:
	    panic("do_match:  match_pattern returned unfortunate value.\n");
	    /*notreached*/
	case MATCH_SUCCEEDED:
	    ans = new_list(4);
	    ans.v.list[1] = Var::new_int(regs[0].start);
	    ans.v.list[2] = Var::new_int(regs[0].end);
	    ans.v.list[3] = new_list(9);
	    for (i = 1; i <= 9; i++) {
		ans.v.list[3].v.list[i] = new_list(2);
		ans.v.list[3].v.list[i].v.list[1] = Var::new_int(regs[i].start);
		ans.v.list[3].v.list[i].v.list[2] = Var::new_int(regs[i].end);
	    }
	    ans.v.list[4] = var_ref(arglist[1]);
	    break;
	case MATCH_FAILED:
	    ans = new_list(0);
	    break;
	case MATCH_ABORTED:
	    ans = Var::new_err(E_QUOTA);
	    break;
	}

    return ans;
}

static package
bf_match(const List& arglist, Objid progr)
{
    Var ans = do_match(arglist, 0);
    free_var(arglist);
    if (ans.is_err())
	return make_error_pack(ans.v.err);
    else
	return make_var_pack(ans);
}

static package
bf_rmatch(const List& arglist, Objid progr)
{
    Var ans = do_match(arglist, 1);
    free_var(arglist);
    if (ans.is_err())
	return make_error_pack(ans.v.err);
    else
	return make_var_pack(ans);
}

int
invalid_pair(int num1, int num2, int max)
{
    if ((num1 == 0 && num2 == -1)
	|| (num1 > 0 && num2 >= num1 - 1 && num2 <= max))
	return 0;
    else
	return 1;
}

int
check_subs_list(const Var& subs)
{
    if (!subs.is_list() || listlength(subs) != 4
	|| !subs.v.list[1].is_int()
	|| !subs.v.list[2].is_int()
	|| !subs.v.list[3].is_list()
	|| listlength(subs.v.list[3]) != 9
	|| !subs.v.list[4].is_str())
	return 1;

    const ref_ptr<const char>& subj = subs.v.list[4].v.str;
    int subj_length = memo_strlen(subj);
    if (invalid_pair(subs.v.list[1].v.num, subs.v.list[2].v.num,
		     subj_length))
	return 1;

    for (int loop = 1; loop <= 9; loop++) {
	Var pair;
	pair = subs.v.list[3].v.list[loop];
	if (!pair.is_list() || listlength(pair) != 2
	    || !pair.v.list[1].is_int()
	    || !pair.v.list[2].is_int()
	    || invalid_pair(pair.v.list[1].v.num, pair.v.list[2].v.num,
			    subj_length))
	    return 1;
    }
    return 0;
}

static package
bf_substitute(const List& arglist, Objid progr)
{
    int template_length, subject_length;
    const char *_template, *subject;
    Var subs, ans;
    package p;
    Stream *s;
    char c = '\0';

    _template = arglist[1].v.str.expose();
    template_length = memo_strlen(arglist[1].v.str);

    subs = arglist[2];
    if (check_subs_list(subs)) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }

    subject = subs.v.list[4].v.str.expose();
    subject_length = memo_strlen(subs.v.list[4].v.str);

    s = new_stream(template_length);
    TRY_STREAM;
    try {
	while ((c = *(_template++)) != '\0') {
	    if (c != '%')
		stream_add_char(s, c);
	    else if ((c = *(_template++)) == '%')
		stream_add_char(s, '%');
	    else {
		int start = 0, end = 0;
		if (c >= '1' && c <= '9') {
		    Var pair = subs.v.list[3].v.list[c - '0'];
		    start = pair.v.list[1].v.num - 1;
		    end = pair.v.list[2].v.num - 1;
		} else if (c == '0') {
		    start = subs.v.list[1].v.num - 1;
		    end = subs.v.list[2].v.num - 1;
		} else {
		    p = make_error_pack(E_INVARG);
		    goto oops;
		}
		while (start <= end)
		    stream_add_char(s, subject[start++]);
	    }
	}
	ans = Var::new_str(stream_contents(s));
	p = make_var_pack(ans);
      oops: ;
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;
    free_var(arglist);
    free_stream(s);
    return p;
}

static package
bf_value_bytes(const List& arglist, Objid progr)
{
    Var r = Var::new_int(value_bytes(arglist[1]));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_decode_binary(const List& arglist, Objid progr)
{
    int length;
    const char *bytes = binary_to_raw_bytes(arglist[1].v.str.expose(), &length);
    int nargs = arglist.length();
    int fully = (nargs >= 2 && is_true(arglist[2]));
    List r;
    int i;

    free_var(arglist);
    if (!bytes)
	return make_error_pack(E_INVARG);

    if (fully) {
	r = new_list(length);
	for (i = 1; i <= length; i++) {
	    r.v.list[i] = Var::new_int((unsigned char)bytes[i - 1]);
	}
    } else {
	int count, in_string;
	Stream *s = new_stream(50);

	for (count = in_string = 0, i = 0; i < length; i++) {
	    unsigned char c = bytes[i];

	    if (isgraph(c) || c == ' ' || c == '\t') {
		if (!in_string)
		    count++;
		in_string = 1;
	    } else {
		count++;
		in_string = 0;
	    }
	}

	r = new_list(count);
	for (count = 1, in_string = 0, i = 0; i < length; i++) {
	    unsigned char c = bytes[i];

	    if (isgraph(c) || c == ' ' || c == '\t') {
		stream_add_char(s, c);
		in_string = 1;
	    } else {
		if (in_string) {
		    r.v.list[count] = Var::new_str(reset_stream(s));
		    count++;
		}
		r.v.list[count] = Var::new_int(c);
		count++;
		in_string = 0;
	    }
	}

	if (in_string) {
	    r.v.list[count] = Var::new_str(reset_stream(s));
	}
	free_stream(s);
    }

    if (value_bytes(r) <= server_int_option_cached(SVO_MAX_LIST_VALUE_BYTES))
	return make_var_pack(r);
    else {
	free_var(r);
	return make_space_pack();
    }
}

static int
encode_binary(Stream * s, const Var& v)
{
    int i;

    switch (v.type) {
    case TYPE_INT:
	if (v.v.num < 0 || v.v.num >= 256)
	    return 0;
	stream_add_char(s, (char) v.v.num);
	break;
    case TYPE_STR:
	stream_add_string(s, v.v.str.expose());
	break;
    case TYPE_LIST:
	for (i = 1; i <= v.v.list[0].v.num; i++)
	    if (!encode_binary(s, v.v.list[i]))
		return 0;
	break;
    default:
	return 0;
    }

    return 1;
}

static package
bf_encode_binary(const List& arglist, Objid progr)
{
    Var r;
    package p;
    Stream *s = new_stream(100);
    Stream *s2 = new_stream(100);

    TRY_STREAM;
    try {
	if (encode_binary(s, arglist)) {
	    stream_add_raw_bytes_to_binary(
		s2, stream_contents(s), stream_length(s));
	    r = Var::new_str(stream_contents(s2));
	    p = make_var_pack(r);
	}
	else
	    p = make_error_pack(E_INVARG);
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;
    free_stream(s2);
    free_stream(s);
    free_var(arglist);
    return p;
}

void
register_list(void)
{
    register_function("value_bytes", 1, 1, bf_value_bytes, TYPE_ANY);

    register_function("decode_binary", 1, 2, bf_decode_binary,
		      TYPE_STR, TYPE_ANY);
    register_function("encode_binary", 0, -1, bf_encode_binary);
    /* list */
    register_function("length", 1, 1, bf_length, TYPE_ANY);
    register_function("setadd", 2, 2, bf_setadd, TYPE_LIST, TYPE_ANY);
    register_function("setremove", 2, 2, bf_setremove, TYPE_LIST, TYPE_ANY);
    register_function("listappend", 2, 3, bf_listappend,
		      TYPE_LIST, TYPE_ANY, TYPE_INT);
    register_function("listinsert", 2, 3, bf_listinsert,
		      TYPE_LIST, TYPE_ANY, TYPE_INT);
    register_function("listdelete", 2, 2, bf_listdelete, TYPE_LIST, TYPE_INT);
    register_function("listset", 3, 3, bf_listset,
		      TYPE_LIST, TYPE_ANY, TYPE_INT);
    register_function("equal", 2, 2, bf_equal, TYPE_ANY, TYPE_ANY);

    /* string */
    register_function("tostr", 0, -1, bf_tostr);
    register_function("toliteral", 1, 1, bf_toliteral, TYPE_ANY);
    setup_pattern_cache();
    register_function("match", 2, 3, bf_match, TYPE_STR, TYPE_STR, TYPE_ANY);
    register_function("rmatch", 2, 3, bf_rmatch, TYPE_STR, TYPE_STR, TYPE_ANY);
    register_function("substitute", 2, 2, bf_substitute, TYPE_STR, TYPE_LIST);
    register_function("index", 2, 4, bf_index,
		      TYPE_STR, TYPE_STR, TYPE_ANY, TYPE_INT);
    register_function("rindex", 2, 4, bf_rindex,
		      TYPE_STR, TYPE_STR, TYPE_ANY, TYPE_INT);
    register_function("strcmp", 2, 2, bf_strcmp, TYPE_STR, TYPE_STR);
    register_function("strsub", 3, 4, bf_strsub,
		      TYPE_STR, TYPE_STR, TYPE_STR, TYPE_ANY);
    register_function("strtr", 3, 4, bf_strtr,
		      TYPE_STR, TYPE_STR, TYPE_STR, TYPE_ANY);
}
