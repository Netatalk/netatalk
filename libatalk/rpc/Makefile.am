# Makefile.am for libatalk/rpc/

noinst_LTLIBRARIES = librpc.la

librpc_la_SOURCES = locking.c
nodist_librpc_la_SOURCES = lockrpc.gen.c
BUILT_SOURCES =  lockrpc.gen.c
EXTRADIST = lockrpc.gen.c lockrpc.rpc
lockrpc.gen.c: lockrpc.rpc $(top_srcdir)/libevent/event_rpcgen.py
	$(top_srcdir)/libevent/event_rpcgen.py lockrpc.rpc || echo "RPC build failure or no Python installed"
	cp  lockrpc.gen.h $(top_srcdir)/include/atalk