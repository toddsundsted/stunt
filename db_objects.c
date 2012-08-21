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
 * Routines for manipulating DB objects
 *****************************************************************************/

#include "my-string.h"

#include "config.h"
#include "db.h"
#include "db_private.h"
#include "list.h"
#include "program.h"
#include "storage.h"
#include "utils.h"
#include "xtrapbits.h"

static Object **objects;
static int num_objects = 0;
static int max_objects = 0;

static unsigned int nonce = 0;

static Var all_users;

/* used in graph traversals */
static unsigned char *bit_array;
static size_t array_size = 0;

#define ARRAY_SIZE_IN_BYTES (array_size / 8)
#define CLEAR_BIT_ARRAY() memset(bit_array, 0, ARRAY_SIZE_IN_BYTES)


/*********** Objects qua objects ***********/

Object *
dbpriv_find_object(Objid oid)
{
    if (oid < 0 || oid >= num_objects)
	return 0;
    else
	return objects[oid];
}

int
valid(Objid oid)
{
    return dbpriv_find_object(oid) != 0;
}

Objid
db_last_used_objid(void)
{
    return num_objects - 1;
}

void
db_reset_last_used_objid(void)
{
    while (!objects[num_objects - 1])
	num_objects--;
}

void
db_set_last_used_objid(Objid oid)
{
    while (!objects[num_objects - 1] && num_objects > oid)
	num_objects--;
}

static void
ensure_new_object(void)
{
    if (max_objects == 0) {
	max_objects = 128;
	objects = mymalloc(max_objects * sizeof(Object *), M_OBJECT_TABLE);
    }
    if (num_objects >= max_objects) {
	int i;
	Object **new;

	new = mymalloc(max_objects * 2 * sizeof(Object *), M_OBJECT_TABLE);
	for (i = 0; i < max_objects; i++)
	    new[i] = objects[i];
	myfree(objects, M_OBJECT_TABLE);
	objects = new;
	max_objects *= 2;
    }

    if (array_size == 0) {
	array_size = 4096;
	bit_array = mymalloc(ARRAY_SIZE_IN_BYTES * sizeof(unsigned char), M_ARRAY);
    }
    if (num_objects >= array_size) {
	myfree(bit_array, M_ARRAY);
	bit_array = mymalloc(ARRAY_SIZE_IN_BYTES * 2 * sizeof(unsigned char), M_ARRAY);
	array_size *= 2;
    }
}

void
dbpriv_assign_nonce(Object *o)
{
    o->nonce = nonce++;
}

Object *
dbpriv_new_object(void)
{
    Object *o;

    ensure_new_object();
    o = objects[num_objects] = mymalloc(sizeof(Object), M_OBJECT);
    o->id = num_objects;
    num_objects++;

    dbpriv_assign_nonce(o);

    return o;
}

void
dbpriv_new_recycled_object(void)
{
    ensure_new_object();
    objects[num_objects++] = 0;
}

Objid
db_create_object(void)
{
    Object *o;
    Objid oid;

    o = dbpriv_new_object();
    oid = o->id;

    o->name = str_dup("");
    o->flags = 0;

    o->parents = var_ref(nothing);
    o->children = new_list(0);

    o->location = var_ref(nothing);
    o->contents = new_list(0);

    o->propval = 0;

    o->propdefs.max_length = 0;
    o->propdefs.cur_length = 0;
    o->propdefs.l = 0;

    o->verbdefs = 0;

    return oid;
}

