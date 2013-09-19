AC_PREREQ([2.64])

# TODO do it automatically from wayland versions
# this should comply with wayland version
m4_define([wit_major_version], [1])
m4_define([wit_minor_version], [2])
m4_define([wit_micro_version], [90])
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

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

PKG_PROG_PKG_CONFIG()

if test "x$GCC" = "xyes"; then
	GCC_CFLAGS="-Wall -Wextra -g"
fi
AC_SUBST(GCC_CFLAGS)

#TODO add version
PKG_CHECK_MODULES([WAYLAND_SERVER], [wayland-server])
PKG_CHECK_MODULES([WAYLAND_CLIENT], [wayland-client])

AC_CHECK_HEADERS([wayland-server.h])
AC_CHECK_HEADERS([wayland-client.h])


AC_CONFIG_FILES([Makefile
		 src/Makefile
		 test/Makefile])

AC_OUTPUT
