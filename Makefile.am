ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src src/swig docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libexword.pc

extradir = $(datadir)/exword
extra_DATA = models.txt

DISTCLEANFILES = src/Makefile.in \
                 src/swig/Makefile.in \
                 src/swig/exword.py \
                 src/swig/exword.pyc \
                 src/swig/exword_wrap.c \
                 docs/Makefile.in \
                 m4/libtool.m4 \
                 m4/lt~obsolete.m4 \
                 m4/ltoptions.m4 \
                 m4/ltsugar.m4 \
                 m4/ltversion.m4 \
                 Makefile.in \
                 aclocal.m4 \
                 configure \
                 compile \
                 INSTALL \
                 install-sh \
                 depcomp \
                 COPYING \
                 missing \
                 ar-lib \
                 py-compile \
                 config.h.in \
                 config.guess \
                 config.sub \
                 ltmain.sh

distclean-local:
	-rm -rf autom4te.cache
