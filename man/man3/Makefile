SRC=	atalk_aton.3 nbp_name.3
SRCTMP= atalk_aton.3.tmp nbp_name.3.tmp

INCPATH=
CFLAGS=
TAGSFILE=
CC=
INSTALL=	install

LINKS=

all : ${SRCTMP}

${SRCTMP}: ../../Makefile
	for i in ${SRC} ; do \
	    sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
		< $$i > $$i.tmp; \
	done

install : ${SRCTMP}
	-mkdir ${MANDIR}/man3
	for i in ${SRC} ; do \
	    rm -f ${MANDIR}/man3/$$i; \
	    ${INSTALL} -m644 $$i.tmp ${MANDIR}/man3/$$i; \
	done

clean :
	for i in ${SRC}; do \
	    rm -f $$i.tmp; \
	done

tags : ${SRC}

depend :

# DO NOT DELETE THIS LINE

