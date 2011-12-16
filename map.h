/******************************************************************************
  Copyright 2010 Todd Sundsted. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY TODD SUNDSTED ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
  EVENT SHALL TODD SUNDSTED OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  The views and conclusions contained in the software and documentation are
  those of the authors and should not be interpreted as representing official
  policies, either expressed or implied, of Todd Sundsted.
 *****************************************************************************/

#include "structures.h"

extern Var new_map(void);
extern void destroy_map(Var map);
extern Var map_dup(Var map);

extern Var mapinsert(Var map, Var key, Var value);
extern int maplookup(Var map, Var key, Var *value, int case_matters);
extern int mapequal(Var lhs, Var rhs, int case_matters);
typedef int (*mapfunc) (Var key, Var value, void *data, int first);
extern int mapforeach(Var map, mapfunc func, void *data);
extern int32 maplength(Var map);
extern int mapempty(Var map);
extern int map_sizeof(rbtree *tree);

extern rbnode *mapfirst(Var map);
extern rbnode *maplast(Var map);

extern Var nodekey(rbnode *node);
extern Var nodevalue(rbnode *node);

extern Var new_iter(Var map);
extern void destroy_iter(Var iter);
extern Var iter_dup(Var iter);

struct mapitem {
    Var key;
    Var value;
};

extern int iterget(Var iter, struct mapitem *item);
extern void iternext(Var iter);

extern Var map_seek(Var map, Var key);
