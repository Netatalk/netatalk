# Makefile.am for top level of netatalk package

SUBDIRS = include libatalk bin config etc contrib distrib doc man macros test

EXTRA_DIST = CONTRIBUTORS COPYRIGHT COPYING Dockerfile NEWS VERSION

ACLOCAL_AMFLAGS = -I macros
AUTOMAKE_OPTIONS = foreign

if RUN_LDCONFIG
install-exec-hook:
	@printf "Running ldconfig to ensure libraries installed in system library directories are added to the dynamic linker cache ... "
	@-@NETA_LDCONFIG@
	@printf "done\n"
endif
