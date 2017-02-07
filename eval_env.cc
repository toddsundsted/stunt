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

#include "config.h"
#include "eval_env.h"
#include "storage.h"
#include "structures.h"
#include "sym_table.h"
#include "utils.h"

/*
 * Keep a pool of rt_envs big enough to hold NUM_READY_VARS variables to
 * avoid lots of malloc/free.
 */
static Var *ready_size_rt_envs;

Var *
new_rt_env(unsigned size)
{
    Var *ret;
    unsigned i;

    if (size <= NUM_READY_VARS && ready_size_rt_envs) {
	ret = ready_size_rt_envs;
	ready_size_rt_envs = ret[0].v.list;
    } else
	ret = (Var *)malloc(MAX(size, NUM_READY_VARS) * sizeof(Var));

    for (i = 0; i < size; i++)
	ret[i] = none;

    return ret;
}

void
free_rt_env(Var * rt_env, unsigned size)
{
    unsigned i;

    for (i = 0; i < size; i++)
	free_var(rt_env[i]);

    if (size <= NUM_READY_VARS) {
	rt_env[0].v.list = ready_size_rt_envs;
	ready_size_rt_envs = rt_env;
    } else
	free(rt_env);
}

Var *
copy_rt_env(Var * from, unsigned size)
{
    unsigned i;

    Var *ret = new_rt_env(size);
    for (i = 0; i < size; i++)
	ret[i] = var_ref(from[i]);
    return ret;
}

void
fill_in_rt_consts(Var * env, DB_Version version)
{
    env[SLOT_ERR] = Var::new_int((int)TYPE_ERR);
    env[SLOT_NUM] = Var::new_int((int)TYPE_INT);
    env[SLOT_STR] = Var::new_int((int)_TYPE_STR);
    env[SLOT_OBJ] = Var::new_int((int)TYPE_OBJ);
    env[SLOT_LIST] = Var::new_int((int)_TYPE_LIST);
    if (version >= DBV_Float) {
	env[SLOT_INT] = Var::new_int((int)TYPE_INT);
	env[SLOT_FLOAT] = Var::new_int((int)_TYPE_FLOAT);
    }
    if (version >= DBV_Map) {
	env[SLOT_MAP] = Var::new_int((int)_TYPE_MAP);
    }
    if (version >= DBV_Anon) {
	env[SLOT_ANON] = Var::new_int((int)_TYPE_ANON);
    }
}

void
set_rt_env_obj(Var * env, int slot, Objid o)
{
    env[slot] = Var::new_obj(o);
}

void
set_rt_env_str(Var * env, int slot, const char* s)
{
    env[slot] = Var::new_str(s);
}

void
set_rt_env_str(Var * env, int slot, const ref_ptr<const char>& s)
{
    env[slot] = Var::new_str(s);
}

void
set_rt_env_var(Var * env, int slot, const Var& v)
{
    env[slot] = var_ref(v);
}
