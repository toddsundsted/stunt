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

/*****************************************************************************
 * Routines for manipulating verbs on DB objects
 *****************************************************************************/

#include <ctype.h>
#include "my-stdlib.h"
#include "my-string.h"

#include "config.h"
#include "db.h"
#include "db_private.h"
#include "parse_cmd.h"
#include "program.h"
#include "storage.h"
#include "utils.h"


/*********** Prepositions ***********/

#define MAXPPHRASE 3		/* max. number of words in a prepositional phrase */

/* NOTE: New prepositional phrases should only be added to this list at the
 * end, never in the middle, and no entries should every be removed from this
 * list; the list indices are stored raw in the DB file.
 */
static const char *prep_list[] =
{
    "with/using",
    "at/to",
    "in front of",
    "in/inside/into",
    "on top of/on/onto/upon",
    "out of/from inside/from",
    "over",
    "through",
    "under/underneath/beneath",
    "behind",
    "beside",
    "for/about",
    "is",
    "as",
    "off/off of",
};

#define NPREPS Arraysize(prep_list)

typedef struct pt_entry {
    int nwords;
    char *words[MAXPPHRASE];
    struct pt_entry *next;
} pt_entry;

struct pt_entry *prep_table[NPREPS];

void
dbpriv_build_prep_table(void)
{
    int i, j;
    int nwords;
    char **words;
    char cprep[100];
    const char *p;
    char *t;
    pt_entry *current_alias, **prev;

    for (i = 0; i < NPREPS; i++) {
	p = prep_list[i];
	prev = &prep_table[i];
	while (*p) {
	    t = cprep;
	    if (*p == '/')
		p++;
	    while (*p && *p != '/')
		*t++ = *p++;
	    *t = '\0';

	    /* This call to PARSE_INTO_WORDS() is the reason that this function
	     * is called from DB_INITIALIZE() instead of from the first call to
	     * DB_FIND_PREP(), below.  You see, PARSE_INTO_WORDS() isn't
	     * re-entrant, and DB_FIND_PREP() is called from code that's still
	     * using the results of a previous call to PARSE_INTO_WORDS()...
	     */
	    words = parse_into_words(cprep, &nwords);

	    current_alias = mymalloc(sizeof(struct pt_entry), M_PREP);
	    current_alias->nwords = nwords;
	    current_alias->next = 0;
	    for (j = 0; j < nwords; j++)
		current_alias->words[j] = str_dup(words[j]);
	    *prev = current_alias;
	    prev = &current_alias->next;
	}
    }
}

db_prep_spec
db_find_prep(int argc, char *argv[], int *first, int *last)
{
    pt_entry *alias;
    int i, j, k;
    int exact_match = (first == 0 || last == 0);

    for (i = 0; i < argc; i++) {
	for (j = 0; j < NPREPS; j++) {
	    for (alias = prep_table[j]; alias; alias = alias->next) {
		if (i + alias->nwords <= argc) {
		    for (k = 0; k < alias->nwords; k++) {
			if (mystrcasecmp(argv[i + k], alias->words[k]))
			    break;
		    }
		    if (k == alias->nwords
			&& (!exact_match || i + k == argc)) {
			if (!exact_match) {
			    *first = i;
			    *last = i + alias->nwords - 1;
			}
			return (db_prep_spec) j;
		    }
		}
	    }
	}
	if (exact_match)
	    break;
    }
    return PREP_NONE;
}

db_prep_spec
db_match_prep(const char *prepname)
{
    db_prep_spec prep;
    int argc;
    char *ptr;
    char **argv;
    char *s, first;

    s = str_dup(prepname);
    first = s[0];
    if (first == '#')
	first = (++s)[0];
    prep = strtol(s, &ptr, 10);
    if (*ptr == '\0') {
	free_str(s);
	if (!isdigit(first) || prep >= NPREPS)
	    return PREP_NONE;
	else
	    return prep;
    }
    if ((ptr = strchr(s, '/')) != '\0')
	*ptr = '\0';

    argv = parse_into_words(s, &argc);
    prep = db_find_prep(argc, argv, 0, 0);
    free_str(s);
    return prep;
}

const char *
db_unparse_prep(db_prep_spec prep)
{
    if (prep == PREP_NONE)
	return "none";
    else if (prep == PREP_ANY)
	return "any";
    else
	return prep_list[prep];
}


/*********** Verbs ***********/

#define DOBJSHIFT  4
#define IOBJSHIFT  6
#define OBJMASK    0x3
#define PERMMASK   0xF

