lib_LTLIBRARIES = libexword.la
bin_PROGRAMS = exword
libexword_la_SOURCES =	exword.c \
			exword.h \
			obex.c   \
			obex.h \
			databuffer.c \
			databuffer.h \
			list.h

include_HEADERS = exword.h

libexword_la_CFLAGS = \
        $(USB_CFLAGS)        \
        $(WARN_CFLAGS)          \
        $(AM_CFLAGS)

exword_SOURCES = main.c
exword_CFLAGS = \
        $(USB_CFLAGS)        \
        $(WARN_CFLAGS)          \
        $(AM_CFLAGS)

exword_LDFLAGS = $(AM_LDFLAGS)
exword_LDADD = $(USB_LIBS) libexword.la -lpopt
