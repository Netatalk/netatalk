# Makefile.am for libatalk/vfs/

noinst_LTLIBRARIES = libvfs.la

libvfs_la_SOURCES = vfs.c unix.c ea.c sys_ea.c ea_sys.c

if HAVE_ACLS
libvfs_la_SOURCES += acl.c
endif
