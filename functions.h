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

#ifndef Functions_h
#define Functions_h 1

#include "my-stdio.h"

#include "config.h"
#include "execute.h"
#include "program.h"
#include "structures.h"

/**
 * Holds state used by builtins so that the operation can
 * continue/complete across server restarts. The destructor is virtual
 * because `delete` is called on the base class at one point in the
 * code.
 */
struct bf_call_data {
    virtual ~bf_call_data() {};
};

typedef struct {
    enum {
	BI_RETURN,		/* Normal function return */
	BI_RAISE,		/* Raising an error */
	BI_CALL,		/* Making a nested verb call */
	BI_SUSPEND,		/* Suspending the current task */
	BI_KILL			/* Killing the current task */
    } kind;
    union u {
	Var ret;
	struct {
	    Var code;
	    ref_ptr<const char> msg;
	    Var value;
	} raise;
	struct {
	    Byte pc;
	    bf_call_data* data;
	} call;
	struct {
	    enum error (*proc) (vm, void *);
	    void *data;
	} susp;
	u() {}
    } u;
} package;

void register_bi_functions();

enum abort_reason {
    ABORT_KILL    = -1, 	/* kill_task(task_id()) */
    ABORT_SECONDS = 0,		/* out of seconds */
    ABORT_TICKS   = 1		/* out of ticks */
};

package make_abort_pack(enum abort_reason reason);
package make_error_pack(enum error err);
package make_raise_pack(enum error err, const char *msg, const Var& value);
package make_var_pack(const Var& v);
package no_var_pack(void);
package make_call_pack(Byte pc, bf_call_data* data);
package tail_call_pack(void);
package make_suspend_pack(enum error (*)(vm, void *), void *);

typedef package(*bf_simple) (const List&, Objid);
typedef package(*bf_complex) (const Var&, Objid, Byte, void *);
typedef void (*bf_write_type) (bf_call_data* vdata);
typedef bf_call_data* (*bf_read_type) (void);

#define MAX_FUNC         256
#define FUNC_NOT_FOUND   MAX_FUNC
/* valid function numbers are 0 - 255, or a total of 256 of them.
   function number 256 is reserved for func_not_found signal.
   hence valid function numbers will fit in one byte but the
   func_not_found signal will not */

extern const ref_ptr<const char>& name_func_by_num(unsigned);
extern unsigned number_func_by_name(const char*);

extern unsigned register_function(const char *, int, int, bf_simple, ...);
extern unsigned register_function(const char *, int, int, bf_complex, ...);

extern unsigned register_function_with_read_write(const char *, int, int,
						  bf_simple, bf_read_type,
						  bf_write_type, ...);
extern unsigned register_function_with_read_write(const char *, int, int,
						  bf_complex, bf_read_type,
						  bf_write_type, ...);

extern package call_bi_func(unsigned, const Var&, Byte, Objid, void *);
/* will free or use Var arglist */

extern void write_bi_func_data(bf_call_data* vdata, Byte f_id);
extern int read_bi_func_data(Byte f_id, bf_call_data** bi_func_state,
			     Byte* bi_func_pc);
extern Byte *pc_for_bi_func_data(void);

extern void load_server_options(void);

#endif
