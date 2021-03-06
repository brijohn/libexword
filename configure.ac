#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT(libexword, 0.1, brijohn@gmail.com)
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/exword.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
#AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_PATH_PROGS([DOX_PROG], [doxygen], missing)
AM_CONDITIONAL([DOX_ENABLED], [test "x$DOX_PROG" != "xmissing"])

case $host in
  *-*-mingw32*)
	EXTRA_LIBS="-lws2_32 -lshlwapi"
	EXTRA_LDFLAGS="-no-undefined"
    ;;
  *-*-darwin*)
	AM_LDFLAGS="-Wl,-framework,IOKit -Wl,-framework,CoreServices -Wl,-framework,CoreFoundation -Wl,-prebind -no-undefined"
    ;;
esac

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for libraries.
AC_CHECK_HEADER([readline/readline.h], [], [AC_MSG_ERROR([readline header not found])])
AC_CHECK_LIB(readline, readline, AC_SUBST([READLINE_LIBS], [-lreadline]), [AC_MSG_ERROR([readline support not available])])
AC_CHECK_FUNC(iconv_open, [], [AC_CHECK_LIB(iconv, libiconv_open, AC_SUBST([ICONV_LIBS], [-liconv]), [AC_MSG_ERROR([iconv support not available])])])

# Checks for typedefs, structures, and compiler characteristics.
LIBUSB_REQURED=1.0
PKG_CHECK_MODULES([USB],[libusb-1.0 >= $LIBUSB_REQURED])

AC_SUBST([EXTRA_LDFLAGS])
AC_SUBST([EXTRA_LIBS])

AC_SUBST([USB_CFLAGS])
AC_SUBST([USB_LIBS])

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])


AC_CONFIG_FILES([Makefile
		src/Makefile
		docs/Makefile
		libexword.pc])
AC_OUTPUT