void
db_destroy_object(Objid oid)
{
    Object *o = dbpriv_find_object(oid);
    Verbdef *v, *w;
    int i;

    db_priv_affected_callable_verb_lookup();

    if (!o)
	panic("DB_DESTROY_OBJECT: Invalid object!");

    if (o->location.v.obj != NOTHING ||
	o->contents.v.list[0].v.num != 0 ||
	(o->parents.type == TYPE_OBJ && o->parents.v.obj != NOTHING) ||
	(o->parents.type == TYPE_LIST && o->parents.v.list[0].v.num != 0) ||
	o->children.v.list[0].v.num != 0)
	panic("DB_DESTROY_OBJECT: Not a barren orphan!");

    free_var(o->parents);
    free_var(o->children);

    free_var(o->location);
    free_var(o->contents);

    if (is_user(oid)) {
	Var t;

	t.type = TYPE_OBJ;
	t.v.obj = oid;
	all_users = setremove(all_users, t);
    }
    free_str(o->name);

    for (i = 0; i < o->propdefs.cur_length; i++) {
	/* As an orphan, the only properties on this object are the ones
	 * defined on it directly, so these two arrays must be the same length.
	 */
	free_str(o->propdefs.l[i].name);
	free_var(o->propval[i].var);
    }
    if (o->propval)
	myfree(o->propval, M_PVAL);
    if (o->propdefs.l)
	myfree(o->propdefs.l, M_PROPDEF);

    for (v = o->verbdefs; v; v = w) {
	if (v->program)
	    free_program(v->program);
	free_str(v->name);
	w = v->next;
	myfree(v, M_VERBDEF);
    }

    myfree(objects[oid], M_OBJECT);
    objects[oid] = 0;
}

void *
db_make_anonymous(Objid oid, Objid last)
{
    Object *o = objects[oid];
    Var old_parents = o->parents;
    Var me = new_obj(oid);

    Var parent;
    int i, c;

    /* remove me from my old parents' children */
    if (old_parents.type == TYPE_OBJ && old_parents.v.obj != NOTHING)
	objects[old_parents.v.obj]->children = setremove(objects[old_parents.v.obj]->children, me);
    else if (old_parents.type == TYPE_LIST)
	FOR_EACH(parent, old_parents, i, c)
	    objects[parent.v.obj]->children = setremove(objects[parent.v.obj]->children, me);

    objects[oid] = 0;
    db_set_last_used_objid(last);

    o->id = NOTHING;

    free_var(o->children);
    free_var(o->location);
    free_var(o->contents);

    /* Last step, reallocate the memory and copy -- anonymous objects
     * require space for reference counting.
     */
    Object *t = mymalloc(sizeof(Object), M_ANON);
    memcpy(t, o, sizeof(Object));
    myfree(o, M_OBJECT);

    return t;
}

void
db_destroy_anonymous_object(void *obj)
{
    Object *o = (Object *)obj;

    free_str(o->name);
    o->name = NULL;

    free_var(o->parents);

    if (o->propval)
	myfree(o->propval, M_PVAL);

    myfree(o, M_ANON);
}

void
db_invalidate_anonymous_object(void *obj)
{
    Object *o = (Object *)obj;

    dbpriv_set_object_flag(o, FLAG_INVALID);
}

int
parents_ok(Object *o)
{
    if (TYPE_LIST == o->parents.type) {
	Var parent;
	int i, c;
	FOR_EACH(parent, o->parents, i, c) {
	    Objid oid = parent.v.obj;
	    if (!valid(oid) || objects[oid]->nonce > o->nonce) {
		dbpriv_set_object_flag(o, FLAG_INVALID);
		return 0;
	    }
	}
    }
    else {
	Objid oid = o->parents.v.obj;
	if (!valid(oid) || objects[oid]->nonce > o->nonce) {
	    dbpriv_set_object_flag(o, FLAG_INVALID);
	    return 0;
	}
    }

    return 1;
}

int
anon_valid(Object *o)
{
    return o
           && !dbpriv_object_has_flag(o, FLAG_INVALID)
           && parents_ok(o);
}

int
is_valid(Var obj)
{
    return (TYPE_ANON == obj.type) ?
           anon_valid(obj.v.anon) :
           valid(obj.v.obj);
}

