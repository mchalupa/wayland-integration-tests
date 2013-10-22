AC_PREREQ([2.64])

# versioning of WIT should follow wayland -
# e. g. WIT tests for wayland A.B.C should have version A.B.C or higer
m4_define([wit_major_version], [1])
m4_define([wit_minor_version], [2])
m4_define([wit_micro_version], [91])
m4_define([wit_version],
          [wit_major_version.wit_minor_version.wit_micro_version])

AC_INIT([wayland integration tests],
        [wit_version],
        [mchqwerty@gmail.com],
        [wit],
        [https://github.com/mchalupa/wayland-integration-tests/])

AC_SUBST([WIT_VERSION_MAJOR], [wit_major_version])
AC_SUBST([WIT_VERSION_MINOR], [wit_minor_version])
AC_SUBST([WIT_VERSION_MICRO], [wit_micro_version])
AC_SUBST([WIT_VERSION], [wit_version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

PKG_PROG_PKG_CONFIG()

TESTS_CFLAGS=
TESTS_LIBS=
COMPILER_CFLAGS=

# check wayland's presence
PKG_CHECK_MODULES([WAYLAND_SERVER], [wayland-server],\
			[HAVE_WAYLAND_SERVER=yes], [HAVE_WAYLAND_SERVER=no])
PKG_CHECK_MODULES([WAYLAND_CLIENT], [wayland-client],\
			[HAVE_WAYLAND_CLIENT=yes], [HAVE_WAYLAND_CLIENT=no])

if test "x$HAVE_WAYLAND_SERVER" != "xyes"; then
	AC_MSG_ERROR([Need wayland-server libraries to compile])
fi

if test "x$HAVE_WAYLAND_CLIENT" != "xyes"; then
	AC_MSG_ERROR([Need wayland-client libraries to compile])
fi

TESTS_CFLAGS="$WAYLAND_CLIENT_CFLAGS $WAYLAND_SERVER_CFLAGS"
TESTS_LIBS="$WAYLAND_CLIENT_LIBS $WAYLAND_SERVER_LIBS"

# check wayland's version
PKG_CHECK_MODULES([WAYLAND_VERSION], [wayland-server >= $WIT_VERSION],\
			[],\
			[AC_MSG_WARN([Wayland version is lower than $WIT_VERSION!])])

PKG_CHECK_MODULES([LIBUNWIND], [libunwind],\
			[HAVE_LIBUNWIND=yes], [HAVE_LIBUNWIND=no])

if test "x$HAVE_LIBUNWIND" = "xyes"; then
	AC_DEFINE(HAVE_LIBUNWIND, [1], [Allow use libunwind to printing backtrace])
	TESTS_CFLAGS="$LIBUNWIND_CFLAGS $TESTS_CFLAGS"
	TESTS_LIBS="$LIBUNWIND_LIBS $TESTS_LIBS"
fi

#export options to makefiles
AC_SUBST([TESTS_LIBS])
AC_SUBST([TESTS_CFLAGS])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 test/Makefile
		 test/test-runner/Makefile])

AC_OUTPUT
