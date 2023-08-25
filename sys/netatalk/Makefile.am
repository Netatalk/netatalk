# Makefile.am for sys/netatalk/

# Install at.h only on systems that don't use glibc
if USE_GLIBC_AT_HEADER
pkginclude_HEADERS = aarp.h at_var.h ddp.h ddp_var.h endian.h phase2.h
noinst_HEADERS = at.h
else
pkginclude_HEADERS = aarp.h at.h at_var.h ddp.h ddp_var.h endian.h phase2.h
endif

SOURCES = aarp.c at_control.c at_proto.c ddp_input.c ddp_output.c ddp_usrreq.c

EXTRA_DIST = $(SOURCES)
