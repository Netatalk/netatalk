# Makefile.am for etc/papd/

sbin_PROGRAMS = papd

papd_SOURCES = main.c printcap.c session.c file.c comment.c lp.c ppd.c magics.c headers.c queries.c auth.c uam.c
papd_LDADD = ../../libatalk/libatalk.a

noinst_HEADERS = comment.h file.h ppd.h printer.h uam_auth.h

CFLAGS = @CFLAGS@ -I../../include
