if BUILD_SWIG

BUILT_SOURCES = exword_wrap.c
SWIG_SOURCES = exword.i

pkgpython_PYTHON = exword.py __init__.py
pkgpyexec_LTLIBRARIES = _exword.la
_exword_la_SOURCES =  exword_wrap.c $(SWIG_SOURCES)
_exword_la_CPPFLAGS = $(AX_SWIG_PYTHON_CPPFLAGS) -I$(top_srcdir)/src
_exword_la_LDFLAGS = -module -version-info $(LIBRARY_VERSION)
_exword_la_LIBADD = ../libexword.la
_exword_la_LIBTOOLFLAGS = --tag=disable-static

exword_wrap.c : $(SWIG_SOURCES)
	$(SWIG) $(AX_SWIG_PYTHON_OPT) -I$(top_srcdir)/src -o $@ $<

CLEANFILES = exword_wrap.c exword.py

endif
