SRC=	afpd.8 atalkd.8 papd.8 psf.8
SRCTMP= afpd.8.tmp atalkd.8.tmp papd.8.tmp psf.8.tmp

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
	-mkdir ${MANDIR}/man8
	for i in ${SRC} ; do \
	    rm -f ${MANDIR}/man8/$$i; \
	    ${INSTALL} -m644 $$i.tmp ${MANDIR}/man8/$$i; \
	done

clean :
	for i in ${SRC}; do \
	    rm -f $$i.tmp; \
	done

tags : ${SRC}

depend :

# DO NOT DELETE THIS LINE

