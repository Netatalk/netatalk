# Makefile.am for etc/papd/

sbin_PROGRAMS = papd

papd_SOURCES = comment.c file.c headers.c lp.c magics.c main.c ppd.c printcap.c queries.c session.c
papd_LDADD = ../../libatalk/libatalk.a

noinst_HEADERS = comment.h file.h ppd.h printer.h

CFLAGS = @CFLAGS@ -I../../include
