ALL=	afpd uams psf atalkd papd 
TAGSFILE=tags
CC=cc
INSTALL=	install

all:	${ALL}

${ALL}: FRC
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" \
            ADMIN_GRP="${ADMIN_GRP}" \
	    ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    DESTDIR="${DESTDIR}" AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" \
	    DESDIR="${DESDIR}" TCPWRAPDIR="${TCPWRAPDIR}" PAMDIR="${PAMDIR}" \
	    CRYPTODIR="${CRYPTODIR}" DB2DIR="${DB2DIR}" \
	    CAPDIR="${CAPDIR}" all

FRC:

tags:
	for i in ${ALL}; do \
	    (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" \
		TAGSFILE=../${TAGSFILE} tags); \
	done

install:
	-mkdir ${ETCDIR}
	-mkdir ${SBINDIR}
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} CC="${CC}" \
		ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	        SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		DESTDIR="${DESTDIR}" AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" \
		TCPWRAPDIR="${TCPWRAPDIR}" PAMDIR="${PAMDIR}" \
		CAPDIR="${CAPDIR}" \
		INSTALL="${INSTALL}" DESDIR="${DESDIR}" install); \
	done

clean:
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

depend:
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS=${DEFS} depend); \
	done
