/* This is just a stub for now; various db-layer-specific tuning functions
 * will eventually live here.
 */

#ifndef DB_Tune_h
#define DB_Tune_h 1

#include "structures.h"

extern void db_log_cache_stats(void);
extern List db_verb_cache_stats(void);

#endif
