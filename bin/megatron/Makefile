SRC=	hqx.c macbin.c megatron.c nad.c asingle.c updcrc.c
OBJ=	hqx.o macbin.o megatron.o nad.o asingle.o updcrc.o

INCPATH=	-I../../include
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH}
LIBS=	-latalk ${ADDLIBS}
TAGSFILE=	tags
CC=	cc
INSTALL=	install
LIBDIRS=	-L../../libatalk

LINKS=	unbin unhex unsingle hqx2bin single2bin macbinary binheader nadheader
TARGET=	megatron

all : ${TARGET}

${TARGET} : ${OBJ}
	${CC} ${CFLAGS} -o ${TARGET} ${OBJ} ${LIBDIRS} ${LIBS}

install : all
	${INSTALL} -c ${TARGET} ${BINDIR}
	for i in ${LINKS} ; do \
	    rm -f ${BINDIR}/$$i; \
	    ln -s ${TARGET} ${BINDIR}/$$i; \
	done

clean :
	rm -f a.out core* *.o *.bak *[Ee]rrs tags
	rm -f ${TARGET}

tags : ${SRC}
	cwd=`pwd`; \
	for i in ${SRC}; do \
	    ctags -t -a -f ${TAGSFILE} $$cwd/$$i; \
	done

depend :
	for i in ${SRC} ; do \
	    ${CC} -M ${DEFS} ${INCPATH} $$i | \
	    awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	sed -n '1,/^# DO NOT DELETE THIS LINE/p' Makefile > Makefile.tmp
	cat makedep >> Makefile.tmp
	rm makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile.tmp
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile.tmp
	echo '# see make depend above' >> Makefile.tmp
	rm -f Makefile.bak
	cp Makefile Makefile.bak
	mv Makefile.tmp Makefile

# DO NOT DELETE THIS LINE

