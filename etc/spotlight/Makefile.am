# Makefile.am for etc/spotlight/

pkgconfdir = @PKGCONFDIR@
moduledir = @UAMS_PATH@
module_LTLIBRARIES =
noinst_PROGRAMS =
noinst_HEADERS = slmod_sparql_map.h slmod_tracker_0_6_map.h
BUILT_SOURCES =

AM_YFLAGS = -d

if HAVE_TRACKER_SPARQL
BUILT_SOURCES += slmod_sparql_parser.h
noinst_PROGRAMS += srp
module_LTLIBRARIES += slmod_sparql.la

slmod_sparql_la_SOURCES = \
	slmod_sparql.c \
	slmod_sparql_map.c \
	slmod_sparql_parser.y \
	spotlight_rawquery_lexer.l

slmod_sparql_la_CFLAGS  = -DDBUS_API_SUBJECT_TO_CHANGE @TRACKER_CFLAGS@ @TRACKER_MINER_CFLAGS@
slmod_sparql_la_LDFLAGS = -module -avoid-version @TRACKER_LIBS@ @TRACKER_MINER_LIBS@

srp_SOURCES = \
	slmod_sparql_map.c \
	slmod_sparql_parser.y \
	spotlight_rawquery_lexer.l

srp_CFLAGS = -DMAIN -I$(top_srcdir)/include @TRACKER_CFLAGS@
srp_LDADD = $(top_builddir)/libatalk/libatalk.la
endif

if HAVE_TRACKER_0_6
BUILT_SOURCES += slmod_tracker_0_6_parser.h
noinst_PROGRAMS += stp
module_LTLIBRARIES += slmod_tracker_0_6.la

slmod_tracker_0_6_la_SOURCES = \
	slmod_tracker_0_6.c \
	slmod_tracker_0_6_map.c \
	slmod_tracker_0_6_parser.y \
	spotlight_rawquery_lexer.l

slmod_tracker_0_6_la_CFLAGS  = @TRACKER_CFLAGS@
slmod_tracker_0_6_la_LDFLAGS = -module -avoid-version @TRACKER_LIBS@

stp_SOURCES = \
	slmod_tracker_0_6_map.c \
	slmod_tracker_0_6_parser.y \
	spotlight_rawquery_lexer.l
stp_CFLAGS = -DMAIN @TRACKER_CFLAGS@
stp_LDADD = $(top_builddir)/libatalk/libatalk.la @TRACKER_LIBS@

endif