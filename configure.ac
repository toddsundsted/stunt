dnl Process this file with `autoconf' to create `configure'.
dnl
dnl Copyright (c) 1992, 1995, 1996 Xerox Corporation.  All rights reserved.
dnl Portions of this code were written by Stephen White, aka ghond.
dnl Use and copying of this software and preparation of derivative works based
dnl upon this software are permitted.  Any distribution of this software or
dnl derivative works must comply with all applicable United States export
dnl control laws.  This software is made available AS IS, and Xerox Corporation
dnl makes no warranty about the software, its performance or its conformity to
dnl any specification.  Any person obtaining a copy of this software is
dnl requested to send their name and post office or electronic mail address to:
dnl   Pavel Curtis
dnl   Xerox PARC
dnl   3333 Coyote Hill Rd.
dnl   Palo Alto, CA 94304
dnl   Pavel@Xerox.Com
dnl
AC_PREREQ([2.61])
AC_INIT([Waverous LambdaMOO], [0.2], [wainstead@gmail.com], [waverous-lambdamoo], [http://code.google.com/p/waverous/])
AC_CONFIG_SRCDIR([functions.c])
AC_CONFIG_HEADERS([config.h])

AC_LANG([C++])

dnl ***************************************************************************
dnl Start of actual configuration tests
dnl ***************************************************************************

AC_PROG_YACC
AC_PROG_CXX
AC_PROG_INSTALL
AC_GCC_TRADITIONAL
AC_EGREP_CPP(yes, [
#if __GNUC__==2
  yes
#endif
], [CC="$CC -Wall -Wwrite-strings"])
test -n "$GCC" && CC="$CC -g"
AC_PROG_CPP
AC_AIX
AC_ISC_POSIX
AC_MINIX
AC_SCO_INTL
AC_PROG_CC_STDC
CPP=""
AC_HEADER_CHECK(sys/bsdtypes.h, AC_DEFINE(NEED_BSDTYPES_H))
AC_HEADER_CHECK(sys/select.h, AC_DEFINE(NEED_SELECT_H))
AC_HEADER_CHECK(memory.h, AC_DEFINE(NEED_MEMORY_H))
AC_PID_T
AC_MODE_T
AC_SIZE_T
AC_STRUCT_TM
AC_TIMEZONE

MOO_HAVE_FUNC_LIBS(sqrt, -lm /lib/libm.a "-static -lm")
MOO_HAVE_FUNC_LIBS(mkfifo waitpid sigemptyset, -lposix /lib/libposix.a)
MOO_HAVE_FUNC_LIBS(accept, "-lsocket -lnsl" -lsocket -linet)
MOO_HAVE_FUNC_LIBS(t_open, -lnsl -lnsl_s)
MOO_HAVE_FUNC_LIBS(crypt, -lcrypt -lcrypt_d)
AC_HAVE_HEADERS(unistd.h sys/cdefs.h stdlib.h tiuser.h machine/endian.h)
AC_HAVE_FUNCS(remove rename poll select strerror strftime strtoul matherr)
AC_HAVE_FUNCS(random lrand48 wait3 wait2 sigsetmask sigprocmask sigrelse)
MOO_NDECL_FUNCS(ctype.h, tolower)
MOO_NDECL_FUNCS(fcntl.h, fcntl)
MOO_NDECL_FUNCS(netinet/in.h, htonl)
dnl *** this next should really be some variety of AC_CHECK_TYPE
dnl *** but this is just a grep test anyway   --wrog
MOO_NDECL_VARS(netinet/in.h, in_addr_t)
MOO_NDECL_FUNCS(sys/ioctl.h, ioctl)
MOO_NDECL_FUNCS(poll.h, poll)
MOO_NDECL_FUNCS(signal.h, kill sigemptyset sigprocmask sigrelse)
MOO_NDECL_FUNCS(sys/socket.h, accept bind shutdown)
MOO_NDECL_FUNCS(sys/stat.h, fstat)
MOO_NDECL_FUNCS(stdio.h, fclose perror remove vfprintf)
MOO_NDECL_FUNCS(stdlib.h, random srandom strtod strtol strtoul)
MOO_NDECL_FUNCS(string.h, memcpy memset strerror, [
#if NEED_MEMORY_H
#include <memory.h>
#endif
])

MOO_NDECL_FUNCS(sys/time.h, getitimer setitimer select, [
#if NEED_SELECT_H
#include <sys/types.h>
#include <sys/select.h>
#endif
])
MOO_NDECL_FUNCS(time.h, strftime time)
MOO_NDECL_VARS(time.h, tzname)
MOO_NDECL_FUNCS(tiuser.h, t_open)
MOO_NDECL_VARS(tiuser.h, t_errlist)
MOO_NDECL_FUNCS(unistd.h, fork)
MOO_NDECL_FUNCS(sys/wait.h, waitpid)

MOO_HEADER_STANDS_ALONE(arpa/inet.h, [struct in_addr foo;])
MOO_HEADER_STANDS_ALONE(signal.h)
MOO_HEADER_STANDS_ALONE(sys/socket.h)
MOO_HEADER_STANDS_ALONE(sys/stat.h)
MOO_HEADER_STANDS_ALONE(time.h)

dnl ***************************************************************************
echo checking for string.h declaring some functions incorrectly
echo "
#include <sys/types.h>
#include <string.h>
#if NEED_MEMORY_H
#include <memory.h>
#endif
" > conftest.c
$CC $DEFS conftest.c -o conftest $LIBS > conftest.out 2>&1
if grep "conflicting types for built-in function" conftest.out >/dev/null; then
  AC_DEFINE(USE_OWN_STRING_H)
fi
rm -f conftest*

dnl ***************************************************************************
echo "checking for incompatibility between <sys/ioctl.h> and <stropts.h>"
AC_TEST_CPP([
#include <sys/ioctl.h>
#include <stropts.h>
], , AC_DEFINE(UNDEF_IO_IN_STROPTS_H))

dnl ***************************************************************************
echo "checking whether or not fstat() can tell how much data is in a FIFO"
AC_TEST_PROGRAM([#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
main()
{
#ifdef NeXT
/* The NeXT claims to have FIFOs, but using them panics the kernel... */
  exit(-1);
#endif
  int	rfd, wfd, result; struct stat st;
  unlink("/tmp/conftest-fifo");
  result = (mknod("/tmp/conftest-fifo", 0666 | S_IFIFO, 0) < 0
	    || (rfd = open("/tmp/conftest-fifo", O_RDONLY | O_NDELAY)) < 0
	    || (wfd = open("/tmp/conftest-fifo", O_WRONLY)) < 0
	    || write(wfd, "foo", 3) != 3
	    || fstat(rfd, &st) < 0
	    || st.st_size != 3);
  unlink("/tmp/conftest-fifo");
  exit(result);
}
], AC_DEFINE(FSTAT_WORKS_ON_FIFOS))

dnl ***************************************************************************
echo "checking whether or not select() can be used on FIFOs"
AC_TEST_PROGRAM([#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef FD_ZERO
#define	NFDBITS		(sizeof(fd_set)*8)
#define	FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#define	FD_SET(n, p)	((p)->fds_bits[0] |= (1L<<((n)%NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[0] &  (1L<<((n)%NFDBITS)))
#endif /* FD_ZERO */
main()
{
#ifdef NeXT
/* The NeXT claims to have FIFOs, but using them panics the kernel... */
  exit(-1);
#endif
  int	rfd, wfd, result; fd_set input; struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  unlink("/tmp/conftest-fifo");
  result = (mknod("/tmp/conftest-fifo", 0666 | S_IFIFO, 0) < 0
	    || (rfd = open("/tmp/conftest-fifo", O_RDONLY | O_NDELAY)) < 0
	    || (wfd = open("/tmp/conftest-fifo", O_WRONLY)) < 0
	    || (FD_ZERO(&input), FD_SET(rfd, &input),
		select(rfd + 1, &input, 0, 0, &tv) != 0)
	    || write(wfd, "foo", 3) != 3
	    || (FD_ZERO(&input), FD_SET(rfd, &input),
		select(rfd + 1, &input, 0, 0, &tv) != 1)
	    || !FD_ISSET(rfd, &input));
  unlink("/tmp/conftest-fifo");
  exit(result);
}
], AC_DEFINE(SELECT_WORKS_ON_FIFOS))

dnl ***************************************************************************
echo "checking whether or not poll() can be used on FIFOs"
AC_TEST_PROGRAM([#include <sys/types.h>
#include <poll.h>
#include <sys/stat.h>
#include <fcntl.h>
main()
{
  int	rfd, wfd, result; struct pollfd fds[1];
  unlink("/tmp/conftest-fifo");
  result = (mknod("/tmp/conftest-fifo", 0666 | S_IFIFO, 0) < 0
	    || (rfd = open("/tmp/conftest-fifo", O_RDONLY | O_NDELAY)) < 0
	    || (wfd = open("/tmp/conftest-fifo", O_WRONLY)) < 0
	    || write(wfd, "foo", 3) != 3
	    || (fds[0].fd = rfd, fds[0].events = POLLIN, poll(fds, 1, 1) != 1)
	    || (fds[0].revents & POLLIN) == 0);
  unlink("/tmp/conftest-fifo");
  exit(result);
}
], AC_DEFINE(POLL_WORKS_ON_FIFOS))

dnl ***************************************************************************
echo checking whether POSIX-style non-blocking I/O works
AC_TEST_PROGRAM([#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
handler(int sig) { }
main ()
{ /* Testing a POSIX feature, so assume FIFOs */
#ifdef NeXT
/* The NeXT claims to have FIFOs, but using them panics the kernel... */
  exit(-1);
#endif
  int	rfd, wfd, flags, result; char buffer[10];
  unlink("/tmp/conftest-fifo");
  signal(SIGALRM, handler);
  result = (mknod("/tmp/conftest-fifo", 0666 | S_IFIFO, 0) < 0
	    || (rfd = open("/tmp/conftest-fifo", O_RDONLY | O_NONBLOCK)) < 0
	    || (wfd = open("/tmp/conftest-fifo", O_WRONLY)) < 0
	    || (flags = fcntl(rfd, F_GETFL, 0)) < 0
	    || fcntl(rfd, F_SETFL, flags | O_NONBLOCK) < 0
	    || (alarm(3), read(rfd, buffer, 10) >= 0)
	    || (alarm(0), errno != EAGAIN));
  unlink("/tmp/conftest-fifo");
  exit(result);
}
], AC_DEFINE(POSIX_NONBLOCKING_WORKS))

dnl ***************************************************************************
echo checking which MOO networking configurations are likely to work...
define(MOO_ADD_NET_CONFIG,[
    NETWORK_CONFIGURATIONS="$NETWORK_CONFIGURATIONS $1"])
NETWORK_CONFIGURATIONS="NP_SINGLE"

AC_HEADER_CHECK(sys/socket.h, [
	MOO_ADD_NET_CONFIG(NS_BSD/NP_LOCAL)
	AC_PROGRAM_CHECK(have_telnet, telnet, yes, no)
        test x$have_telnet = xyes && MOO_ADD_NET_CONFIG(NS_BSD/NP_TCP)])
AC_PROGRAM_EGREP(yes-there-are-FIFOs, [#include <sys/stat.h>
#if defined(S_IFIFO) && !defined(NeXT) && \
    (SELECT_WORKS_ON_FIFOS || POLL_WORKS_ON_FIFOS || FSTAT_WORKS_ON_FIFOS)
	yes-there-are-FIFOs
#endif
], MOO_ADD_NET_CONFIG(NS_SYSV/NP_LOCAL))
AC_PROGRAM_EGREP(yes, [
#if HAVE_TIUSER_H && HAVE_T_OPEN && HAVE_POLL
yes
#endif
], test -r /dev/tcp && MOO_ADD_NET_CONFIG(NS_SYSV/NP_TCP))

AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h malloc.h memory.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h])