Objid
db_renumber_object(Objid old)
{
    Objid new;
    Object *o;

    db_priv_affected_callable_verb_lookup();

    for (new = 0; new < old; new++) {
	if (objects[new] == NULL) {
	    /* Change the identity of the object. */
	    o = objects[new] = objects[old];
	    objects[old] = 0;
	    objects[new]->id = new;

	    /* Fix up the parents/children hierarchy and the
	     * location/contents hierarchy.
	     */
	    int i1, c1, i2, c2;
	    Var obj1, obj2;

#define	    FIX(up, down)							\
	    if (TYPE_LIST == o->up.type) {					\
		FOR_EACH(obj1, o->up, i1, c1) {					\
		    FOR_EACH(obj2, objects[obj1.v.obj]->down, i2, c2)		\
			if (obj2.v.obj == old)					\
			    break;						\
		    objects[obj1.v.obj]->down.v.list[i2].v.obj = new;		\
		}								\
	    }									\
	    else if (TYPE_OBJ == o->up.type && NOTHING != o->up.v.obj) {	\
		FOR_EACH(obj1, objects[o->up.v.obj]->down, i2, c2)		\
		if (obj1.v.obj == old)						\
		    break;							\
		objects[o->up.v.obj]->down.v.list[i2].v.obj = new;		\
	    }									\
	    FOR_EACH(obj1, o->down, i1, c1) {					\
		if (TYPE_LIST == objects[obj1.v.obj]->up.type) {		\
		    FOR_EACH(obj2, objects[obj1.v.obj]->up, i2, c2)		\
			if (obj2.v.obj == old)					\
			    break;						\
		    objects[obj1.v.obj]->up.v.list[i2].v.obj = new;		\
		}								\
		else {								\
		    objects[obj1.v.obj]->up.v.obj = new;			\
		}								\
	    }

	    FIX(parents, children);
	    FIX(location, contents);

#undef	    FIX

	    /* Fix up the list of users, if necessary */
	    if (is_user(new)) {
		int i;

		for (i = 1; i <= all_users.v.list[0].v.num; i++)
		    if (all_users.v.list[i].v.obj == old) {
			all_users.v.list[i].v.obj = new;
			break;
		    }
	    }
	    /* Fix the owners of verbs, properties and objects */
	    {
		Objid oid;

		for (oid = 0; oid < num_objects; oid++) {
		    Object *o = objects[oid];
		    Verbdef *v;
		    Pval *p;
		    int i, count;

		    if (!o)
			continue;

		    if (o->owner == new)
			o->owner = NOTHING;
		    else if (o->owner == old)
			o->owner = new;

		    for (v = o->verbdefs; v; v = v->next)
			if (v->owner == new)
			    v->owner = NOTHING;
			else if (v->owner == old)
			    v->owner = new;

		    count = dbpriv_count_properties(oid);
		    p = o->propval;
		    for (i = 0; i < count; i++)
			if (p[i].owner == new)
			    p[i].owner = NOTHING;
			else if (p[i].owner == old)
			    p[i].owner = new;
		}
	    }

	    return new;
	}
    }

    /* There are no recycled objects less than `old', so keep its number. */
    return old;
}

int
db_object_bytes(Objid oid)
{
    Object *o = objects[oid];
    int i, len, count;
    Verbdef *v;

    count = sizeof(Object) + sizeof(Object *);
    count += memo_strlen(o->name) + 1;

    for (v = o->verbdefs; v; v = v->next) {
	count += sizeof(Verbdef);
	count += memo_strlen(v->name) + 1;
	if (v->program)
	    count += program_bytes(v->program);
    }

    count += sizeof(Propdef) * o->propdefs.cur_length;
    for (i = 0; i < o->propdefs.cur_length; i++)
	count += memo_strlen(o->propdefs.l[i].name) + 1;

    len = dbpriv_count_properties(oid);
    count += (sizeof(Pval) - sizeof(Var)) * len;
    for (i = 0; i < len; i++)
	count += value_bytes(o->propval[i].var);

    return count;
}


/* Traverse the tree/graph twice.  First to count the maximal number
 * of members, and then to copy the members.  Use the bit array to
 * mark objects that have been copied, to prevent double copies.
 * Note: the final step forcibly sets the length of the list, which
 * may be less than the allocated length.  It's possible to calculate
 * the correct length, but that would require one more round of bit
 * array bookkeeping.
 */

