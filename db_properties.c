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
 * Routines for manipulating properties on DB objects
 *****************************************************************************/

#include "collection.h"
#include "config.h"
#include "db.h"
#include "db_private.h"
#include "list.h"
#include "storage.h"
#include "utils.h"

Propdef
dbpriv_new_propdef(const char *name)
{
    Propdef newprop;

    newprop.name = str_ref(name);
    newprop.hash = str_hash(name);
    return newprop;
}

int
dbpriv_count_properties(Objid oid)
{
    Var ancestor, ancestors;
    int i, c, nprops = 0;
    Object *o;

    if (NOTHING == oid)
	return 0;

    ancestors = db_ancestors(oid, true);

    FOR_EACH(ancestor, ancestors, i, c) {
	o = dbpriv_find_object(ancestor.v.obj);
	nprops += o->propdefs.cur_length;
    }

    free_var(ancestors);

    return nprops;
}

/*
 * Finds the offset of the properties defined on `target' in `this'.
 * Returns -1 if `target' is not an ancestor of `this'.
 */
static int
properties_offset(Objid target, Objid this)
{
    Var ancestor, ancestors;
    int i, c, offset = 0;
    Object *o;

    ancestors = db_ancestors(this, true);

    FOR_EACH(ancestor, ancestors, i, c) {
	if (target == ancestor.v.obj)
	    break;
	o = dbpriv_find_object(ancestor.v.obj);
	offset += o->propdefs.cur_length;
    }

    free_var(ancestors);

    return target == ancestor.v.obj ? offset : -1;
}

/*
 * Returns true iff `oid' defines a property named `pname'.
 */
static int
property_defined_at(const char *pname, int phash, Objid oid)
{
    Proplist *props = &dbpriv_find_object(oid)->propdefs;
    int length = props->cur_length;
    int i;

    for (i = 0; i < length; i++)
	if (props->l[i].hash == phash
	    && !mystrcasecmp(props->l[i].name, pname))
	    return 1;

    return 0;
}

/*
 * Return true iff some descendant of `oid' defines a property named
 * `pname'.
 */
static int
property_defined_at_or_below(const char *pname, int phash, Objid oid)
{
    Proplist *props = &dbpriv_find_object(oid)->propdefs;
    int length = props->cur_length;
    int i;

    for (i = 0; i < length; i++)
	if (props->l[i].hash == phash
	    && !mystrcasecmp(props->l[i].name, pname))
	    return 1;

    Var children = dbpriv_find_object(oid)->children;
    for (i = 1; i <= children.v.list[0].v.num; i++)
	if (property_defined_at_or_below(pname, phash, children.v.list[i].v.obj))
	    return 1;

    return 0;
}

static void
insert_prop(Objid oid, int pos, Pval pval)
{
    Pval *new_propval;
    Object *o;
    int i, nprops;

    nprops = dbpriv_count_properties(oid);
    new_propval = (Pval *) mymalloc(nprops * sizeof(Pval), M_PVAL);

    o = dbpriv_find_object(oid);

    for (i = 0; i < pos; i++)
	new_propval[i] = o->propval[i];

    new_propval[pos] = pval;
    new_propval[pos].var = var_ref(pval.var);
    if (new_propval[pos].perms & PF_CHOWN)
	new_propval[pos].owner = o->owner;

    for (i = pos + 1; i < nprops; i++)
	new_propval[i] = o->propval[i - 1];

    if (o->propval)
	myfree(o->propval, M_PVAL);
    o->propval = new_propval;
}

static void
insert_prop_recursively(Objid root, int prop_pos, Pval pv)
{
    insert_prop(root, prop_pos, pv);

    pv.var.type = TYPE_CLEAR;	/* do after initial insert_prop so only
				   children will be TYPE_CLEAR */

    Var descendant, descendants = db_descendants(root, false);
    int i, c, offset = 0;
    int offsets[listlength(descendants)];

    FOR_EACH(descendant, descendants, i, c) {
	offset = properties_offset(root, descendant.v.obj);
	offsets[i - 1] = offset;
    }

    FOR_EACH(descendant, descendants, i, c) {
	offset = offsets[i - 1];
	insert_prop(descendant.v.obj, offset + prop_pos, pv);
    }

    free_var(descendants);
}

