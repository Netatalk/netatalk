# FreeBSD specific defines, passed to subdirectories.
DEFS=	-DBSD4_4 $$OSDEFS
OPTOPTS=	-O2
CC=	gcc
CSHAREDFLAGS=-fPIC
LDSHARED=ld
LDSHAREDFLAGS=-shared
LDFLAGS_EXPORT=-export-dynamic
#LIBSHARED=-ldl
INSTALL=	install
AFPLIBS=
ADDLIBS=	

ALL=	../../libatalk ../../include ../../bin ../../etc ../../man

oops:
	@echo "Read README again.  Don't type 'make' here."
	@exit 1

all:	${ALL}

../../bin ../../etc:	../../libatalk

${ALL}:	FRC
	@case ${OSVERSION} in \
	1.*) ;; \
	2.*) ;; \
	*) OSDEFS=-DSENDFILE_FLAVOR_BSD ;; \
	esac; \
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
	@echo "in /etc/rc.  See README and README.FREEBSD for more"
	@echo "information."

clean :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

depend :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" depend); \
	done

# DO NOT DELETE THIS LINE

