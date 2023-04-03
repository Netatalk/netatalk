# Makefile.am for libatalk/talloc/

noinst_LTLIBRARIES = libtalloc.la
if USE_BUILTIN_TALLOC
libtalloc_la_SOURCES = talloc.c dalloc.c
else
libtalloc_la_SOURCES = dalloc.c
endif

libtalloc_la_CFLAGS = @TALLOC_CFLAGS@
libtalloc_la_LIBADD = @TALLOC_LIBS@
