dnl LambdaMOO-specific version of AC_FUNC_CHECK, uses <assert.h> instead of
dnl <ctype.h>, since OSF/1's <ctype.h> includes <sys/types.h> which includes
dnl <sys/select.h> which declares select() correctly in conflict with the
dnl bogus `extern char foo();' declaration below.  This change is adapted
dnl from autoconf-2.4, which we ought to start using at some point.
dnl
undefine([AC_FUNC_CHECK])dnl
define(AC_FUNC_CHECK,
[ifelse([$3], , [AC_COMPILE_CHECK($1, [#include <assert.h>], [
/* The GNU C library defines this for functions which it implements
    to always fail with ENOSYS.  Some functions are actually named
    something starting with __ and the normal name is an alias.  */
#if defined (__stub_$1) || defined (__stub___$1)
choke me
#else
/* Override any gcc2 internal prototype to avoid an error.  */
extern char $1(); $1();
#endif
],
$2)], [AC_COMPILE_CHECK($1, [#include <assert.h>], [
/* The GNU C library defines this for functions which it implements
    to always fail with ENOSYS.  Some functions are actually named
    something starting with __ and the normal name is an alias.  */
#if defined (__stub_$1) || defined (__stub___$1)
choke me
#else
/* Override any gcc2 internal prototype to avoid an error.  */
extern char $1(); $1();
#endif
],
$2, $3)])dnl
])dnl