#define DEFUNC(name, field)						\
static									\
int32 db1_##name(Object *o)						\
{									\
    int i, c;								\
    Var tmp, field = enlist_var(var_ref(o->field));			\
    Object *o2;								\
    Objid oid;								\
    int32 n = 0;							\
									\
    FOR_EACH(tmp, field, i, c) {					\
	oid = tmp.v.obj;						\
	if (valid(oid)) {						\
	    o2 = dbpriv_find_object(oid);				\
	    n += db1_##name(o2) + 1;					\
	}								\
    }									\
									\
    free_var(field);							\
									\
    return n;								\
}									\
static									\
void db2_##name(Object *o, Var *plist, int *px)				\
{									\
    int i, c;								\
    Var tmp, field = enlist_var(var_ref(o->field));			\
    Object *o2;								\
    Objid oid;								\
									\
    FOR_EACH(tmp, field, i, c) {					\
	oid = tmp.v.obj;						\
	if (valid(oid)) {						\
	    if (bit_is_false(bit_array, oid)) {				\
		bit_true(bit_array, oid);				\
		plist->v.list[++(*px)] = new_obj(oid);			\
		o2 = dbpriv_find_object(oid);				\
		db2_##name(o2, plist, px);				\
	    }								\
	}								\
    }									\
									\
    free_var(field);							\
}									\
									\
Var db_##name(Objid oid, bool full)					\
{									\
    if (oid == NOTHING)							\
	return new_list(0);						\
									\
    Object *o = dbpriv_find_object(oid);				\
									\
    if ((o->field.type == TYPE_OBJ && o->field.v.obj == NOTHING) ||	\
	(o->field.type == TYPE_LIST && listlength(o->field) == 0))	\
	return full ? enlist_var(new_obj(oid)) : new_list(0);		\
									\
    int32 n = db1_##name(o) + (full ? 1 : 0);				\
									\
    CLEAR_BIT_ARRAY();							\
									\
    Var list = new_list(n);						\
    int i = 0;								\
									\
    if (full) {								\
	list.v.list[++i] = new_obj(oid);				\
	bit_true(bit_array, oid);					\
    }									\
									\
    db2_##name(o, &list, &i);						\
									\
    list.v.list[0].v.num = i; /* sketchy */				\
									\
    return list;							\
}

DEFUNC(ancestors, parents);
DEFUNC(descendants, children);

/* the following two could be replace by better/more specific implementations */
DEFUNC(all_locations, location);
DEFUNC(all_contents, contents);

#undef DEFUNC


/*********** Object attributes ***********/

Objid
db_object_owner2(Var obj)
{
    return (TYPE_ANON == obj.type) ?
           dbpriv_object_owner(obj.v.anon) :
           db_object_owner(obj.v.obj);
}

Var
db_object_parents2(Var obj)
{
    return (TYPE_ANON == obj.type) ?
           dbpriv_object_parents(obj.v.anon) :
           db_object_parents(obj.v.obj);
}

Objid
dbpriv_object_owner(Object *o)
{
    return o->owner;
}

void
dbpriv_set_object_owner(Object *o, Objid owner)
{
    o->owner = owner;
}

Objid
db_object_owner(Objid oid)
{
    return dbpriv_object_owner(objects[oid]);
}

void
db_set_object_owner(Objid oid, Objid owner)
{
    dbpriv_set_object_owner(objects[oid], owner);
}

const char *
dbpriv_object_name(Object *o)
{
    return o->name;
}

void
dbpriv_set_object_name(Object *o, const char *name)
{
    if (o->name)
	free_str(o->name);
    o->name = name;
}

const char *
db_object_name(Objid oid)
{
    return dbpriv_object_name(objects[oid]);
}

void
db_set_object_name(Objid oid, const char *name)
{
    dbpriv_set_object_name(objects[oid], name);
}

