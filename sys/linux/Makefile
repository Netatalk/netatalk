# Linux specific defines, passed to subdirectories.
#DEFS= -DTRY_64BITOFF_T -DNO_STRUCT_TM_GMTOFF -DHAVE_IFNAMEINDEX 
DEFS=$$OSDEFS $$MACHINEDEFS $$QUOTADEF
OPTOPTS=-O2 -fomit-frame-pointer -fsigned-char -Wunused -Wuninitialized 
#OPTOPTS=	-g -fsigned-char
CC=	gcc 
CSHAREDFLAGS=-fPIC
LDSHARED=ld
LDSHAREDFLAGS=-shared
LDFLAGS_EXPORT=-rdynamic
LIBSHARED=-ldl

AFPLIBS=
ADDLIBS=

INSTALL=	install 

ALL=	../../libatalk ../../include ../../bin ../../etc ../../man

oops:
	@echo "Read README again.  Don't type 'make' here."
	@exit 1

all:	${ALL}
	sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
	< ../../distrib/initscripts/rc.atalk.redhat > atalk
	sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
		< ../../config/afpd.conf > afpd.conf

../../bin ../../etc:	../../libatalk

${ALL}:	FRC
	@case ${OSVERSION} in \
	1.0*) ;; \
	2.0*) ;; \
	*) OSDEFS=-DSENDFILE_FLAVOR_LINUX ;; \
	esac; \
	if [ x"${MACHINETYPE}" = x"alpha" ]; then \
	   MACHINEDEFS=-DHAVE_GCC_MEMCPY_BUG;  \
	fi; \
	if [ ! -f /usr/include/sys/quota.h ]; then \
	   QUOTADEF=-DNEED_QUOTACTL_WRAPPER; \
	fi; \
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" \
	    ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    DESTDIR="${DESTDIR}" AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" \
	    CAPDIR="${CAPDIR}" \
	    AFPLIBS="${AFPLIBS}" LDSHARED="${LDSHARED}" \
	    LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	    LDSHAREDFLAGS="${LDSHAREDFLAGS}" CSHAREDFLAGS="${CSHAREDFLAGS}" \
	    LIBSHARED="${LIBSHARED}" \
	    all

FRC:

${ETCDIR}:
	-mkdir -p ${ETCDIR}

install-sysv:
	if [ -d ${INSTALL_PREFIX}/etc/rc.d/init.d ]; then \
	  ${INSTALL} -m744 atalk ${INSTALL_PREFIX}/etc/rc.d/init.d/atalk; \
	  rm -f ${INSTALL_PREFIX}/etc/rc.d/init.d/atalk.init \
	        ${INSTALL_PREFIX}/etc/rc.d/rc?.d/[SK]??atalk; \
	fi; \
	if [ x"${INSTALL_PREFIX}" = x ]; then \
		/sbin/chkconfig --add atalk; \
	fi; \
	if [ -f ${ETCDIR}/netatalk.conf ]; then \
		echo "Retaining old netatalk.conf file."; \
	else \
		${INSTALL} -m644 ../../config/netatalk.conf ${ETCDIR}; \
	fi

install-bsd:
	-if [ -f ${ETCDIR}/rc.atalk ]; then \
		echo "Retaining old rc.atalk file."; \
	else if [ ! -d /etc/rc.d/init.d ]; then \
	sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
		< ../../distrib/initscripts/rc.atalk.bsd > ${ETCDIR}/rc.atalk; \
	fi \
	fi

install : ${ETCDIR} install-sysv install-bsd
	-mkdir ${DESTDIR} ${SBINDIR} ${BINDIR} ${LIBDIR}
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
	if [ -d ${INSTALL_PREFIX}/etc/pam.d -a \
	     ! -f ${INSTALL_PREFIX}/etc/pam.d/netatalk ]; then \
	   	${INSTALL} -m644 ../../config/netatalk.pamd \
	                         ${INSTALL_PREFIX}/etc/pam.d/netatalk; \
		echo "PAM netatalk file installed."; \
	fi 
	if [ -f ${ETCDIR}/afpd.conf ]; then \
		echo "Retaining old afpd.conf file.";  \
	else \
		${INSTALL} -m644 afpd.conf ${ETCDIR}/afpd.conf; \
	fi
	@echo
	@echo "Install is done.  Don't forget to add lines from"
	@echo "services.atalk to /etc/services."
	if [ ! -d /etc/rc.d/init.d ]; then \
		echo "Don't forget to call rc.atalk in /etc/rc."; \
	fi
	@echo "See README and README.LINUX for more information."


clean :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done
	-rm -f atalk afpd.conf

depend :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" depend); \
	done

# DO NOT DELETE THIS LINE

