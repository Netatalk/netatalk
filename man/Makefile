ALL=	man1 man3 man4 man8
TAGSFILE=tags
CC=cc
INSTALL=	install

all:	${ALL}

${ALL}: FRC
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" DEFS="${DEFS}" \
	    OPTOPTS="${OPTOPTS}" DESTDIR="${DESTDIR}" MANDIR="${MANDIR}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}"


FRC:

tags:
	for i in ${ALL}; do \
	    (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" \
		TAGSFILE=../${TAGSFILE} tags); \
	done

install:
	-mkdir ${MANDIR}
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} \
		DESTDIR="${DESTDIR}"  MANDIR="${MANDIR}" \
		SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		INSTALL="${INSTALL}" install); \
	done

clean:
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

depend:
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS=${DEFS} depend); \
	done
