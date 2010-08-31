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

#include "structures.h"

typedef struct HashNode HashNode;

struct HashNode {
    Var key;
    Var value;
    struct HashNode *next;
};

typedef void (*hashfunc) (Var key, Var value, void *data, int32 first);

extern Var new_hash(void);
extern void destroy_hash(Var v);
extern void hashforeach(Var v, hashfunc func, void *data);
extern void hashinsert(Var v, Var key, Var value);
extern int hashremove(Var v, Var key);
extern int hashlookup(Var v, Var key, Var *value);
extern unsigned32 hashsize(Var v);
extern unsigned32 hashnodes(Var v);
extern int32 hash_equal(Var *a, Var *b);
extern void register_hash(void);