void
db_add_verb(Objid oid, const char *vnames, Objid owner, unsigned flags,
	    db_arg_spec dobj, db_prep_spec prep, db_arg_spec iobj)
{
    Object *o = dbpriv_find_object(oid);
    Verbdef *v, *newv;

    newv = mymalloc(sizeof(Verbdef), M_VERBDEF);
    newv->name = vnames;
    newv->owner = owner;
    newv->perms = flags | (dobj << DOBJSHIFT) | (iobj << IOBJSHIFT);
    newv->prep = prep;
    newv->next = 0;
    newv->program = 0;
    if (o->verbdefs) {
	for (v = o->verbdefs; v->next; v = v->next);
	v->next = newv;
    } else
	o->verbdefs = newv;
}

static Verbdef *
find_verbdef_by_name(Object * o, const char *vname, int check_x_bit)
{
    Verbdef *v;

    for (v = o->verbdefs; v; v = v->next)
	if (verbcasecmp(v->name, vname)
	    && (!check_x_bit || (v->perms & VF_EXEC)))
	    break;

    return v;
}

int
db_count_verbs(Objid oid)
{
    int count = 0;
    Object *o = dbpriv_find_object(oid);
    Verbdef *v;

    for (v = o->verbdefs; v; v = v->next)
	count++;

    return count;
}

int
db_for_all_verbs(Objid oid,
		 int (*func) (void *data, const char *vname),
		 void *data)
{
    Object *o = dbpriv_find_object(oid);
    Verbdef *v;

    for (v = o->verbdefs; v; v = v->next)
	if ((*func) (data, v->name))
	    return 1;

    return 0;
}

typedef struct {		/* Non-null db_verb_handles point to these */
    Objid definer;
    Verbdef *verbdef;
} handle;

void
db_delete_verb(db_verb_handle vh)
{
    handle *h = (handle *) vh.ptr;
    Objid oid = h->definer;
    Verbdef *v = h->verbdef;
    Object *o = dbpriv_find_object(oid);
    Verbdef *vv;

    vv = o->verbdefs;
    if (vv == v)
	o->verbdefs = v->next;
    else {
	while (vv->next != v)
	    vv = vv->next;
	vv->next = v->next;
    }

    if (v->program)
	free_program(v->program);
    if (v->name)
	free_str(v->name);
    myfree(v, M_VERBDEF);
}

db_verb_handle
db_find_command_verb(Objid oid, const char *verb,
		     db_arg_spec dobj, unsigned prep, db_arg_spec iobj)
{
    Object *o;
    Verbdef *v;
    static handle h;
    db_verb_handle vh;

    for (o = dbpriv_find_object(oid); o; o = dbpriv_find_object(o->parent))
	for (v = o->verbdefs; v; v = v->next) {
	    db_arg_spec vdobj = (v->perms >> DOBJSHIFT) & OBJMASK;
	    db_arg_spec viobj = (v->perms >> IOBJSHIFT) & OBJMASK;

	    if (verbcasecmp(v->name, verb)
		&& (vdobj == ASPEC_ANY || vdobj == dobj)
		&& (v->prep == PREP_ANY || v->prep == prep)
		&& (viobj == ASPEC_ANY || viobj == iobj)) {
		h.definer = o->id;
		h.verbdef = v;
		vh.ptr = &h;

		return vh;
	    }
	}

    vh.ptr = 0;

    return vh;
}

db_verb_handle
db_find_callable_verb(Objid oid, const char *verb)
{
    Object *o;
    Verbdef *v;
    static handle h;
    db_verb_handle vh;

    for (o = dbpriv_find_object(oid); o; o = dbpriv_find_object(o->parent))
	if ((v = find_verbdef_by_name(o, verb, 1)) != 0) {
	    h.definer = o->id;
	    h.verbdef = v;
	    vh.ptr = &h;

	    return vh;
	}
    vh.ptr = 0;

    return vh;
}

db_verb_handle
db_find_defined_verb(Objid oid, const char *vname, int allow_numbers)
{
    Object *o = dbpriv_find_object(oid);
    Verbdef *v;
    char *p;
    int num, i;
    static handle h;
    db_verb_handle vh;

    if (!allow_numbers ||
	(num = strtol(vname, &p, 10),
	 (isspace(*vname) || *p != '\0')))
	num = -1;

    for (i = 0, v = o->verbdefs; v; v = v->next, i++)
	if (i == num || verbcasecmp(v->name, vname))
	    break;

    if (v) {
	h.definer = o->id;
	h.verbdef = v;
	vh.ptr = &h;

	return vh;
    }
    vh.ptr = 0;

    return vh;
}

db_verb_handle
db_find_indexed_verb(Objid oid, unsigned index)
{
    Object *o = dbpriv_find_object(oid);
    Verbdef *v;
    unsigned i;
    static handle h;
    db_verb_handle vh;

    for (v = o->verbdefs, i = 0; v; v = v->next)
	if (++i == index) {
	    h.definer = o->id;
	    h.verbdef = v;
	    vh.ptr = &h;

	    return vh;
	}
    vh.ptr = 0;

    return vh;
}