int
db_add_propdef(Objid oid, const char *pname, Var value, Objid owner,
	       unsigned flags)
{
    Object *o;
    Pval pval;
    int i;
    db_prop_handle h;

    h = db_find_property(oid, pname, 0);

    if (h.ptr || property_defined_at_or_below(pname, str_hash(pname), oid))
	return 0;

    o = dbpriv_find_object(oid);
    if (o->propdefs.cur_length == o->propdefs.max_length) {
	Propdef *old_props = o->propdefs.l;
	int new_size = (o->propdefs.max_length == 0
			? 8 : 2 * o->propdefs.max_length);

	o->propdefs.l = (Propdef *) mymalloc(new_size * sizeof(Propdef), M_PROPDEF);
	for (i = 0; i < o->propdefs.max_length; i++)
	    o->propdefs.l[i] = old_props[i];
	o->propdefs.max_length = new_size;

	if (old_props)
	    myfree(old_props, M_PROPDEF);
    }
    o->propdefs.l[o->propdefs.cur_length++] = dbpriv_new_propdef(pname);

    pval.var = value;
    pval.owner = owner;
    pval.perms = flags;

    insert_prop_recursively(oid, o->propdefs.cur_length - 1, pval);

    return 1;
}

int
db_rename_propdef(Objid oid, const char *old, const char *_new)
{
    Proplist *props = &dbpriv_find_object(oid)->propdefs;
    int hash = str_hash(old);
    int count = props->cur_length;
    int i;
    db_prop_handle h;

    for (i = 0; i < count; i++) {
	Propdef p;

	p = props->l[i];
	if (p.hash == hash && !mystrcasecmp(p.name, old)) {
	    if (mystrcasecmp(old, _new) != 0) {	/* Not changing just the case */
		h = db_find_property(oid, _new, 0);
		if (h.ptr
		|| property_defined_at_or_below(_new, str_hash(_new), oid))
		    return 0;
	    }
	    free_str(props->l[i].name);
	    props->l[i].name = str_ref(_new);
	    props->l[i].hash = str_hash(_new);

	    return 1;
	}
    }

    return 0;
}

static void
remove_prop(Objid oid, int pos)
{
    Pval *new_propval;
    Object *o;
    int i, nprops;

    o = dbpriv_find_object(oid);
    nprops = dbpriv_count_properties(oid);

    free_var(o->propval[pos].var);	/* free deleted property */

    if (nprops) {
	new_propval = (Pval *) mymalloc(nprops * sizeof(Pval), M_PVAL);
	for (i = 0; i < pos; i++)
	    new_propval[i] = o->propval[i];
	for (i = pos; i < nprops; i++)
	    new_propval[i] = o->propval[i + 1];
    } else
	new_propval = 0;

    if (o->propval)
	myfree(o->propval, M_PVAL);
    o->propval = new_propval;
}

static void
remove_prop_recursively(Objid root, int prop_pos)
{
    remove_prop(root, prop_pos);

    Var descendant, descendants = db_descendants(root, false);
    int i, c, offset = 0;
    int offsets[listlength(descendants)];

    FOR_EACH(descendant, descendants, i, c) {
	offset = properties_offset(root, descendant.v.obj);
	offsets[i - 1] = offset;
    }

    FOR_EACH(descendant, descendants, i, c) {
	offset = offsets[i - 1];
	remove_prop(descendant.v.obj, offset + prop_pos);
    }

    free_var(descendants);
}

