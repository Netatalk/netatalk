TARGETS=	psf psa
SRC=	psf.c psa.c

INCPATH=	-I../../include
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH} -DZEROWIDTH
LIBS=	${ADDLIBS}
TAGSFILE=	tags
CC=	cc
INSTALL=	install
LIBDIRS=

LINKS=	ofpap ifpap tfpap ifpaprev tfpaprev \
	ofwpap ifwpap tfwpap ifwpaprev tfwpaprev \
	ofmpap ifmpap tfmpap ifmpaprev tfmpaprev \
	ofwmpap ifwmpap tfwmpap ifwmpaprev tfwmpaprev

all : ${TARGETS} ${LINKS}

${LINKS} :
	-ln -sf ${SBINDIR}/psf $@

psf : psf.o
	${CC} ${CFLAGS} -o psf psf.o ${LIBDIRS} ${LIBS}

psa : psa.o
	${CC} ${CFLAGS} -o psa psa.o ${LIBDIRS} ${LIBS}

psf.o : psf.c
	${CC} ${CFLAGS} -D_PATH_PAP=\"${BINDIR}/pap\" \
	    -D_PATH_PSORDER=\"${BINDIR}/psorder\" \
	    -D_PATH_PSA=\"${SBINDIR}/psa\" \
	    -D_PATH_PSFILTER=\"${SBINDIR}/etc2ps\" \
	    -D_PATH_PAGECOUNT=\"${RESDIR}/pagecount.ps\" \
	    ${CPPFLAGS} -c psf.c

install : all
	-mkdir -p ${RESDIR}/filters
	${INSTALL} -c psa ${SBINDIR}
	${INSTALL} -c etc2ps.sh ${SBINDIR}/etc2ps
	${INSTALL} -c pagecount.ps ${RESDIR}
	${INSTALL} -c psf ${SBINDIR}
	tar cBf - ${LINKS} | (cd ${RESDIR}/filters; tar xBf -)

clean :
	rm -f a.out core* *.o *.bak *[Ee]rrs tags
	rm -f ${TARGETS} ${LINKS}

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

