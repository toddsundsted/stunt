/**
 * A string intern table.
 *
 * The intern table holds a list of strings.  Given a new string, we
 * either duplicate it and add it to the table or return a reference
 * to an existing copy of the string from the table if present.
 *
 * This implementation has one big table that's designed to be used
 * during db load then freed all at once.
 *
 * @file
 */

#ifndef Str_Intern_h
#define Str_Intern_h

#include "storage.h"

/* 0 for a default size */
extern void str_intern_open(int table_size);
extern void str_intern_close(void);

/**
 * Make an immutable copy of `string`.
 *
 * If there's an intern table open, possibly share storage.
 */
extern ref_ptr<const char> str_intern(const char *string);

#endif