int
db_delete_propdef(Objid oid, const char *pname)
{
    Proplist *props = &dbpriv_find_object(oid)->propdefs;
    int hash = str_hash(pname);
    int count = props->cur_length;
    int max = props->max_length;
    int i, j;

    for (i = 0; i < count; i++) {
	Propdef p;

	p = props->l[i];
	if (p.hash == hash && !mystrcasecmp(p.name, pname)) {
	    if (p.name)
		free_str(p.name);

	    if (max > 8 && props->cur_length <= ((max * 3) / 8)) {
		int new_size = max / 2;
		Propdef *new_props;

		new_props = (Propdef *) mymalloc(new_size * sizeof(Propdef), M_PROPDEF);

		for (j = 0; j < i; j++)
		    new_props[j] = props->l[j];
		for (j = i + 1; j < count; j++)
		    new_props[j - 1] = props->l[j];

		myfree(props->l, M_PROPDEF);
		props->l = new_props;
		props->max_length = new_size;
	    } else
		for (j = i + 1; j < count; j++)
		    props->l[j - 1] = props->l[j];

	    props->cur_length--;
	    remove_prop_recursively(oid, i);

	    return 1;
	}
    }

    return 0;
}

int
db_count_propdefs(Objid oid)
{
    return dbpriv_find_object(oid)->propdefs.cur_length;
}

int
db_for_all_propdefs(Objid oid, int (*func) (void *, const char *), void *data)
{
    int i;
    Object *o = dbpriv_find_object(oid);
    int len = o->propdefs.cur_length;

    for (i = 0; i < len; i++)
	if (func(data, o->propdefs.l[i].name))
	    return 1;

    return 0;
}

struct contents_data {
    Var r;
    int i;
};

static int
add_to_list(void *data, Objid c)
{
    struct contents_data *d = (contents_data *)data;

    d->i++;
    d->r.v.list[d->i].type = TYPE_OBJ;
    d->r.v.list[d->i].v.obj = c;

    return 0;
}

static void
get_bi_value(db_prop_handle h, Var * value)
{
    Objid oid = *((Objid *) h.ptr);

    switch (h.built_in) {
    case BP_NAME:
        value->type = (var_type)TYPE_STR;
	value->v.str = str_ref(db_object_name(oid));
	break;
    case BP_OWNER:
	value->type = TYPE_OBJ;
	value->v.obj = db_object_owner(oid);
	break;
    case BP_PROGRAMMER:
	value->type = TYPE_INT;
	value->v.num = db_object_has_flag(oid, FLAG_PROGRAMMER);
	break;
    case BP_WIZARD:
	value->type = TYPE_INT;
	value->v.num = db_object_has_flag(oid, FLAG_WIZARD);
	break;
    case BP_R:
	value->type = TYPE_INT;
	value->v.num = db_object_has_flag(oid, FLAG_READ);
	break;
    case BP_W:
	value->type = TYPE_INT;
	value->v.num = db_object_has_flag(oid, FLAG_WRITE);
	break;
    case BP_F:
	value->type = TYPE_INT;
	value->v.num = db_object_has_flag(oid, FLAG_FERTILE);
	break;
    case BP_LOCATION:
	value->type = TYPE_OBJ;
	value->v.obj = db_object_location(oid);
	break;
    case BP_CONTENTS:
	{
	    struct contents_data d;

	    d.r = new_list(db_count_contents(oid));
	    d.i = 0;
	    db_for_all_contents(oid, add_to_list, &d);

	    *value = d.r;
	}
	break;
    default:
	panic("Unknown built-in property in GET_BI_VALUE!");
    }
}

