# stuff for os x server. This needs some include files from the darwin
# source to work properly. namely, it needs /usr/include/at,
# /usr/include/h, and
# /System/Library/Frameworks/LibcAT/Headers/at_proto.h. The first two are
# found in the kernel sources while the last one is found in LibcAT.
#
# notes: apple's dynamic library loader is pretty braindead.
#        you cannot do a kill -HUP afpd as a result. in addition, you
#        can't load both uams_randnum.so and uams_dhx.so at the same time
#        as they use the same libraries. 
# 	 don't do tail /var/log/system.log a lot. it will cause os x 
#	 server to crash. it's probably a race somewhere.
#
DEFS=-DBSD4_4 -DHAVE_BROKEN_CPP -DHAVE_2ARG_DBTOB -DNO_DLFCN_H \
	-DMACOSX_SERVER 
OPTOPTS=	-O2 -fomit-frame-pointer
CC =   cc
#CC=	gcc
CSHAREDFLAGS=-fPIC -fno-common
LDSHARED=cc
LDSHAREDFLAGS=-bundle -undefined suppress
INSTALL=	install
ADDLIBS=-framework LibcAT

ALL=	../../libatalk ../../include ../../bin ../../etc ../../man

oops:
	@echo "Read README again.  Don't type 'make' here."
	@exit 1

all:	${ALL}

../../bin ../../etc:	../../libatalk

${ALL}:	FRC
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" \
	    ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    DESTDIR="${DESTDIR}" AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" \
	    AFPLIBS="${AFPLIBS}" LDSHARED="${LDSHARED}" \
	    LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	    LDSHAREDFLAGS="${LDSHAREDFLAGS}" CSHAREDFLAGS="${CSHAREDFLAGS}" \
	    LIBSHARED="${LIBSHARED}" \
	    all

FRC:

install :
	-mkdir ${DESTDIR} ${SBINDIR} ${BINDIR} ${ETCDIR} ${LIBDIR}
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} CC="${CC}" \
		ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	        SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" AFPLIBS="${AFPLIBS}" \
		LDSHARED="${LDSHARED}" LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	  	LDSHAREDFLAGS="${LDSHAREDFLAGS}" \
		CSHAREDFLAGS="${CSHAREDFLAGS}" LIBSHARED="${LIBSHARED}" \
		INSTALL="${INSTALL}" $@); \
	done
	rm -f ${ETCDIR}/rc.atalk
	sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
	    < ../../distrib/initscripts/rc.atalk.bsd > ${ETCDIR}/rc.atalk
	chmod 744 ${ETCDIR}/rc.atalk
	if [ -f ${ETCDIR}/afpd.conf ]; then \
		echo "Retaining old afpd.conf file.";  \
	else \
		sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
			-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
			-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
			-e s@:INCDIR:@${INCDIR}@ \
			< ../../config/afpd.conf > ${ETCDIR}/afpd.conf; \
	fi
	@echo
	@echo "Install is done.  Don't forget to add lines from"
	@echo "services.atalk to /etc/services and to call rc.atalk"
	@echo "in /etc/rc.  See README and README.GENERIC for more"
	@echo "information. Also, you do not need to start up atalkd."

clean :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

depend :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" depend); \
	done

# DO NOT DELETE THIS LINE

