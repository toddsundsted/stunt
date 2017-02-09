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

/*
 *  The server_version is a character string containing three decimal numbers
 *  separated by periods:
 *
 *      <major>.<minor>.<release>
 *
 *  The major version number changes very slowly, only when existing MOO code
 *  might stop working, due to an incompatible change in the syntax or
 *  semantics of the programming language, or when an incompatible change is
 *  made to the database format.
 *
 *  The minor version number changes more quickly, whenever an upward-
 *  compatible change is made in the programming language syntax or semantics.
 *  The most common cause of this is the addition of a new kind of expression,
 *  statement, or built-in function.
 *
 *  The release version number changes as frequently as bugs are fixed in the
 *  server code.  Changes in the release number indicate changes that should
 *  only be visible to users as bug fixes, if at all.
 *
 */

#include "config.h"
#include "version.h"
#include "structures.h"
#include "list.h"
#include "storage.h"
#include "utils.h"
#include "options.h"
#include "server.h"

/*
 * Server executable version
 */

#include "version_src.h"
#ifndef VERSION_MAJOR
#  define VERSION_MAJOR 1
#  define VERSION_MINOR 8
#  define VERSION_RELEASE 3
#endif
#ifndef VERSION_EXT
#  define VERSION_EXT "+?_ad_hoc_??"
#endif
#ifndef VERSION_STRING
#  define _V(MAJ,MIN,REL) #MAJ "." #MIN "." #REL
#  define _V2(_1,_2,_3) _V(_1,_2,_3)
#  define VERSION_STRING _V2(VERSION_MAJOR,VERSION_MINOR,VERSION_RELEASE) VERSION_EXT
#endif
#ifndef VERSION_SOURCE
#  define VERSION_SOURCE(DEF) DEF(vcs,"unknown")
#endif

const char *server_version = VERSION_STRING;

static ref_ptr<Var> version_structure;

static void init_version_structure()
{

#define SET_INT(W,value) (W) = Var::new_int(value)
#define SET_STR(W,value) (W) = Var::new_str(value)
#define SET_OBJ(W,value) (W) = Var::new_obj(value)
#define SET_VAR(W,value) (W) = var_ref(value)

#define DEPTH 4

    List stack[DEPTH];
    List* the_list = stack;

#define BEGIN_LIST(n)				\
    if (++the_list - stack >= DEPTH)		\
	panic("init_version_structure:  push");	\
    the_list[0] = new_list(n)

#define END_LIST()				\
    if (the_list-- == stack)			\
	panic("init_version_structure:  pop");	\
    the_list[0] = listappend(the_list[0],the_list[1])

#define BEGIN_GROUP(name)			\
    BEGIN_LIST(1);				\
    SET_STR(the_list[0][1],#name);		\
    BEGIN_LIST(0)

#define END_GROUP()   END_LIST(); END_LIST()

    Var item;

#define PUSH_VALUE(WHAT,value)			\
    SET_##WHAT(item,value);			\
    the_list[0] = listappend(the_list[0], item);

    List list;

#define PUSH_PAIR(name,WHAT,value)		\
    list = new_list(2);				\
    SET_STR(list[1],name);			\
    SET_##WHAT(list[2],value);			\
    the_list[0] = listappend(the_list[0], list);

    /* create non-string/int true and false values */
    Var falsev;
    List truev = new_list(1);
    SET_INT(truev[1],0);
    SET_OBJ(falsev,-1);

    the_list[0] = new_list(0);
    PUSH_PAIR("major",INT,VERSION_MAJOR);
    PUSH_PAIR("minor",INT,VERSION_MINOR);
    PUSH_PAIR("release",INT,VERSION_RELEASE);
    PUSH_PAIR("ext",STR,VERSION_EXT);
    PUSH_PAIR("string",STR,server_version);

    BEGIN_GROUP(features);
#define _FDEF(name) PUSH_VALUE(STR,#name)
#ifdef VERSION_FEATURES
    VERSION_FEATURES(_FDEF);
#endif
    END_GROUP();

    BEGIN_GROUP(options);
#define _DINT(name,value) PUSH_PAIR(name,INT,value)
#define _DSTR(name,value) PUSH_PAIR(name,STR,value)
#define _DDEF(name)       PUSH_PAIR(name,VAR,truev)
#define _DNDEF(name)      PUSH_PAIR(name,VAR,falsev)
#include "version_options.h"
    END_GROUP();

#ifdef VERSION_MAKEVARS
    BEGIN_GROUP(make);
#define _MDEF(name,value) PUSH_PAIR(#name,STR,value)
    VERSION_MAKEVARS(_MDEF);
    END_GROUP();
#endif

    BEGIN_GROUP(source);
#define _SDEF(name,value) PUSH_PAIR(#name,STR,value)
#ifdef VERSION_SOURCE
    VERSION_SOURCE(_SDEF);
#endif
    END_GROUP();

    if (stack != the_list)
	panic("init_version_structure: unpopped stuff");

    free_var(truev);
    free_var(falsev);

    version_structure = the_list[0].v.list;
}

Var
server_version_full(const Var& arg)
{
    Var r;
    const char *s;
    Var *tree;
    if (!version_structure) {
	init_version_structure();
    }
    if (!arg.is_str() || arg.v.str.expose()[0] == '\0' ) {
	r.type   = TYPE_LIST;
	r.v.list = version_structure;
	return var_ref(r);
    }
    s = arg.v.str.expose();
    tree = version_structure.expose();
    for (;;) {
	/* invariants:
	 *   s is a nonempty string;
	 *   tree has at least one string or {string,_} pair
	 */
	int i = tree[0].v.num;
	const char *e = s;
	while (*e != '/' && *++e != '\0');
	do {
	    --i; ++tree;
	    switch (tree[0].type) {
	    default:
		break;
	    case TYPE_STR:
		{
		    Str& str = static_cast<Str&>(tree[0]);
		    if ((memo_strlen(str.v.str) == e - s) && strncmp(str.v.str.expose(), s, e - s) == 0)
			goto found;
		    break;
		}
	    case TYPE_LIST:
		{
		    List& list = static_cast<List&>(tree[0]);
		    if (list.length() == 2 && list[1].is_str() && memo_strlen(list[1].v.str) == e - s && strncmp(list[1].v.str.expose(), s, e - s) == 0) {
			if (list.length() > 1)
			    tree = list.v.list.expose() + 2;
			else
			    tree = list.v.list.expose() + 1;
			goto found;
		    }
		    break;
		}
	    }
	} while (i > 0);
	break;
      found:
	s = (*e != '\0' ? e+1 : e); /* skip trailing slash */
	if (*s == '\0')
	    return var_ref(tree[0]);
	if (!tree[0].is_list())
	    break;
	tree = tree[0].v.list.expose();
	if (tree[0].v.num <= 0 || (!tree[1].is_str() && !tree[1].is_list()))
	    break;
    }
    r = Var::new_err(E_INVARG);
    return r;
}

/*
 * Database format version
 */

int
check_db_version(DB_Version version)
{
    return version < Num_DB_Versions;
}
