SRC = aecho.c
OBJ = aecho.o

INCPATH=	-I../../include
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH}
LIBS=	-latalk ${ADDLIBS}
TAGSFILE=	tags
CC=	cc
INSTALL=	install
LIBDIRS=	-L../../libatalk

all : aecho

aecho : ${OBJ} ../../libatalk/libatalk.a
	${CC} ${CFLAGS} -o aecho ${OBJ} ${LIBDIRS} ${LIBS}

install : all
	${INSTALL} -c aecho ${BINDIR}

clean :
	rm -f a.out core* *.o *.bak *[Ee]rrs tags
	rm -f aecho

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

