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

#include <string>
#include <sstream>
#include <vector>

#include <stdarg.h>

#include "bf_register.h"
#include "config.h"
#include "db_io.h"
#include "functions.h"
#include "list.h"
#include "log.h"
#include "map.h"
#include "server.h"
#include "storage.h"
#include "structures.h"
#include "unparse.h"
#include "utils.h"

/*****************************************************************************
 * This is the table of procedures that register MOO built-in functions.  To
 * add new built-in functions to the server, add to the list below the name of
 * a C function that will register your new MOO built-ins; your C function will
 * be called exactly once, during server initialization.  Also add a
 * declaration of that C function to `bf_register.h' and add the necessary .c
 * files to the `CSRCS' line in the Makefile.
 ****************************************************************************/

typedef void (*registry) ();

static registry bi_function_registries[] =
{
#ifdef ENABLE_GC
    register_gc,
#endif
    register_collection,
    register_disassemble,
    register_extensions,
    register_execute,
    register_functions,
    register_list,
    register_log,
    register_map,
    register_numbers,
    register_objects,
    register_property,
    register_server,
    register_tasks,
    register_verbs,
    register_yajl,
    register_base64,
    register_fileio,
    register_system,
    register_exec,
    register_crypto
};

void
register_bi_functions()
{
    int loop, num_registries =
	sizeof(bi_function_registries) / sizeof(bi_function_registries[0]);

    for (loop = 0; loop < num_registries; loop++)
	(void) (*(bi_function_registries[loop])) ();
}

/*** register ***/

struct bft_entry {
    ref_ptr<const char> name;
    ref_ptr<const char> protect_str;
    ref_ptr<const char> verb_str;
    int minargs;
    int maxargs;
    std::vector<var_type> prototype;
    enum bft_func_type {
	SIMPLE, COMPLEX
    } type;
    union {
	bf_simple simple;
	bf_complex complex;
    } func;
    bf_read_type read;
    bf_write_type write;
    bool is_protected;

    bft_entry(const char *name, int minargs, int maxargs) {
	this->name = str_dup(name);
	std::stringstream s1;
	s1 << "protect_" << name;
	this->protect_str = str_dup(s1.str().c_str());
	std::stringstream s2;
	s2 << "bf_" << name;
	this->verb_str = str_dup(s2.str().c_str());
	this->minargs = minargs;
	this->maxargs = maxargs;
	this->is_protected = false;
    }

    bft_entry(const char *name, int minargs, int maxargs, bf_simple func,
	      const std::vector<var_type>& prototype)
	: bft_entry(name, minargs, maxargs)
    {
	this->prototype = prototype;
	this->type = SIMPLE;
	this->func.simple = func;
	this->read = 0;
	this->write = 0;
    }

    bft_entry(const char *name, int minargs, int maxargs, bf_complex func,
	      const std::vector<var_type>& prototype)
	: bft_entry(name, minargs, maxargs)
    {
	this->prototype = prototype;
	this->type = COMPLEX;
	this->func.complex = func;
	this->read = 0;
	this->write = 0;
    }

    bft_entry(const char *name, int minargs, int maxargs, bf_simple func,
	      const std::vector<var_type>& prototype,
	      bf_read_type read, bf_write_type write)
	: bft_entry(name, minargs, maxargs, func, prototype)
    {
	this->read = read;
	this->write = write;
    }

    bft_entry(const char *name, int minargs, int maxargs, bf_complex func,
	      const std::vector<var_type>& prototype,
	      bf_read_type read, bf_write_type write)
	: bft_entry(name, minargs, maxargs, func, prototype)
    {
	this->read = read;
	this->write = write;
    }

    bft_entry(bft_entry&& that) {
	std::swap(this->name, that.name);
	std::swap(this->protect_str, that.protect_str);
	std::swap(this->verb_str, that.verb_str);
	this->minargs = that.minargs;
	this->maxargs = that.maxargs;
	std::swap(this->prototype, that.prototype);
	this->type = that.type;
	this->func = that.func;
	this->read = that.read;
	this->write = that.write;
	this->is_protected = that.is_protected;
    }

