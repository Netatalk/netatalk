# Makefile.am for etc/spotlight/

pkgconfdir = @PKGCONFDIR@
noinst_LTLIBRARIES =
noinst_PROGRAMS =
noinst_HEADERS = sparql_map.h
BUILT_SOURCES =

AM_YFLAGS = -d

if HAVE_TRACKER
BUILT_SOURCES += sparql_parser.h
noinst_PROGRAMS += srp
noinst_LTLIBRARIES += libspotlight.la

libspotlight_la_SOURCES = \
	sparql_map.c \
	sparql_parser.y \
	spotlight_rawquery_lexer.l

libspotlight_la_CFLAGS  = \
	-DDBUS_API_SUBJECT_TO_CHANGE \
	@TRACKER_CFLAGS@ @TALLOC_CFLAGS@ \
	-D_PATH_STATEDIR='"$(localstatedir)/netatalk"'

libspotlight_la_LDFLAGS = @TRACKER_LIBS@ @TRACKER_MINER_LIBS@ @TALLOC_LIBS@

srp_SOURCES = \
	sparql_map.c \
	sparql_parser.y \
	spotlight_rawquery_lexer.l

srp_CFLAGS = -DMAIN -I$(top_srcdir)/include @TRACKER_CFLAGS@ @TALLOC_CFLAGS@
srp_LDADD = $(top_builddir)/libatalk/libatalk.la @MYSQL_LIBS@ @TALLOC_LIBS@
endif
