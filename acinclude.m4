dnl @synopsis AX_RANDOM_DEVICE
dnl
dnl This macro will check for a random device, allowing the user to explicitly
dnl set the path. The user uses '--with-random=FILE' as an argument to
dnl configure.
dnl
dnl If A random device is found then HAVE_RANDOM_DEVICE is set to 1 and
dnl RANDOM_DEVICE contains the path.
dnl
dnl @category Miscellaneous
dnl @author Martin Ebourne
dnl @version 2005/07/01
dnl @license AllPermissive

AC_DEFUN([AX_RANDOM_DEVICE], [
  AC_ARG_WITH([random],
    [AC_HELP_STRING([--with-random=FILE], [Use FILE as random number seed [auto-detected]])],
    [RANDOM_DEVICE="$withval"],
    [AC_CHECK_FILE("/dev/random", [RANDOM_DEVICE="/dev/random";])])
  if test "x$RANDOM_DEVICE" != "x" ; then
    AC_DEFINE([HAVE_RANDOM_DEVICE], 1,
              [Define to 1 (and set RANDOM_DEVICE) if a random device is available])
    AC_SUBST([RANDOM_DEVICE])
    AC_DEFINE_UNQUOTED([RANDOM_DEVICE], ["$RANDOM_DEVICE"],
                       [Define to the filename of the random device (and set HAVE_RANDOM_DEVICE)])
  fi
  ])

dnl Original LambdaMOO m4 macros from configure.in...

dnl ***************************************************************************
dnl	MOO_FUNC_DECL_CHECK(header, func,
dnl			    if-declared [, if-not-declared[, extra-hdr]])
dnl Do `if-declared' is `func' is declared in `header', and if-not-declared
dnl otherwise.  If `extra-hdr' is provided, it is added after the #include of
dnl `header'.
define(MOO_FUNC_DECL_CHECK, [
changequote(,)dnl
pattern="[^_a-zA-Z0-9]$2 *\("
changequote([,])dnl
AC_PROGRAM_EGREP($pattern, [
#include <$1>
$5
], [$3], [$4])])

dnl ***************************************************************************
dnl 	MOO_NDECL_FUNCS(header, func1 func2 ...[, extra-hdr])
dnl Defines NDECL_func1, NDECL_func2, ... if they are not declared in header.
dnl
define(MOO_NDECL_FUNCS, [
changequote(,)dnl
trfrom='[a-z]' trto='[A-Z]'
changequote([,])dnl
for func in $2
do
echo "checking whether $func is declared in $1"
MOO_FUNC_DECL_CHECK($1, $func, ,
	AC_DEFINE(NDECL_`echo $func | tr "$trfrom" "$trto"`), $3)
done
])

dnl ***************************************************************************
dnl	MOO_VAR_DECL_CHECK(header, variable,
dnl			   if-declared [, if-not-declared[, extra-hdr]])
dnl Do `if-declared' is `variable' is declared in `header', and if-not-declared
dnl otherwise.  If `extra-hdr' is provided, it is added after the #include of
dnl `header'.
define(MOO_VAR_DECL_CHECK, [
changequote(,)dnl
pattern="[^_a-zA-Z0-9]$2"
changequote([,])dnl
AC_PROGRAM_EGREP($pattern, [
#include <$1>
$5
], $3, $4)])

dnl ***************************************************************************
dnl 	MOO_NDECL_VARS(header, var1 var2 ...[, extra-hdr])
dnl Defines NDECL_var1, NDECL_var2, ... if they are not declared in header.
dnl
define(MOO_NDECL_VARS, [
changequote(,)dnl
trfrom='[a-z]' trto='[A-Z]'
changequote([,])dnl
for var in $2
do
echo "checking whether $var is declared in $1"
MOO_VAR_DECL_CHECK($1, $var, ,
	AC_DEFINE(NDECL_`echo $var | tr "$trfrom" "$trto"`), $3)
done
])

dnl ***************************************************************************
dnl 	MOO_HEADER_STANDS_ALONE(header [, extra-code])
dnl Defines header_NEEDS_HELP if can't be compiled all by itself.
define(MOO_HEADER_STANDS_ALONE, [
changequote(,)dnl
trfrom='[a-z]./' trto='[A-Z]__'
changequote([,])dnl
AC_COMPILE_CHECK(self-sufficiency of $1, [
#include <$1>
$2
], , , AC_DEFINE(`echo $1 | tr "$trfrom" "$trto"`_NEEDS_HELP))
echo
])

dnl ***************************************************************************
dnl	MOO_HAVE_FUNC_LIBS(func1 func2 ..., lib1 "lib2a lib2b" lib3 ...)
dnl For each `func' in turn, if `func' is defined using the current LIBS value,
dnl leave LIBS alone.  Otherwise, try adding each of the given libs to LIBS in
dnl turn, stopping when one of them succeeds in providing `func'.  Define
dnl HAVE_func if `func' is eventually found.
define(MOO_HAVE_FUNC_LIBS, [
for func in $1
do
  changequote(,)dnl
  trfrom='[a-z]' trto='[A-Z]'
  var=HAVE_`echo $func | tr "$trfrom" "$trto"`
  changequote([,])dnl
  AC_FUNC_CHECK($func, AC_DEFINE_UNQUOTED($var), [
    SAVELIBS="$LIBS"
    for lib in $2
    do
      LIBS="$LIBS $lib"
      AC_FUNC_CHECK($func, [AC_DEFINE_UNQUOTED($var)
			 break],
		    LIBS="$SAVELIBS")
    done
    ])
done
])

dnl ***************************************************************************
dnl	MOO_HAVE_HEADER_DIRS(header1 header2 ..., dir1 dir2 ...)
dnl For each `header' in turn, if `header' is found using the current CC value
dnl leave CC alone.  Otherwise, try adding each of the given `dir's to CC in
dnl turn, stopping when one of them succeeds in providing `header'.  Define
dnl HAVE_header if `header' is eventually found.
define(MOO_HAVE_HEADER_DIRS, [
for hdr in $1
do
  changequote(,)dnl
  trfrom='[a-z]./' trto='[A-Z]__'
  var=HAVE_`echo $hdr | tr "$trfrom" "$trto"`
  changequote([,])dnl
  AC_HEADER_CHECK($hdr, AC_DEFINE($var), [
    SAVECC="$CC"
    for dir in $2
    do
      CC="$CC $dir"
      AC_HEADER_CHECK($hdr, [AC_DEFINE($var)
			     break],
		      CC="$SAVECC")
    done
    ])
done
])