    ~bft_entry() {
	if (this->name)
	    free_str(this->name);
	if (this->protect_str)
	    free_str(this->protect_str);
	if (this->verb_str)
	    free_str(this->verb_str);
    }
};

static std::vector<bft_entry> bf_table;

static std::vector<var_type>
make_prototype(int minargs, int maxargs, va_list args)
{
    // while we're here...
    if (bf_table.size() == MAX_FUNC) {
	panic("too many functions.\n");
    }

    std::vector<var_type> prototype;

    int num_arg_types = maxargs == -1 ? minargs : maxargs;

    for (int va_index = 0; va_index < num_arg_types; va_index++)
	prototype.push_back((var_type)va_arg(args, int));

    return prototype;
}

unsigned
register_function(const char *name, int minargs, int maxargs,
		  bf_simple func, ...)
{
    va_list args;

    va_start(args, func);
    auto prototype = make_prototype(minargs, maxargs, args);
    va_end(args);

    bf_table.emplace_back(name, minargs, maxargs, func,
			  prototype);


    return bf_table.size() - 1;
}

unsigned
register_function(const char *name, int minargs, int maxargs,
		  bf_complex func, ...)
{
    va_list args;

    va_start(args, func);
    auto prototype = make_prototype(minargs, maxargs, args);
    va_end(args);

    bf_table.emplace_back(name, minargs, maxargs, func,
			  prototype);


    return bf_table.size() - 1;
}

unsigned
register_function_with_read_write(const char *name, int minargs, int maxargs,
				  bf_simple func, bf_read_type read,
				  bf_write_type write, ...)
{
    va_list args;

    va_start(args, write);
    auto prototype = make_prototype(minargs, maxargs, args);
    va_end(args);

    bf_table.emplace_back(name, minargs, maxargs, func,
			  prototype, read, write);

    return bf_table.size() - 1;
}

unsigned
register_function_with_read_write(const char *name, int minargs, int maxargs,
				  bf_complex func, bf_read_type read,
				  bf_write_type write, ...)
{
    va_list args;

    va_start(args, write);
    auto prototype = make_prototype(minargs, maxargs, args);
    va_end(args);

    bf_table.emplace_back(name, minargs, maxargs, func,
			  prototype, read, write);

    return bf_table.size() - 1;
}

/*** looking up functions -- by name or num ***/

static const ref_ptr<const char> NO_SUCH_FUNCTION = str_dup("no such function");

const ref_ptr<const char>&
name_func_by_num(unsigned n)
{
    if (n >= bf_table.size())
	return NO_SUCH_FUNCTION;
    else
	return bf_table[n].name;
}

unsigned
number_func_by_name(const char* name)
{
    unsigned i;

    for (i = 0; i < bf_table.size(); i++)
	if (!mystrcasecmp(name, bf_table[i].name.expose()))
	    return i;

    return FUNC_NOT_FOUND;
}

/*** calling built-in functions ***/

