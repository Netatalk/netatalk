# Makefile.am for include/atalk/

atalkincludedir = $(includedir)/atalk

atalkinclude_HEADERS = \
	adouble.h \
	afp.h \
	vfs.h \
	cnid.h \
	locking.h \
	logger.h \
	paths.h \
	unicode.h \
	util.h \
	volinfo.h \
	ea.h \
	acl.h \
	unix.h \
	volume.h

noinst_HEADERS = \
	directory.h \
	hash.h \
	zip.h \
	uuid.h \
	queue.h \
	server_child.h \
	server_ipc.h \
	tdb.h \
	uam.h \
	cnid_dbd_private.h \
	cnid_private.h \
	bstradd.h \
	bstrlib.h \
	errchk.h \
	ftw.h \
	talloc.h \
	tevent.h \
	tsocket.h \
	boolean.h \
	compat.h \
	dsi.h \
	ldapconfig.h \
	list.h \
	globals.h \
	fce_api.h \
	standards.h

BUILT_SOURCES = lockrpc.gen.h
EXTRADIST = lockrpc.gen.h
lockrpc.gen.h: $(top_srcdir)/libevent/event_rpcgen.py $(top_srcdir)/libatalk/rpc/lockrpc.rpc
	cd $(top_srcdir)/libatalk/rpc/ \
		&& $(top_srcdir)/libevent/event_rpcgen.py lockrpc.rpc \
		&& mv lockrpc.gen.h $(top_srcdir)/include/atalk
