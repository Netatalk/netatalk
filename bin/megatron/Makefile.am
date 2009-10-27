# Makefile.am for bin/megatron/

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/sys

bin_PROGRAMS = megatron

megatron_SOURCES = asingle.c hqx.c macbin.c megatron.c nad.c updcrc.c
megatron_LDADD = $(top_builddir)/libatalk/libatalk.la

noinst_HEADERS = asingle.h megatron.h hqx.h macbin.h nad.h updcrc.h

LINKS = unbin unhex unsingle hqx2bin single2bin macbinary binheader nadheader

install-exec-hook:
	@for LINK in $(LINKS); do \
		rm -f $(DESTDIR)$(bindir)/$$LINK; \
		$(LN_S) megatron $(DESTDIR)$(bindir)/$$LINK; \
	done

uninstall-hook:
	@for LINK in $(LINKS); do \
		rm -f $(DESTDIR)$(bindir)/$$LINK; \
	done