db_prop_handle
db_find_property(Objid oid, const char *name, Var * value)
{
    static struct {
	const char *name;
	enum bi_prop prop;
	int hash;
    } ptable[] = {
#define _ENTRY(P,p) { #p, BP_##P, 0 },
      BUILTIN_PROPERTIES(_ENTRY)
#undef _ENTRY
    };
    static int ptable_init = 0;
    int i, n;
    db_prop_handle h;
    int hash = str_hash(name);
    Object *o;

    if (!ptable_init) {
	for (i = 0; i < Arraysize(ptable); i++)
	    ptable[i].hash = str_hash(ptable[i].name);
	ptable_init = 1;
    }
    h.definer = NOTHING;
    for (i = 0; i < Arraysize(ptable); i++) {
	if (ptable[i].hash == hash && !mystrcasecmp(name, ptable[i].name)) {
	    static Objid ret;

	    ret = oid;
	    h.built_in = ptable[i].prop;
	    h.ptr = &ret;
	    if (value)
		get_bi_value(h, value);
	    return h;
	}
    }

    h.built_in = BP_NONE;
    n = 0;

    Var ancestor, ancestors;
    int i1, c1;

    ancestors = db_ancestors(oid, true);

    FOR_EACH(ancestor, ancestors, i1, c1) {
	o = dbpriv_find_object(ancestor.v.obj);

	Proplist *props = &(o->propdefs);
	Propdef *defs = props->l;
	int length = props->cur_length;

	for (i = 0; i < length; i++, n++) {
	    if (defs[i].hash == hash
		&& !mystrcasecmp(defs[i].name, name)) {
		Pval *prop;

		h.definer = o->id;
		o = dbpriv_find_object(oid);
		h.ptr = (Pval *)o->propval + n;

		if (value) {
                  prop = (Pval *)h.ptr;

		    while (prop->var.type == TYPE_CLEAR) {
			if (TYPE_LIST == o->parents.type) {
			    Var parent, parents = o->parents;
			    int i2, c2, offset = 0;
			    FOR_EACH(parent, parents, i2, c2)
				if ((offset = properties_offset(h.definer, parent.v.obj)) > -1)
				    break;
			    o = dbpriv_find_object(parent.v.obj);
			    prop = o->propval + offset + i;
			}
			else if (TYPE_OBJ == o->parents.type && NOTHING != o->parents.v.obj) {
			    int offset = properties_offset(h.definer, o->parents.v.obj);
			    o = dbpriv_find_object(o->parents.v.obj);
			    prop = o->propval + offset + i;
			}
		    }
		    *value = prop->var;
		}

		free_var(ancestors);

		return h;
	    }
	}
    }

    free_var(ancestors);

    h.ptr = 0;
    return h;
}

Var
db_property_value(db_prop_handle h)
{
    Var value;

    if (h.built_in)
	get_bi_value(h, &value);
    else {
        Pval *prop = (Pval *)h.ptr;

	value = prop->var;
    }

    return value;
}

void
db_set_property_value(db_prop_handle h, Var value)
{
    if (!h.built_in) {
        Pval *prop = (Pval *)h.ptr;

	free_var(prop->var);
	prop->var = value;
    } else {
	Objid oid = *((Objid *) h.ptr);
	db_object_flag flag;

	switch (h.built_in) {
	case BP_NAME:
	    if (value.type != TYPE_STR)
		goto complain;
	    db_set_object_name(oid, value.v.str);
	    break;
	case BP_OWNER:
	    if (value.type != TYPE_OBJ)
		goto complain;
	    db_set_object_owner(oid, value.v.obj);
	    break;
	case BP_PROGRAMMER:
	    flag = FLAG_PROGRAMMER;
	    goto finish_flag;
	case BP_WIZARD:
	    flag = FLAG_WIZARD;
	    goto finish_flag;
	case BP_R:
	    flag = FLAG_READ;
	    goto finish_flag;
	case BP_W:
	    flag = FLAG_WRITE;
	    goto finish_flag;
	case BP_F:
	    flag = FLAG_FERTILE;
	  finish_flag:
	    if (is_true(value))
		db_set_object_flag(oid, flag);
	    else
		db_clear_object_flag(oid, flag);
	    free_var(value);
	    break;
	case BP_LOCATION:
	case BP_CONTENTS:
	  complain:
	    panic("Inappropriate value in DB_SET_PROPERTY_VALUE!");
	    break;
	default:
	    panic("Unknown built-in property in DB_SET_PROPERTY_VALUE!");
	}
    }
}

Objid
db_property_owner(db_prop_handle h)
{
    if (h.built_in) {
	panic("Built-in property in DB_PROPERTY_OWNER!");
	return NOTHING;
    } else {
        Pval *prop = (Pval *)h.ptr;

	return prop->owner;
    }
}