package
call_bi_func(unsigned n, const Var& value, Byte func_pc,
	     Objid progr, void *vdata)
     /* call_bi_func will free value */
{
    struct bft_entry *f;

    if (n >= bf_table.size()) {
	errlog("CALL_BI_FUNC: Unknown function number: %d\n", n);
	free_var(value);
	return no_var_pack();
    }
    f = &bf_table[n];

    if (func_pc == 1) {		/* check arg types and count *ONLY* for first entry */
	int k, max;
	const Var *args = value.v.list.expose();

	/*
	 * Check permissions, if protected
	 */
	if ((!caller().is_obj() || caller().v.obj != SYSTEM_OBJECT) && f->is_protected) {
	    /* Try calling #0:bf_FUNCNAME(@ARGS) instead */
	    enum error e = call_verb2(SYSTEM_OBJECT, f->verb_str, Var::new_obj(SYSTEM_OBJECT), value, 0);

	    if (e == E_NONE)
		return tail_call_pack();

	    if (e == E_MAXREC || !is_wizard(progr)) {
		free_var(value);
		return make_error_pack(e == E_MAXREC ? e : E_PERM);
	    }
	}
	/*
	 * Check argument count
	 * (Can't always check in the compiler, because of @)
	 */
	if (args[0].v.num < f->minargs
	    || (f->maxargs != -1 && args[0].v.num > f->maxargs)) {
	    free_var(value);
	    return make_error_pack(E_ARGS);
	}
	/*
	 * Check argument types
	 */
	max = (f->maxargs == -1) ? f->minargs : args[0].v.num;

	for (k = 0; k < max; k++) {
	    var_type proto = f->prototype[k];
	    var_type arg = args[k + 1].type;

	    if (!(proto == TYPE_ANY
		  || (proto == TYPE_NUMERIC && (arg == TYPE_INT
						|| arg == TYPE_FLOAT))
		  || proto == arg)) {
		free_var(value);
		return make_error_pack(E_TYPE);
	    }
	}
    } else if (func_pc == 2 && vdata == &call_bi_func) {
	/* This is a return from calling #0:bf_FUNCNAME(@ARGS); return what
	 * it returned.  If it errored, what we do will be ignored.
	 */
	return make_var_pack(value);
    }
    /*
     * do the function
     * f->func is responsible for freeing/using up value.
     */
    if (bft_entry::SIMPLE == f->type && value.is_list()) {
	return (*(f->func.simple)) (static_cast<const List&>(value), progr);
    } else if (bft_entry::COMPLEX == f->type) {
	return (*(f->func.complex)) (value, progr, func_pc, vdata);
    } else {
	panic("Error in call_bi_func()");
	/* dead code - eliminate compiler warning */
	return no_var_pack();
    }
}

void
write_bi_func_data(bf_call_data* vdata, Byte f_id)
{
    if (f_id >= bf_table.size())
	errlog("WRITE_BI_FUNC_DATA: Unknown function number: %d\n", f_id);
    else if (bf_table[f_id].write)
	(*(bf_table[f_id].write)) (vdata);
}

static Byte *pc_for_bi_func_data_being_read;

Byte *
pc_for_bi_func_data(void)
{
    return pc_for_bi_func_data_being_read;
}

int
read_bi_func_data(Byte f_id, bf_call_data** bi_func_state, Byte* bi_func_pc)
{
    pc_for_bi_func_data_being_read = bi_func_pc;

    if (f_id >= bf_table.size()) {
	errlog("READ_BI_FUNC_DATA: Unknown function number: %d\n", f_id);
	*bi_func_state = 0;
	return 0;
    } else if (bf_table[f_id].read) {
	*bi_func_state = (*(bf_table[f_id].read)) ();
	if (*bi_func_state == 0) {
	    errlog("READ_BI_FUNC_DATA: Can't read data for %s()\n",
		   bf_table[f_id].name);
	    return 0;
	}
    } else {
	*bi_func_state = 0;
	/* The following code checks for the easily-detectable case of the
	 * bug described in the Version 1.8.0p4 entry in ChangeLog.txt.
	 */
	if (*bi_func_pc == 2 && dbio_input_version == DBV_Float
	    && strcmp(bf_table[f_id].name.expose(), "eval") != 0) {
	    oklog("LOADING: Warning: patching bogus return to `%s()'\n",
		  bf_table[f_id].name);
	    oklog("         (See 1.8.0p4 ChangeLog.txt entry for details.)\n");
	    *bi_func_pc = 0;
	}
    }
    return 1;
}


package
make_abort_pack(enum abort_reason reason)
{
    package p;

    p.kind = package::BI_KILL;
    p.u.ret = Var::new_int(reason);
    return p;
}

package
make_error_pack(enum error err)
{
    return make_raise_pack(err, unparse_error(err), zero);
}