Var
dbpriv_object_parents(Object *o)
{
    return o->parents;
}

Var
dbpriv_object_children(Object *o)
{
    return o->children;
}

Var
db_object_parents(Objid oid)
{
    return dbpriv_object_parents(objects[oid]);
}

Var
db_object_children(Objid oid)
{
    return dbpriv_object_children(objects[oid]);
}

int
db_count_children(Objid oid)
{
    return listlength(objects[oid]->children);
}

int
db_for_all_children(Objid oid, int (*func) (void *, Objid), void *data)
{
    int i, c = db_count_children(oid);

    for (i = 1; i <= c; i++)
	if (func(data, objects[oid]->children.v.list[i].v.obj))
	    return 1;

    return 0;
}

static int
check_for_duplicate_parents(Var parents)
{
    if (TYPE_LIST != parents.type && TYPE_OBJ != parents.type)
	return 0;

    if (TYPE_LIST == parents.type && listlength(parents) > 1) {
	int i, j, c = listlength(parents);
	for (i = 1; i <= c; i++)
	    for (j = i + i; j <= c; j++)
		if (equality(parents.v.list[i], parents.v.list[j], 1))
		    return 0;
    }

    return 1;
}

int
db_change_parent(Objid oid, Var new_parents)
{
    if (!check_for_duplicate_parents(new_parents))
	return 0;

    if (!dbpriv_check_properties_for_chparent(oid, new_parents))
	return 0;

    if (listlength(objects[oid]->children) == 0 && objects[oid]->verbdefs == NULL) {
	/* Since this object has no children and no verbs, we know that it
	   can't have had any part in affecting verb lookup, since we use first
	   parent with verbs as a key in the verb lookup cache. */
	/* The "no kids" rule is necessary because potentially one of the
	   children could have verbs on it--and that child could have cache
	   entries for THIS object's parentage. */
	/* In any case, don't clear the cache. */
	;
    } else {
	db_priv_affected_callable_verb_lookup();
    }

    Var me = new_obj(oid);
    Var old_parents = objects[oid]->parents;

    /* save this; we need it later */
    Var old_ancestors = db_ancestors(oid, true);

    Var parent;
    int i, c;

    /* Remove me from my old parents' children. */
    if (old_parents.type == TYPE_OBJ && old_parents.v.obj != NOTHING)
	objects[old_parents.v.obj]->children = setremove(objects[old_parents.v.obj]->children, me);
    else if (old_parents.type == TYPE_LIST)
	FOR_EACH(parent, old_parents, i, c)
	    objects[parent.v.obj]->children = setremove(objects[parent.v.obj]->children, me);

    /* Add me to my new parents' children. */
    if (new_parents.type == TYPE_OBJ && new_parents.v.obj != NOTHING)
	objects[new_parents.v.obj]->children = setadd(objects[new_parents.v.obj]->children, me);
    else if (new_parents.type == TYPE_LIST)
	FOR_EACH(parent, new_parents, i, c)
	    objects[parent.v.obj]->children = setadd(objects[parent.v.obj]->children, me);

    free_var(objects[oid]->parents);

    objects[oid]->parents = var_dup(new_parents);
    Var new_ancestors = db_ancestors(oid, true);

    dbpriv_fix_properties_after_chparent(oid, old_ancestors, new_ancestors);

    free_var(old_ancestors);
    free_var(new_ancestors);

    return 1;
}

Var
dbpriv_object_location(Object *o)
{
    return o->location;
}

Objid
db_object_location(Objid oid)
{
    return dbpriv_object_location(objects[oid]).v.obj;
}

Var
dbpriv_object_contents(Object *o)
{
    return o->contents;
}

int
db_count_contents(Objid oid)
{
    return listlength(objects[oid]->contents);
}

int
db_for_all_contents(Objid oid, int (*func) (void *, Objid), void *data)
{
    int i, c = db_count_contents(oid);

    for (i = 1; i <= c; i++)
	if (func(data, objects[oid]->contents.v.list[i].v.obj))
	    return 1;

    return 0;
}

