# Makefile.am for bin/megatron/

bin_PROGRAMS = megatron

megatron_SOURCES = asingle.c hqx.c macbin.c megatron.c nad.c asingle.c updcrc.c
megatron_LDADD = ../../libatalk/libatalk.a

noinst_HEADERS = megatron.h

CFLAGS = @CFLAGS@ -I../../include -I../../sys

# ************************************************
# FIXME! FIXME! Links need to work for:
# LINKS = unbin unhex unsingle hqx2bin single2bin macbinary binheader nadheader