package
make_raise_pack(enum error err, const char *msg, const Var& value)
{
    package p;

    p.kind = package::BI_RAISE;
    p.u.raise.code = Var::new_err(err);
    p.u.raise.msg = str_dup(msg);
    p.u.raise.value = value;

    return p;
}

package
make_var_pack(const Var& v)
{
    package p;

    p.kind = package::BI_RETURN;
    p.u.ret = v;

    return p;
}

package
no_var_pack(void)
{
    return make_var_pack(zero);
}

package
make_call_pack(Byte pc, bf_call_data* data)
{
    package p;

    p.kind = package::BI_CALL;
    p.u.call.pc = pc;
    p.u.call.data = data;

    return p;
}

package
tail_call_pack(void)
{
    return make_call_pack(0, 0);
}

package
make_suspend_pack(enum error(*proc) (vm, void *), void *data)
{
    package p;

    p.kind = package::BI_SUSPEND;
    p.u.susp.proc = proc;
    p.u.susp.data = data;

    return p;
}

static List
function_description(int i)
{
    List l, v;
    int j, nargs;

    const struct bft_entry& entry = bf_table[i];
    l = new_list(4);
    l[1] = str_ref_to_var(entry.name);
    l[2] = Var::new_int(entry.minargs);
    l[3] = Var::new_int(entry.maxargs);
    nargs = entry.maxargs == -1 ? entry.minargs : entry.maxargs;
    l[4] = v = new_list(nargs);
    for (j = 0; j < nargs; j++) {
	int proto = entry.prototype[j];
	v[j + 1] = Var::new_int(proto < 0 ? proto : (proto & TYPE_DB_MASK));
    }

    return l;
}

static package
bf_function_info(const List& arglist, Objid progr)
{
    unsigned int i;

    if (arglist.length() == 1) {
	i = number_func_by_name(arglist[1].v.str.expose());
	if (i == FUNC_NOT_FOUND) {
	    free_var(arglist);
	    return make_error_pack(E_INVARG);
	}
	Var r = function_description(i);
	free_var(arglist);
	return make_var_pack(r);
    } else {
	List l = new_list(bf_table.size());
	for (i = 0; i < bf_table.size(); i++)
	    l[i + 1] = function_description(i);
	free_var(arglist);
	return make_var_pack(l);
    }
}

static void
load_server_protect_function_flags(void)
{
    unsigned int i;

    for (i = 0; i < bf_table.size(); i++) {
	bf_table[i].is_protected
	    = server_flag_option(bf_table[i].protect_str.expose(), 0);
    }
    oklog("Loaded protect cache for %d builtin functions\n", i);
}

int32_t _server_int_option_cache[SVO__CACHE_SIZE];

void
load_server_options(void)
{
    int value;

    load_server_protect_function_flags();

# define _BP_DO(PROPERTY, property)				\
      _server_int_option_cache[SVO_PROTECT_##PROPERTY]		\
	  = server_flag_option("protect_" #property, 0);	\

    BUILTIN_PROPERTIES(_BP_DO);

# undef _BP_DO

# define _SVO_DO(SVO_MISC_OPTION, misc_option,			\
		 kind, DEFAULT, CANONICALIZE)			\
      value = server_##kind##_option(#misc_option, DEFAULT);	\
      CANONICALIZE;						\
      _server_int_option_cache[SVO_MISC_OPTION] = value;	\

    SERVER_OPTIONS_CACHED_MISC(_SVO_DO, value);

# undef _SVO_DO
}

static package
bf_load_server_options(const List& arglist, Objid progr)
{
    free_var(arglist);

    if (!is_wizard(progr)) {
	return make_error_pack(E_PERM);
    }
    load_server_options();

    return no_var_pack();
}

void
register_functions(void)
{
    register_function("function_info", 0, 1, bf_function_info, TYPE_STR);
    register_function("load_server_options", 0, 0, bf_load_server_options);
}
