# NetBSD specific defines, passed to subdirectories.
DEFS=	-DBSD4_4 -DDLSYM_PREPEND_UNDERSCORE
OPTOPTS=	-O2
CC=	gcc -I../../sys/netbsd/ -I/usr/include/kerberosIV
CSHAREDFLAGS=-fPIC
LDSHARED=ld
LDSHAREDFLAGS=$$MACHLDSHAREDFLAGS
#LDFLAGS_EXPORT=-Wl,-E
#LIBSHARED=-ldl
INSTALL=	install
AFPLIBS =
ADDLIBS =

ALL=	../../libatalk ../../include ../../bin ../../etc ../../man

oops:
	@echo "Read README again.  Don't type 'make' here."
	@exit 1

all:	${ALL}

../../bin ../../etc:	../../libatalk

${ALL}:	FRC
	@case ${MACHINETYPE} in \
	alpha|mips|powerpc) MACHLDSHAREDFLAGS=-shared ;; \
	*) MACHLDSHAREDFLAGS=-Bshareable ;; \
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
	@echo "Install is done.  You might need to add services from"
	@echo "services.atalk to /etc/services. The ddp services need to be"
	@echo "added for NetBSD versions earlier than 1.3, and the AFP over"
	@echo "TCP services need to be added for NetBSD 1.3.X, and for"
	@echo "1.3-current dated June 27, 1998 and earlier."
	@echo
	@echo "Don't forget to call rc.atalk in /etc/rc.local.  See README"
	@echo "and README.NETBSD for more information."

clean :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

depend :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" depend); \
	done

# DO NOT DELETE THIS LINE

