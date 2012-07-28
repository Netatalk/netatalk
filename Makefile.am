# Makefile.am for top level of netatalk package

SUBDIRS = libatalk bin config etc man contrib distrib include doc macros test

EXTRA_DIST = CONTRIBUTORS COPYRIGHT COPYING NEWS VERSION

ACLOCAL_AMFLAGS = -I macros
AUTOMAKE_OPTIONS = foreign

if USE_BUILTIN_LIBEVENT
	SUBDIRS += libevent
endif

if RUN_LDCONFIG
install-exec-hook:
	@printf "Running ldconfig to ensure libraries installed in system library directories are added to the dynamic linker cache ... "
	@-@NETA_LDCONFIG@
	@printf "done\n"
endif