void
db_set_property_owner(db_prop_handle h, Objid oid)
{
    if (h.built_in)
	panic("Built-in property in DB_SET_PROPERTY_OWNER!");
    else {
        Pval *prop = (Pval *)h.ptr;

	prop->owner = oid;
    }
}

unsigned
db_property_flags(db_prop_handle h)
{
    if (h.built_in) {
	panic("Built-in property in DB_PROPERTY_FLAGS!");
	return 0;
    } else {
        Pval *prop = (Pval *)h.ptr;

	return prop->perms;
    }
}

void
db_set_property_flags(db_prop_handle h, unsigned flags)
{
    if (h.built_in)
	panic("Built-in property in DB_SET_PROPERTY_FLAGS!");
    else {
        Pval *prop = (Pval *)h.ptr;

	prop->perms = flags;
    }
}

int
db_property_allows(db_prop_handle h, Objid progr, db_prop_flag flag)
{
    return ((db_property_flags(h) & flag)
	    || progr == db_property_owner(h)
	    || is_wizard(progr));
}

/*
 * `parents' is the proposed set of new parents.  Ensure that object
 * `oid' or one of its descendants does not define a property with the
 * same name as one defined on `parents' or any of their ancestors.
 * Ensure that none of the `parents' not their ancestors define a
 * property with the same name.
 */
int
dbpriv_check_properties_for_chparent(Objid oid, Var parents)
{
    /* build a list of ancestors from the supplied parents */

    Var ancestors = new_list(0);
    Var stack = enlist_var(var_dup(parents));

    while (listlength(stack) > 0) {
	Var top;
	POP_TOP(top, stack);
	if (valid(top.v.obj)) {
	    if (top.v.obj != oid) {
		Var tmp = dbpriv_find_object(top.v.obj)->parents;
		tmp = enlist_var(var_ref(tmp));
		stack = listconcat(tmp, stack);
	    }
	    ancestors = setadd(ancestors, top);
	}
    }

    free_var(stack);

    Object *o;
    Var ancestor;
    int i, c, x;

    /* check props in descendants */

    FOR_EACH(ancestor, ancestors, i, c) {
	o = dbpriv_find_object(ancestor.v.obj);
	Proplist *props = &o->propdefs;

	for (x = 0; x < props->cur_length; x++)
	    if (property_defined_at_or_below(props->l[x].name,
					     props->l[x].hash,
					     oid)) {
		free_var(ancestors);
		return 0;
	    }
    }

    /* check props in parents */

    while (listlength(ancestors) > 0) {
	POP_TOP(ancestor, ancestors);

	o = dbpriv_find_object(ancestor.v.obj);
	Proplist *props = &o->propdefs;

	Var obj;

	FOR_EACH(obj, ancestors, i, c) {
	    if (obj.v.obj == ancestor.v.obj)
		continue;
	    for (x = 0; x < props->cur_length; x++)
		if (property_defined_at(props->l[x].name,
					props->l[x].hash,
					obj.v.obj)) {
		    free_var(ancestors);
		    return 0;
		}
	    
	}
    }

    free_var(ancestors);
    return 1;
}