dnl Let the user choose to enable expat or not.
dnl Expat support: search for the library, then expat.h.
AC_ARG_WITH([expat],
        [AS_HELP_STRING([--with-expat], [enable XML parsing via the Expat library @<:@default: yes@:>@])],
        [expat=${withval}],
        [expat=yes])

if test "x${expat}" = xyes; then

   have_expat=no
   AC_SEARCH_LIBS([XML_ParserCreate], [expat], [have_expat=yes])

   if test "x${have_expat}" = xyes; then

      AC_CHECK_HEADERS([expat.h], [], [have_expat=no])

   fi

   if test "x${have_expat}" = xno; then
      AC_MSG_ERROR([
  ----------------------------------------------------
  Unable to find the Expat XML library on this system.
  ----------------------------------------------------])
   fi
else
  AC_MSG_NOTICE([The Expat XML library will not be included in this build])
fi



dnl Include (or don't) the ext-FUP extension for manipulating external files
AC_ARG_WITH([fup],
        [AS_HELP_STRING([--with-fup],	[Include the ext-FUP extension for manipulating external files @<:@default: no@:>@])],
        [fup=${withval}],
        [fup=no])

if test "x${fup}" = xyes; then
   AC_MSG_NOTICE([The ext-FUP file manipulation package included])
   AC_DEFINE([USE_EXT_FUP], 1, [The ext-FUP external file manipulation included])
else
   AC_MSG_NOTICE([The ext-FUP file manipulation extension will not be included in this build])
fi

dnl Include (or don't) the File I/O extension for manipulating external files
AC_ARG_WITH([fileio],
        [AS_HELP_STRING([--with-fileio],	[Include the File I/O extension for manipulating external files @<:@default: no@:>@])],
        [fileio=${withval}],
        [fileio=no])

if test "x${fileio}" = xyes; then
   AC_MSG_NOTICE([The File I/O file manipulation package included])
   AC_DEFINE([USE_FILEIO], 1, [File I/O external file manipulation included])
else
   AC_MSG_NOTICE([The File I/O file manipulation extension will not be included in this build])
fi


dnl Enable/disable user command logging
AC_ARG_WITH([command-logging],
        [AS_HELP_STRING([--with-command-logging],	[Log user commands since the last database checkpoint if the server panics @<:@default: no@:>@])],
        [log_commands=${withval}],
        [log_commands=no])

if test "x${log_commands}" = xyes; then
   AC_MSG_NOTICE([All user commands will be logged since the last database checkpoint if the server panics])
   AC_DEFINE([LOG_COMMANDS], 1, [All user commands will be logged since the last database checkpoint if the server panics])
else
   AC_MSG_NOTICE([No user commands will be logged in the event of a server panic])
fi


dnl Enable/disable unforked checkpoints
AC_ARG_WITH([forked-checkpoints],
        [AS_HELP_STRING([--with-forked-checkpoints],	[fork() when doing a database checkpoint @<:@default: yes@:>@])],
        [forked_checkpoints=${withval}],
        [forked_checkpoints=yes])

if test "x${forked_checkpoints}" = xno; then
   AC_MSG_NOTICE([The server will not fork() when doing a database checkpoint])
   AC_DEFINE([UNFORKED_CHECKPOINTS], 1, [The server will not fork() when doing a database checkpoint])
else
   AC_MSG_NOTICE([The server will fork() when doing a database checkpoint])
fi

dnl Enable/disable outbound network connections
AC_ARG_WITH([outbound-network],
        [AS_HELP_STRING([--with-outbound-network],
        [Outbound network connections. Args: enabled/disabled/blocked  @<:@default: blocked@:>@])],
        [outbound_network=${withval}],
        [outbound_network=blocked])

if test "x${outbound_network}" = xblocked || test "x${outbound_network}" = xno; then
   AC_MSG_NOTICE([Outbound network connections will be completely blocked])
elif test "x${outbound_network}" = xdisabled; then
   AC_MSG_NOTICE([Outbound network connections are disabled by default (enable with +O at runtime)])
   AC_DEFINE([OUTBOUND_NETWORK], 0, [Outbound network connections are disabled by default (enable with +O at runtime)])
elif test "x${outbound_network}" = xenabled || test "x${outbound_network}" = xyes; then
   AC_MSG_NOTICE([Outbound network connections will be enabled by default (disable with -O at runtime)])
   AC_DEFINE([OUTBOUND_NETWORK], 1, [Outbound network connections are enabled by default (disable with -O at runtime)])
else
   AC_MSG_ERROR([Invalid argument for outbound-network: '${outbound_network}' (valid options are: blocked/enabled/disabled)])
fi


dnl Let the user choose to include openssl or not
AC_ARG_WITH([openssl],
        [AS_HELP_STRING([--with-openssl], [include openssl (Secure Socket Layers) (experimental) @<:@default: no@:>@])],
        [openssl=${withval}],
        [openssl=no])


if test "x${openssl}" = xyes; then
   have_openssl=no
   AC_SEARCH_LIBS([SSL_CTX_ctrl], [ssl], [have_openssl=yes])

   if test "x${have_openssl}" = xyes; then
      AC_CHECK_HEADERS([openssl/ssl.h])
      AC_CHECK_HEADERS([openssl/err.h])
   fi

   if test "x${have_openssl}" = xno; then
   AC_MSG_ERROR([
  ----------------------------------------------------
  Unable to find the openssl library on this system.
  ----------------------------------------------------])
   else
   AC_DEFINE([USE_OPENSSL], 1, [Using openssl for Secure Sockets Layer])
   fi
else
   AC_MSG_NOTICE([openssl will not be included in this build])
fi

dnl Check for GNU gperf, issue a warning if not found. gperf is not a
dnl must-have for the end user unless they want to tinker with the Moo
dnl language.
AC_CHECK_PROG(HAVE_GPERF, gperf, true, false)

if test "x$HAVE_GPERF" = "xfalse"; then
   AC_MSG_WARN([GNU gperf not found. This probably won't matter unless you intend to hack on the MOO language itself.])
else
   AC_MSG_NOTICE([GNU gperf found])
fi

AC_C_INLINE
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_FUNC_WAIT3
AC_HEADER_STDBOOL

AC_PROG_LN_S
AC_TYPE_SSIZE_T
AC_CHECK_FUNCS([alarm bzero floor gethostbyaddr gethostbyname getpagesize gettimeofday inet_ntoa memmove memset mkdir mkfifo modf pow re_comp rmdir select socket strchr strcspn strerror strrchr strstr strtol strtoul])

echo "----------------------------------------------------------------------"
echo "| The following networking configurations will probably work on your"
echo "| system; any configuration *not* listed here almost certainly will"
echo "| *not* work on your system:"
echo "|"
echo "|   $NETWORK_CONFIGURATIONS"
echo "----------------------------------------------------------------------"


AC_OUTPUT(Makefile)


# Local Variables:
# mode: autoconf
# End:
