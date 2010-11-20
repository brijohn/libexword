SUBDIRS = src                     
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libexword.pc

DISTCLEANFILES = src/Makefile.in \
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