/*
 * Given lists of `old_ancestors' and `new_ancestors', fix the
 * properties of `oid' by 1) preserving properties whose definition is
 * present in both `old_ancestors' and `new_ancestors', 2) removing
 * all properties whose definition is not present in `new_ancestors',
 * and 3) adding new clear properties for properties whose definition
 * was added in `new_ancestors'.
 *
 * Consider the following graph.  The challenge is to figure out what
 * properties we must preserve when we chparent `a' to `c' (bypassing
 * `b' which is still a parent of `f').
 *
 *      x - m
 *     /     \
 *    z       a ---- b - c
 *     \     /       /
 *      y - n       /
 *           \     /
 *            e - f
 *
 * Ancestors Before
 *  m - a, b, c
 *  x - m, a, b, c
 *  n - a, b, c, e, f
 *  y - n, a, b, c, e, f
 *  z - x, m, a, b, c, y, n, e, f
 *
 * Ancestors After
 *  m - a, c
 *  x - m, a, c
 *  n - a, c, e, f, b
 *  y - n, a, c, e, f, b
 *  z - x, m, a, c, y, n, e, f, b
 *
 * Note that for `z', `b' is still an ancestor, however the path
 * changed from _through `x'_ to _through `y'_.  If `z' changed the
 * value of a property on `b', then that value should be preserved.
 * If the value of a property on `b' was clear, then access through
 * `x' will pick up the non-clear value from an ancestor based on the
 * new in inheritance path.  This applies to `y' and `n', as well.
 *
 * In short, in the case of `z', `y' and `n', the properties of `b'
 * should be preserved, even though `b' moved in the inheritance path.
 * The only thing that should change is the apparent value from clear
 * properties.
 *
 * Note!  Ownership of 'c' properties is a sticky issue -- however
 * ickyness exists in the single-inheritance implementation, too --
 * once ownership is established in a child via the `c' flag, it is
 * never revoked, even if the parent changes or the parent changes the
 * `c' flag!
 *
 * Implementation: for this object and all of its descendants,
 * calculate the set of old ancestors, the set of new ancestors, and
 * find their intersection.  The layout may change, but as we're
 * laying out the properties in memory, consult the intersection, and
 * preserve information about those properties.
 */
void
dbpriv_fix_properties_after_chparent(Objid oid, Var old_ancestors, Var new_ancestors)
{
    Object *o;
    Var ancestor;

    /*
     * Property values are laid corresponding to the order of
     * ancestors in the inheritance hierarchy.  The offsets arrays
     * hold the starting point of each sub-array.
     */
    int offset;
    int old_count, *old_offsets = mymalloc((listlength(old_ancestors) + 1) * sizeof(int), M_INT);
    int new_count, *new_offsets = mymalloc((listlength(new_ancestors) + 1) * sizeof(int), M_INT);
    int i1, c1;

    /* C arrays start at index 0, MOO arrays start at index 1 */

    offset = 0;
    FOR_EACH(ancestor, old_ancestors, i1, c1) {
	o = dbpriv_find_object(ancestor.v.obj);
	old_offsets[i1 - 1] = offset;
	offset += o->propdefs.cur_length;
    }
    old_count = old_offsets[i1 - 1] = offset;

    offset = 0;
    FOR_EACH(ancestor, new_ancestors, i1, c1) {
	o = dbpriv_find_object(ancestor.v.obj);
	new_offsets[i1 - 1] = offset;
	offset += o->propdefs.cur_length;
    }
    new_count = new_offsets[i1 - 1] = offset;

    /*
     * Iterate through the new ancestors.  Copy property values that
     * are in both the new ancestors and the old ancestors.
     * Otherwise, add new clear property values and delete any
     * remaining property values.
     */
    Object *me = dbpriv_find_object(oid);
    Pval *new_propval = NULL;

    if (new_count != 0) {
      new_propval = (Pval *) mymalloc(new_count * sizeof(Pval), M_PVAL);
	int i2, c2, i3, c3;
	FOR_EACH(ancestor, new_ancestors, i2, c2) {
	    int n1 = new_offsets[i2 - 1];
	    int n2 = new_offsets[i2];
	    int l, x;
	    if ((l = ismember(ancestor, old_ancestors, 1)) > 0) {
		int o1 = old_offsets[l - 1];
		int o2 = old_offsets[l];
		for (x = o1; x < o2; x++, n1++) {
		    new_propval[n1].var = var_ref(me->propval[x].var);
		    new_propval[n1].owner = me->propval[x].owner;
		    new_propval[n1].perms = me->propval[x].perms;
		}
	    }
	    else {
		Var parent, parents = enlist_var(var_ref(me->parents));
		FOR_EACH(parent, parents, i3, c3)
		    if (valid(parent.v.obj))
			if ((offset = properties_offset(ancestor.v.obj, parent.v.obj)) > -1)
			    break;
		free_var(parents);
		for (x = 0; n1 < n2; x++, n1++) {
		    Pval pv = dbpriv_find_object(parent.v.obj)->propval[offset + x];
		    new_propval[n1].var = clear;
		    new_propval[n1].owner = pv.perms & PF_CHOWN ? me->owner : pv.owner;
		    new_propval[n1].perms = pv.perms;
		}
	    }
	}
    }

    /*
     * Clean up.
     */
    int c;
    if (me->propval) {
	for (c = 0; c < old_count; c++)
	  free_var(me->propval[c].var);
	myfree(me->propval, M_PVAL);
    }
    me->propval = new_propval;

    myfree(old_offsets, M_INT);
    myfree(new_offsets, M_INT);

    /*
     * Recursively call dbpriv_fix_properties_after_chparent for each
     * child.  For each child's parents assemble the old ancestors.
     * When we come to me (oid) as parent, use my old ancestors
     * instead of my new ancestors.
     */
    Var child, parent;
    int i4, c4, i5, c5, i6, c6;
    FOR_EACH(child, dbpriv_find_object(oid)->children, i4, c4) {
	Object *oc = dbpriv_find_object(child.v.obj);
	Var _new = new_list(1);
	Var old = new_list(1);
	_new.v.list[1] = var_ref(child);
	old.v.list[1] = var_ref(child);
	if (TYPE_LIST == oc->parents.type) {
	    FOR_EACH(parent, oc->parents, i5, c5) {
		Object *op = dbpriv_find_object(parent.v.obj);
		if (op->id == oid) {
		    Var tmp;
		    FOR_EACH(tmp, old_ancestors, i6, c6)
			old = setadd(old, var_ref(tmp));
		    FOR_EACH(tmp, new_ancestors, i6, c6)
			_new = setadd(new, var_ref(tmp));
		}
		else {
		    Var tmp, all = db_ancestors(op->id, true);
		    FOR_EACH(tmp, all, i6, c6) {
			old = setadd(old, var_ref(tmp));
			_new = setadd(new, var_ref(tmp));
		    }
		    free_var(all);
		}
	    }
	}
	else {
	    _new = listconcat(new, var_ref(new_ancestors));
	    old = listconcat(old, var_ref(old_ancestors));
	}
	dbpriv_fix_properties_after_chparent(child.v.obj, old, _new);
	free_var(_new);
	free_var(old);
    }
}

