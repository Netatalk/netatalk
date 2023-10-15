# Makefile.am for libatalk/cnid/

SUBDIRS = last dbd

noinst_LTLIBRARIES = libcnid.la
LIBCNID_DEPS = dbd/libcnid_dbd.la

if USE_LAST_BACKEND
LIBCNID_DEPS += last/libcnid_last.la
endif

if USE_MYSQL_BACKEND
SUBDIRS += mysql
LIBCNID_DEPS += @MYSQL_LIBS@ mysql/libcnid_mysql.la
endif

libcnid_la_SOURCES = cnid.c cnid_init.c
libcnid_la_LIBADD = $(LIBCNID_DEPS)

EXTRA_DIST = README

