SRC=	aecho.1 getzones.1 nbp.1 nbplkup.1 nbprgstr.1 \
	pap.1 papstatus.1 psorder.1 megatron.1 unhex.1 unbin.1 \
	unsingle.1 macbinary.1 hqx2bin.1 single2bin.1
SRCTMP= aecho.1.tmp getzones.1.tmp nbp.1.tmp nbplkup.1.tmp nbprgstr.1.tmp \
	pap.1.tmp papstatus.1.tmp psorder.1.tmp megatron.1.tmp \
	unhex.1.tmp unbin.1.tmp unsingle.1.tmp macbinary.1.tmp \
	hqx2bin.1.tmp single2bin.1.tmp

INCPATH=
CFLAGS=
TAGSFILE=
CC=
INSTALL=	install

LINKS=


all: ${SRCTMP}

${SRCTMP}: ../../Makefile
	for i in ${SRC} ; do \
		sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
		< $$i > $$i.tmp; \
	done

install : ${SRCTMP}
	-mkdir ${MANDIR}/man1
	for i in ${SRC} ; do \
	    rm -f ${MANDIR}/man1/$$i; \
	    ${INSTALL} -m644 $$i.tmp ${MANDIR}/man1/$$i; \
	done

clean : 
	for i in ${SRC}; do \
	    rm -f $$i.tmp; \
	done

tags : ${SRC}

depend :

# DO NOT DELETE THIS LINE