char rcsid_db_properties[] = "$Id: db_properties.c,v 1.5 2010/04/23 04:46:18 wrog Exp $";

/* 
 * $Log: db_properties.c,v $
 * Revision 1.5  2010/04/23 04:46:18  wrog
 * whitespace
 *
 * Revision 1.4  2010/03/26 23:46:47  wrog
 * Moved builtin properties into a macro\nFixed compiler warning about unassigned field
 *
 * Revision 1.3  1998/12/14 13:17:38  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:31  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:44:59  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.6  1996/04/08  01:08:32  pavel
 * Fixed `db_rename_propdef()' to allow case-only changes.  Release 1.8.0p3.
 *
 * Revision 2.5  1996/02/11  00:46:48  pavel
 * Enhanced db_find_property() to report the defining object of the found
 * property.  Release 1.8.0beta2.
 *
 * Revision 2.4  1996/02/08  07:18:02  pavel
 * Renamed TYPE_NUM to TYPE_INT.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.3  1995/12/31  03:27:40  pavel
 * Removed a few more uses of `unsigned'.  Reordered things in
 * db_delete_propdef() to fix an occasional memory smash.
 * Release 1.8.0alpha4.
 *
 * Revision 2.2  1995/12/28  00:41:34  pavel
 * Made *all* built-in property references return fresh value references.
 * Release 1.8.0alpha3.
 *
 * Revision 2.1  1995/12/11  07:52:27  pavel
 * Added support for renaming propdefs.
 *
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:21:13  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  04:21:02  pavel
 * Initial revision
 */
