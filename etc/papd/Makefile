SRC=    main.c printcap.c session.c file.c comment.c lp.c ppd.c \
	magics.c headers.c queries.c auth.c uam.c
OBJ=	main.o printcap.o session.o file.o comment.o lp.o ppd.o \
	magics.o headers.o queries.o auth.o uam.o

INCPATH = -I../../include ${KRBINCPATH} ${ABSINCPATH}
CFLAGS=	${DEFS} ${KRBDEFS} ${ABSDEFS} ${CAPDEFS} ${OPTOPTS} ${INCPATH}
TAGSFILE=	tags
LIBDIRS=	-L../../libatalk ${KRBLIBDIRS} ${ABSLIBDIRS} ${PAMLIBDIRS}
LIBS=	-latalk ${ABSLIBS} ${KRBLIBS} ${ADDLIBS} ${PAMLIBS} ${LIBSHARED}
CC=	cc
INSTALL=	install

all :
	if [ x"${KRBDIR}" != x ]; then \
	    KRBLIBS="-lkrb -ldes"; \
	    KRBLIBDIRS="-L${KRBDIR}/lib"; \
	    KRBINCPATH="-I${KRBDIR}/include"; \
	    KRBDEFS="-DKRB"; \
	fi; \
	if [ x"${PAMDIR}" != x ]; then \
	    PAMLIBS="-lpam"; \
	    if [ "${PAMDIR}" != "/usr" ]; then \
	      PAMLIBDIRS="-L${PAMDIR}/lib"; \
	      PAMINCPATH="-I${PAMDIR}/include"; \
	    fi; \
	    PAMDEFS="-DUSE_PAM"; \
	fi; \
	if [ x"${CAPDIR}" != x ]; then \
	     CAPDEFS="-DCAPDIR='\"${CAPDIR}\"'"; \
	fi; \
        ${MAKE} ${MFLAGS} CC="${CC}" ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" \
	    OPTOPTS="${OPTOPTS}" DESTDIR="${DESTDIR}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    PAMLIBS="$${PAMLIBS}" PAMLIBDIRS="$${PAMLIBDIRS}" \
	    PAMINCPATH="$${PAMINCPATH}" PAMDEFS="$${PAMDEFS}" \
	    KRBLIBS="$${KRBLIBS}" KRBLIBDIRS="$${KRBLIBDIRS}" \
	    KRBINCPATH="$${KRBINCPATH}" KRBDEFS="$${KRBDEFS}" \
	    CAPDEFS="$${CAPDEFS}" papd showppd

# UMICH stuff.  Don't ask, I'm not telling.
CRAP=/afs/umich.edu/group/itd/software/packages
ABSPATH=${CRAP}/a/abs1.5/sun4m_412
AFSPATH=${CRAP}/a/afs-3.3a/sun4m_412
papd.abs: FRC
	${MAKE} ${MFLAGS} \
	    KRBLIBS="-lkrb -ldes" \
	    KRBLIBDIRS="-L${KRBDIR}/lib" \
	    KRBINCPATH="-I${KRBDIR}/include" \
	    KRBDEFS="-DKRB" \
	    ABSLIBS="-labs -lsys -lrx -llwp -lrxkad -lrx \
		-lubik -lkauth -lcom_err -lauth -lrxkad \
		${AFSPATH}/lib/afs/util.a" \
	    ABSLIBDIRS="-L${ABSPATH}/lib -L${AFSPATH}/lib \
		-L${AFSPATH}/lib/afs" \
	    ABSINCPATH="-I${ABSPATH}/include -I${AFSPATH}/include" \
	    ABSDEFS="-DABS_PRINT" \
	    CC="gcc" SRC="${SRC} abs.c" OBJ="${OBJ} abs.o" \
	    papd

FRC:

showppd: showppd.c ppd.c
	${CC} ${CFLAGS} -DSHOWPPD -o showppd showppd.c ppd.c

papd : ${OBJ} ../../libatalk/libatalk.a
	${CC} ${CFLAGS} ${LDFLAGS_EXPORT} ${PAMINCPATH} ${PAMDEFS} \
		${LDFLAGS} -o papd ${OBJ} ${LIBDIRS} ${LIBS}

main.o : main.c
	${CC} ${CFLAGS} -D_PATH_PAPDCONF=\"${ETCDIR}/papd.conf\" \
	    -D_PATH_PAPDUAMPATH=\"${RESDIR}/uams/\" \
	    -DVERSION=\"`cat ../../VERSION`\" \
	    ${CPPFLAGS} -c main.c

queries.o : queries.c
	${CC} ${CFLAGS} ${PAMDEFS} -c queries.c

install : all
	${INSTALL} -c papd ${SBINDIR}
	${INSTALL} -c showppd ${BINDIR}

clean :
	rm -f a.out core* *.o *.bak *[Ee]rrs tags
	rm -f papd
	rm -f showppd

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