Objid
db_verb_definer(db_verb_handle vh)
{
    handle *h = (handle *) vh.ptr;

    if (h)
	return h->definer;

    panic("DB_VERB_DEFINER: Null handle!");
    return 0;
}

const char *
db_verb_names(db_verb_handle vh)
{
    handle *h = (handle *) vh.ptr;

    if (h)
	return h->verbdef->name;

    panic("DB_VERB_NAMES: Null handle!");
    return 0;
}

void
db_set_verb_names(db_verb_handle vh, const char *names)
{
    handle *h = (handle *) vh.ptr;

    if (h) {
	if (h->verbdef->name)
	    free_str(h->verbdef->name);
	h->verbdef->name = names;
    } else
	panic("DB_SET_VERB_NAMES: Null handle!");
}

Objid
db_verb_owner(db_verb_handle vh)
{
    handle *h = (handle *) vh.ptr;

    if (h)
	return h->verbdef->owner;

    panic("DB_VERB_OWNER: Null handle!");
    return 0;
}

void
db_set_verb_owner(db_verb_handle vh, Objid owner)
{
    handle *h = (handle *) vh.ptr;

    if (h)
	h->verbdef->owner = owner;
    else
	panic("DB_SET_VERB_OWNER: Null handle!");
}

unsigned
db_verb_flags(db_verb_handle vh)
{
    handle *h = (handle *) vh.ptr;

    if (h)
	return h->verbdef->perms & PERMMASK;

    panic("DB_VERB_FLAGS: Null handle!");
    return 0;
}

void
db_set_verb_flags(db_verb_handle vh, unsigned flags)
{
    handle *h = (handle *) vh.ptr;

    if (h) {
	h->verbdef->perms &= ~PERMMASK;
	h->verbdef->perms |= flags;
    } else
	panic("DB_SET_VERB_FLAGS: Null handle!");
}

Program *
db_verb_program(db_verb_handle vh)
{
    handle *h = (handle *) vh.ptr;

    if (h) {
	Program *p = h->verbdef->program;

	return p ? p : null_program();
    }
    panic("DB_VERB_PROGRAM: Null handle!");
    return 0;
}

void
db_set_verb_program(db_verb_handle vh, Program * program)
{
    handle *h = (handle *) vh.ptr;

    if (h) {
	if (h->verbdef->program)
	    free_program(h->verbdef->program);
	h->verbdef->program = program;
    } else
	panic("DB_SET_VERB_PROGRAM: Null handle!");
}

void
db_verb_arg_specs(db_verb_handle vh,
	     db_arg_spec * dobj, db_prep_spec * prep, db_arg_spec * iobj)
{
    handle *h = (handle *) vh.ptr;

    if (h) {
	*dobj = (h->verbdef->perms >> DOBJSHIFT) & OBJMASK;
	*prep = h->verbdef->prep;
	*iobj = (h->verbdef->perms >> IOBJSHIFT) & OBJMASK;
    } else
	panic("DB_VERB_ARG_SPECS: Null handle!");
}

void
db_set_verb_arg_specs(db_verb_handle vh,
		   db_arg_spec dobj, db_prep_spec prep, db_arg_spec iobj)
{
    handle *h = (handle *) vh.ptr;

    if (h) {
	h->verbdef->perms = ((h->verbdef->perms & PERMMASK)
			     | (dobj << DOBJSHIFT)
			     | (iobj << IOBJSHIFT));
	h->verbdef->prep = prep;
    } else
	panic("DB_SET_VERB_ARG_SPECS: Null handle!");
}

int
db_verb_allows(db_verb_handle h, Objid progr, db_verb_flag flag)
{
    return ((db_verb_flags(h) & flag)
	    || progr == db_verb_owner(h)
	    || is_wizard(progr));
}


char rcsid_db_verbs[] = "$Id: db_verbs.c,v 1.2 1997/03/03 04:18:31 nop Exp $";

/* $Log: db_verbs.c,v $
/* Revision 1.2  1997/03/03 04:18:31  nop
/* GNU Indent normalization
/*
 * Revision 1.1.1.1  1997/03/03 03:44:59  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.4  1996/05/12  21:32:23  pavel
 * Changed db_add_verb() not to bump the reference count of the given
 * verb-names string.  Release 1.8.0p5.
 *
 * Revision 2.3  1996/02/08  07:17:40  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.2  1995/12/28  00:29:03  pavel
 * Broke out the building of the preposition table into a separate function,
 * called during initialization, to avoid an storage-overwriting bug.  Changed
 * db_delete_verb() to unbundle how the verb is found.  Changed
 * db_find_defined_verb() to allow suppressing old numeric-string behavior.
 * Release 1.8.0alpha3.
 *
 * Revision 2.1  1995/12/11  08:11:07  pavel
 * Made verb programs never be NULL any more.  Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:21:33  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  04:21:20  pavel
 * Initial revision
 */