void
db_change_location(Objid oid, Objid new_location)
{
    Var me = new_obj(oid);

    Objid old_location = objects[oid]->location.v.obj;

    if (valid(old_location))
	objects[old_location]->contents = setremove(objects[old_location]->contents, var_dup(me));

    if (valid(new_location))
	objects[new_location]->contents = setadd(objects[new_location]->contents, me);

    free_var(objects[oid]->location);

    objects[oid]->location = new_obj(new_location);
}

int
dbpriv_object_has_flag(Object *o, db_object_flag f)
{
    return (o->flags & (1 << f)) != 0;
}

void
dbpriv_set_object_flag(Object *o, db_object_flag f)
{
    o->flags |= (1 << f);
}

void
dbpriv_clear_object_flag(Object *o, db_object_flag f)
{
    o->flags &= ~(1 << f);
}

int
db_object_has_flag(Objid oid, db_object_flag f)
{
    return dbpriv_object_has_flag(objects[oid], f);
}

void
db_set_object_flag(Objid oid, db_object_flag f)
{
    dbpriv_set_object_flag(objects[oid], f);

    if (f == FLAG_USER)
	all_users = setadd(all_users, new_obj(oid));
}

void
db_clear_object_flag(Objid oid, db_object_flag f)
{
    dbpriv_clear_object_flag(objects[oid], f);
    if (f == FLAG_USER)
	all_users = setremove(all_users, new_obj(oid));
}

int
db_object_allows(Objid oid, Objid progr, db_object_flag f)
{
    return (progr == db_object_owner(oid)
	    || is_wizard(progr)
	    || db_object_has_flag(oid, f));
}

int
is_wizard(Objid oid)
{
    return valid(oid) && db_object_has_flag(oid, FLAG_WIZARD);
}

int
is_programmer(Objid oid)
{
    return valid(oid) && db_object_has_flag(oid, FLAG_PROGRAMMER);
}

int
is_user(Objid oid)
{
    return valid(oid) && db_object_has_flag(oid, FLAG_USER);
}

Var
db_all_users(void)
{
    return all_users;
}

void
dbpriv_set_all_users(Var v)
{
    all_users = v;
}

char rcsid_db_objects[] = "$Id: db_objects.c,v 1.5 2006/09/07 00:55:02 bjj Exp $";

/*
 * $Log: db_objects.c,v $
 * Revision 1.5  2006/09/07 00:55:02  bjj
 * Add new MEMO_STRLEN option which uses the refcounting mechanism to
 * store strlen with strings.  This is basically free, since most string
 * allocations are rounded up by malloc anyway.  This saves lots of cycles
 * computing strlen.  (The change is originally from jitmoo, where I wanted
 * inline range checks for string ops).
 *
 * Revision 1.4  1998/12/14 13:17:36  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:53  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.2  1997/07/07 01:40:20  nop
 * Because we use first-parent-with-verbs as a verb cache key, we can skip
 * a generation bump if the target of a chparent has no kids and no verbs.
 *
 * Revision 1.2.2.1  1997/03/20 07:26:01  nop
 * First pass at the new verb cache.  Some ugly code inside.
 *
 * Revision 1.2  1997/03/03 04:18:29  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:44:59  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.5  1996/04/08  00:42:11  pavel
 * Adjusted computation in `db_object_bytes()' to account for change in the
 * definition of `value_bytes()'.  Release 1.8.0p3.
 *
 * Revision 2.4  1996/02/08  07:18:13  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.3  1996/01/16  07:23:45  pavel
 * Fixed object-array overrun when a recycled object is right on the boundary.
 * Release 1.8.0alpha6.
 *
 * Revision 2.2  1996/01/11  07:30:53  pavel
 * Fixed memory-smash bug in db_renumber_object().  Release 1.8.0alpha5.
 *
 * Revision 2.1  1995/12/11  08:08:28  pavel
 * Added `db_object_bytes()'.  Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:20:51  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  04:20:41  pavel
 * Initial revision
 */
