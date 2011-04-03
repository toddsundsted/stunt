/******************************************************************************
  Copyright (c) 2000 Phil Schwan <phil@off.net>.  All rights reserved.

  Use and copying of this software and preparation of derivative works based
  upon this software are permitted.  Any distribution of this software or
  derivative works must comply with all applicable United States export
  control laws.  This software is made available AS IS, and neither the author
  nor Xerox Corporation makes no warranty about the software, its performance
  or its conformity to any specification.  Any person obtaining a copy of this
  software is requested to send their name and post office or electronic mail
  address to:
    Pavel Curtis
    Xerox PARC
    3333 Coyote Hill Rd.
    Palo Alto, CA 94304
    Pavel@Xerox.Com
 *****************************************************************************/

#include "my-ctype.h"
#include "my-string.h"

#include "bf_register.h"
#include "config.h"
#include "exceptions.h"
#include "functions.h"
#include "hash.h"
#include "list.h"
#include "log.h"
#include "options.h"
#include "pattern.h"
#include "random.h"
#include "ref_count.h"
#include "streams.h"
#include "storage.h"
#include "structures.h"
#include "unparse.h"
#include "utils.h"

/* I'm not thrilled with the function names, but I tried to mimic the rest
 * of LambdaMOO's coding style.  How I wish LambdaMOO used glib.
 *
 * Known issues:
 *
 * - hash loops (ie, inserting a hash into itself) are hazardous.  Don't do
 *   it.  You'll crash your server.
 *   - the code still lets you do it, however; I haven't decided if there's
 *     a good way to allow loops, so I haven't written code that prevents it
 * - do_hash isn't portable or very efficient.  portable is easy, efficient
 *   might be harder.
 */

#define HASH_DEF_SIZE 11

static void hashresize(Var v);
static int32 do_hash(Var v);

Var
new_hash(void)
{
    Var v;

    v.type = TYPE_HASH;
    v.v.hash = (Hash *) mymalloc(1 * sizeof(Hash), M_HASH);
    v.v.hash->size = HASH_DEF_SIZE;
    v.v.hash->nnodes = 0;
    v.v.hash->frozen = 0;
    v.v.hash->nodes =
        (HashNode **) mymalloc(v.v.hash->size * sizeof(HashNode *), M_VM);
    memset(v.v.hash->nodes, 0, v.v.hash->size * sizeof(HashNode *));

    return v;
}

static void
destroy_hashnode(HashNode *n)
{
    free_var(n->key);
    free_var(n->value);
    myfree(n, M_VM);
}

void
destroy_hash(Var v)
{
    int n;

    for (n = 0; n < v.v.hash->size; n++) {
        while (v.v.hash->nodes[n]) {
            HashNode *next = v.v.hash->nodes[n]->next;
            destroy_hashnode(v.v.hash->nodes[n]);
            v.v.hash->nodes[n] = next;
        }
    }

    myfree(v.v.hash->nodes, M_VM);
    myfree(v.v.hash, M_HASH);
}

/* Iterate over the entire hash, calling the function once per key/value 
 * pair */
void
hashforeach(Var v, hashfunc func, void *data)
{
    int n;
    int first = 1;
    HashNode *hn;

    for (n = 0; n < v.v.hash->size; n++) {
        for (hn = v.v.hash->nodes[n]; hn; hn = hn->next) {
            (* func)(hn->key, hn->value, data, first);
            first = 0;
        }
    }
}

static HashNode *
new_hashnode(Var key, Var value)
{
    HashNode *n;

    n = (HashNode *) mymalloc(1 * sizeof(HashNode), M_VM);
    n->key = key;
    n->value = value;
    n->next = NULL;
 
    return n;
}

static void
do_hash_hash(Var key, Var value, void *data, int32 first)
{
    int32 *h = (int32 *)data;

    *h *= do_hash(key);
    *h *= do_hash(value);
}

/* FIXME: these are really just for testing; they should be replaced with
 * portable, more reasonable hashes. */
static int32
do_hash(Var v)
{
    int t = v.type;

    switch (t) {
    case TYPE_STR: {
        unsigned32 h = str_hash(v.v.str);

        return h;
        
        break;
    }
    case TYPE_OBJ:
    case TYPE_ERR:
    case TYPE_INT:
        return v.v.num;
        break;
    case TYPE_FLOAT:
        return (int)(*v.v.fnum);
        break;
    case TYPE_HASH:
        {
            int32 value = 1;

            hashforeach(v, do_hash_hash, &value);

            return value;
        }
        break;
    case TYPE_LIST:
        {
            int32 value = 1;
            int n;

            for (n = 0; n <= v.v.list[0].v.num; n++ ) {
                value *= do_hash(v.v.list[n]);
            }
            return value;
        }
        break;
    default:
        errlog("DO_HASH: Unknown type (%d)\n", t);
        return 0;
	break;
    }
}

static int32
do_compare(Var a, Var b)
{
    return (equality(a, b, 0) == 0);
}

static inline HashNode **
do_hashlookup(Var v, Var key)
{
    HashNode **n;

    int32 index = do_hash(key) % v.v.hash->size;
    n = &(v.v.hash->nodes[index]);

    while (*n && do_compare((*n)->key, key))
        n = &(*n)->next;

    return n;
}

