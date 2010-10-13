#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(libexword, 0.1, brijohn@gmail.com)
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/exword.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
#AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

AC_CHECK_LIB(readline, readline, [], [AC_MSG_ERROR([readline support not available])])
# Checks for typedefs, structures, and compiler characteristics.
LIBUSB_REQURED=1.0
PKG_CHECK_MODULES([USB],[libusb-1.0 >= $LIBUSB_REQURED])

AC_SUBST([USB_CFLAGS])
AC_SUBST([USB_LIBS])

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])


AC_CONFIG_FILES([Makefile
		src/Makefile])
AC_OUTPUT
