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

extern Map new_map(void);
extern void destroy_map(Map& map);
extern Map map_dup(const Map& map);

extern Map mapinsert(const Map& map, const Var& key, const Var& value);
extern const rbnode* maplookup(const Map& map, const Var& key, Var* value, int case_matters);
extern int mapseek(const Map& map, const Var& key, Iter* iter, int case_matters);
extern int mapequal(const Map& lhs, const Map& rhs, int case_matters);
extern int32_t maplength(const Map& map);
extern int mapempty(const Map& map);

extern int map_sizeof(rbtree *tree);

extern int mapfirst(const Map& map, var_pair *pair);
extern int maplast(const Map& map, var_pair *pair);

extern Iter new_iter(const Map& map);
extern void destroy_iter(Iter iter);

extern int iterget(const Iter& iter, var_pair *pair);
extern void iternext(Iter& iter);

extern Map maprange(const Map& map, rbtrav *from, rbtrav *to);
extern Map maprangeset(const Map& map, rbtrav *from, rbtrav *to, const Map& values);

typedef int (*mapfunc) (const Var& key, const Var& value, void *data, int first);
extern int mapforeach(const Map& map, mapfunc func, void *data);

/* You're never going to need to use this!
 * Clears a node in place by setting the associated value type to
 * `E_NONE'.  This _destructively_ updates the associated tree.  The
 * method is used in `execute.c' to clear a node's value in a map when
 * the vm knows that it will eventually replace that value.  This
 * removes a `var_ref' and eventual `map_dup' when the vm can
 * guarantee that a nested map is not shared.
 */
extern void clear_node_value(const rbnode *node);
