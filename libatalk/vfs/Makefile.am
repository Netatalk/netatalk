
# Makefile.am for libatalk/adouble/

noinst_LTLIBRARIES = libvfs.la

libvfs_la_SOURCES = vfs.c unix.c ea.c sys_ea.c ea_sys.c

if USE_NFSv4_ACLS
libvfs_la_SOURCES += acl.c
endif
