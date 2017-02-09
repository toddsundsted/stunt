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

#include "bf_register.h"
#include "collection.h"
#include "functions.h"
#include "list.h"
#include "map.h"
#include "utils.h"

struct ismember_data {
    int i;
    Var value;
    int case_matters;
};

static int
do_map_iteration(const Var& key, const Var& value, void *data, int first)
{
    struct ismember_data *ismember_data = (struct ismember_data *)data;

    if (equality(value, ismember_data->value, ismember_data->case_matters)) {
	return ismember_data->i;
    }

    ismember_data->i++;

    return 0;
}

int
ismember(const Var& lhs, const Var& rhs, int case_matters)
{
    if (rhs.is_list()) {
	int i;

	const List& list = static_cast<const List&>(rhs);

	for (i = 1; i <= list.length(); i++) {
	    if (equality(lhs, list[i], case_matters)) {
		return i;
	    }
	}

	return 0;
    } else if (rhs.is_map()) {
	struct ismember_data ismember_data;

	ismember_data.i = 1;
	ismember_data.value = lhs;
	ismember_data.case_matters = case_matters;

	return mapforeach(static_cast<const Map&>(rhs), do_map_iteration, &ismember_data);
    } else {
	return 0;
    }
}

/**** built in functions ****/

static package
bf_is_member(const List& arglist, Objid progr)
{
    Var r;
    const Var& lhs = arglist[1];
    const Var& rhs = arglist[2];

    if (!rhs.is_list() && !rhs.is_map()) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }

    r = Var::new_int(ismember(lhs, rhs, 1));

    free_var(arglist);

    return make_var_pack(r);
}

void
register_collection(void)
{
    register_function("is_member", 2, 2, bf_is_member, TYPE_ANY, TYPE_ANY);
}
