# Makefile.am for bin/megatron/

bin_PROGRAMS = megatron

megatron_SOURCES = asingle.c hqx.c macbin.c megatron.c nad.c updcrc.c
megatron_LDADD = $(top_builddir)/libatalk/libatalk.la

noinst_HEADERS = megatron.h

LINKS = unbin unhex unsingle hqx2bin single2bin macbinary binheader nadheader

install-exec-hook:
	for LINK in $(LINKS); do
		ln -s $(bindir)/megatron $(bindir)/$(LINK)
	done
