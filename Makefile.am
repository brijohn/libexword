SUBDIRS = src docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libexword.pc

extradir = $(datadir)/exword
extra_DATA = models.txt

DISTCLEANFILES = src/Makefile.in \
                 docs/Makefile.in \
                 Makefile.in \
                 aclocal.m4 \
                 configure \
                 compile \
                 INSTALL \
                 install-sh \
                 depcomp \
                 COPYING \
                 missing \
                 config.guess \
                 config.sub \
                 ltmain.sh

distclean-local:
	-rm -rf autom4te.cache
