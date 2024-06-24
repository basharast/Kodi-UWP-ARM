dnl --------------------------------------------------------------
dnl Making releases: dvdnav_sub += 1; change DVDNAV_LT_* accordingly
dnl
dnl These are defined in m4 so they can be passed to AC_INIT
dnl --------------------------------------------------------------
m4_define([dvdnav_major], [6])
m4_define([dvdnav_minor], [1])
m4_define([dvdnav_sub],   [1])
m4_define([dvdnav_pre],   [])

AC_INIT([libdvdnav], [dvdnav_major.dvdnav_minor.dvdnav_sub[]dvdnav_pre])
AC_CONFIG_SRCDIR([src/dvdnav.c])

AC_PREREQ([2.53])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2 no-dist-gzip subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

LT_INIT

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

DVDNAV_MAJOR="dvdnav_major"
DVDNAV_MINOR="dvdnav_minor"
DVDNAV_SUB="dvdnav_sub"
DVDNAV_PRE="dvdnav_pre"

DVDREAD_MIN_VERSION="6.0.0"

AC_SUBST(DVDNAV_MAJOR)
AC_SUBST(DVDNAV_MINOR)
AC_SUBST(DVDNAV_SUB)
AC_SUBST(DVDREAD_MIN_VERSION)

dnl The libtool version numbers (DVDNAV_LT_*); Don't even think about faking this!
dnl
dnl immediately before every release do:
dnl ===================================
dnl if (the interface is totally unchanged from previous release)
dnl    DVDNAV_LT_REVISION ++;
dnl else { /* interfaces have been added, removed or changed */
dnl    DVDNAV_LT_REVISION = 0;
dnl    DVDNAV_LT_CURRENT ++;
dnl    if (any interfaces have been _added_ since last release)
dnl       AGE ++;
dnl    if (any interfaces have been _removed_ or _incompatibly changed_)
dnl       AGE = 0;
dnl }
dnl
dnl If you want to know more about what you are doing, here are some details:
dnl  * DVDNAV_LT_CURRENT is the current API version
dnl  * DVDNAV_LT_REVISION is an internal revision number which is increased when the API
dnl    itself did not change
dnl  * DVDNAV_LT_AGE is the number of previous API versions still supported by this library
dnl  * libtool has its own numbering scheme, because local library numbering schemes
dnl    are platform dependent
dnl  * in Linux, the library will be named
dnl    libname.so.(DVDNAV_LT_CURRENT - DVDNAV_LT_AGE).DVDNAV_LT_AGE.DVDNAV_LT_REVISION

DVDNAV_LT_CURRENT=8
DVDNAV_LT_AGE=4
DVDNAV_LT_REVISION=0

AC_SUBST(DVDNAV_LT_CURRENT)
AC_SUBST(DVDNAV_LT_AGE)
AC_SUBST(DVDNAV_LT_REVISION)

AC_PROG_CC_C99

AC_SYS_LARGEFILE
AC_C_BIGENDIAN

AS_CASE([$host],
  [*mingw32* | *cygwin*], [AC_CHECK_FUNCS(gettimeofday)])

dnl ---------------------------------------------
dnl threads
dnl ---------------------------------------------
case $host in
  *-*-freebsd*)
    THREAD_LIBS="-L/usr/local/lib -pthread"
    THREAD_CFLAGS="-I/usr/local/include -D_THREAD_SAFE"
    CFLAGS="$THREAD_CFLAGS $CFLAGS"
    ;;
  *mingw32* | *cygwin*)
    ;;
  *android*)
    ;;
  *)
    AC_CHECK_LIB(pthread, pthread_create,
        [THREAD_LIBS="-lpthread"],
        AC_MSG_ERROR(pthread needed))
    ;;
esac
AC_SUBST(THREAD_LIBS)
AC_SUBST(THREAD_CFLAGS)

PKG_CHECK_MODULES([DVDREAD], [dvdread >= ${DVDREAD_MIN_VERSION}])

dnl ---------------------------------------------
dnl Check for bitfield compiler flag
dnl ---------------------------------------------
CC_CHECK_CFLAG_APPEND([-mno-ms-bitfields])

dnl check for -Wall -Wextra support
CC_CHECK_CFLAG_APPEND([-Wall -Wsign-compare -Wextra])

dnl ---------------------------------------------
dnl Check for doxygen (dynamic documentation generator)
dnl ---------------------------------------------
AC_CHECK_PROG(DOXYGEN, doxygen, doxygen, no)

dnl ---------------------------------------------
dnl -no-undefined libtool flag
dnl
dnl This should be after all checks
dnl ---------------------------------------------
AS_CASE([$host_os],
  [os2*], [LDFLAGS="$LDFLAGS -no-undefined"])

dnl ---------------------------------------------
dnl Output configuration files
dnl ---------------------------------------------
AC_OUTPUT([
Makefile
misc/dvdnav.pc
misc/libdvdnav.spec
src/dvdnav/version.h
])
