# Makefile.am for include/atalk/

atalkincludedir = $(includedir)/atalk

atalkinclude_HEADERS = \
	adouble.h \
	afp.h \
	vfs.h \
	cnid.h \
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
	rtmp.h \
	server_child.h \
	server_ipc.h \
	tdb.h \
	uam.h \
	nbp.h \
	netddp.h \
	pap.h \
	cnid_dbd_private.h \
	cnid_private.h \
	bstradd.h \
	bstrlib.h \
	errchk.h \
	ftw.h \
	talloc.h \
	tevent.h \
	tsocket.h \
	aep.h \
	asp.h \
	atp.h \
	boolean.h \
	compat.h \
	ddp.h \
	dsi.h \
	ldapconfig.h \
	list.h

BUILT_SOURCES = lockrpc.gen.h
EXTRADIST = lockrpc.gen.h
lockrpc.gen.h: $(top_srcdir)/libevent/event_rpcgen.py $(top_srcdir)/libatalk/rpc/lockrpc.rpc
	cd $(top_srcdir)/libatalk/rpc/ \
		&& $(top_srcdir)/libevent/event_rpcgen.py lockrpc.rpc \
		&& mv lockrpc.gen.h $(top_srcdir)/include/atalk