int
hashremove(Var v, Var key)
{
    HashNode **n, *d;

    n = do_hashlookup(v, key);

    if (*n) {
        d = *n;

        *n = d->next;
        destroy_hashnode(d);
        v.v.hash->nnodes--;
        if (!v.v.hash->frozen) {
            hashresize(v);
        }
        return 1;
    } else {
        return 0;
    }
}

Var
hashdelete(Var v, Var key)
{
    Var new;
    int n;
    HashNode *hn;

    new = new_hash();
    for (n = 0; n < v.v.hash->size; n++) {
        for (hn = v.v.hash->nodes[n]; hn; hn = hn->next) {
            if (do_compare(hn->key, key))
	        hashinsert(new, hn->key, hn->value);
        }
    }

    free_var(v);
    return new;
}

int
hashlookup(Var v, Var key, Var *value)
{
    HashNode **n = do_hashlookup(v, key);

    if (*n) {
        if (value) {
            *value = (*n)->value;
        }
        return 1;
    } else {
        return 0;
    }
}

/* Based loosely on glib's hash resizing heuristics.  I don't bother finding
 * the nearest prime, though a motivated reader may be interested in doing
 * so. */
static void
hashresize(Var v)
{
    double npl;
    int new_size, n;
    HashNode **new_nodes;

    npl = (double) v.v.hash->nnodes / (double) v.v.hash->size;

    if ((npl > 0.3 || v.v.hash->size <= HASH_DEF_SIZE) && npl < 3.0)
        return;

    if (npl > 1.0) {
        new_size = v.v.hash->size * 2;
    } else {
        new_size = v.v.hash->size / 2;
    }

    new_nodes = (HashNode **) mymalloc(new_size * sizeof(HashNode *), M_VM);
    memset(new_nodes, 0, new_size * sizeof(HashNode *));

    for (n = 0; n < v.v.hash->size; n++) {
        HashNode *node, *next;

        for (node = v.v.hash->nodes[n]; node; node = next) {
            int32 index = do_hash(node->key) % new_size;

            next = node->next;
            node->next = new_nodes[index];
            new_nodes[index] = node;
        }
    }

    myfree(v.v.hash->nodes, M_VM);
    v.v.hash->nodes = new_nodes;
    v.v.hash->size = new_size;
}

void
hashinsert(Var hash, Var key, Var value)
/* consumes `key' and `value', does not consume `hash' */
{
    HashNode **n = do_hashlookup(hash, key);

    if (*n) {
        free_var((*n)->value);
        (*n)->value = value;
        free_var(key);
    } else {
        *n = new_hashnode(key, value);
        hash.v.hash->nnodes++;
        if (!hash.v.hash->frozen) {
            hashresize(hash);
        }
    }
}

unsigned32
hashsize(Var v)
{
    return v.v.hash->size;
}

unsigned32
hashnodes(Var v)
{
    return v.v.hash->nnodes;
}

int32
hash_equal(Var *a, Var *b)
{
    int n;
    HashNode *hn;

    /* If they have different numbers of nodes, they can't be identical. */
    if (a->v.hash->nnodes != b->v.hash->nnodes)
        return 0;

    /* otherwise, for each node in a, do a lookup in b */
    for (n = 0; n < a->v.hash->size; n++) {
        for (hn = a->v.hash->nodes[n]; hn; hn = hn->next) {
            Var value;

            if (!hashlookup(*b, hn->key, &value)) {
                return 0;
            }
            if (!equality(hn->value, value, 0)) {
                return 0;
            }
        }
    }

    return 1;
}

#ifdef DEBUG
void
dumphash(Var v)
{
    int n;
    HashNode *hn;

    for (n = 0; n < v.v.hash->size; n++) {
        for (hn = v.v.hash->nodes[n]; hn; hn = hn->next) {
            errlog("key type: %d  value type: %d\n",
                   hn->key.type, hn->value.type);
        }
    }
}
#endif

static package
bf_hash_remove(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r = arglist.v.list[1];
    Var key = arglist.v.list[2];

    Var retval;
    retval.type = TYPE_INT;
    retval.v.num = hashremove(r, key);
    free_var(arglist);

    return make_var_pack(retval);
}

static package
bf_hashdelete(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    if (!hashlookup(arglist.v.list[1], arglist.v.list[2], &r)) {
	free_var(arglist);
	return make_error_pack(E_RANGE);
    }
    else {
	r = hashdelete(var_ref(arglist.v.list[1]), arglist.v.list[2]);
    }

    free_var(arglist);
    return make_var_pack(r);
}

static void
do_hash_keys(Var key, Var value, void *data, int32 first)
{
    Var *list = (Var *)data;
    *list = listappend(*list, var_ref(key));
}

static package
bf_hashkeys(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r = new_list(0);
    hashforeach(arglist.v.list[1], do_hash_keys, &r);
    free_var(arglist);
    return make_var_pack(r);
}

static void
do_hash_values(Var key, Var value, void *data, int32 first)
{
    Var *list = (Var *)data;
    *list = listappend(*list, var_ref(value));
}

static package
bf_hashvalues(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r = new_list(0);
    hashforeach(arglist.v.list[1], do_hash_values, &r);
    free_var(arglist);
    return make_var_pack(r);
}

void
register_hash(void)
{
    register_function("mapdelete", 2, 2, bf_hashdelete, TYPE_HASH, TYPE_ANY);
    register_function("mapkeys", 1, 1, bf_hashkeys, TYPE_HASH);
    register_function("mapvalues", 1, 1, bf_hashvalues, TYPE_HASH);
}
