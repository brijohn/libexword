lib_LTLIBRARIES = libexword.la
bin_PROGRAMS = exword
libexword_la_SOURCES =	exword.c \
			exword.h \
			crypt.c \
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

libexword_la_LDFLAGS = -version-info $(LIBRARY_VERSION) $(EXTRA_LDFLAGS)
libexword_la_LIBADD = $(USB_LIBS) $(ICONV_LIBS) $(EXTRA_LIBS)

exword_SOURCES = main.c content.c util.c
exword_CFLAGS = \
        $(WARN_CFLAGS)          \
        $(AM_CFLAGS)

exword_LDFLAGS = $(AM_LDFLAGS)
exword_LDADD = $(READLINE_LIBS) libexword.la
