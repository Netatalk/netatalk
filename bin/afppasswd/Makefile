SRC = afppasswd.c
OBJ = afppasswd.o

INCPATH=	-I../../include
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH} ${CRYPTOINCPATH} ${CRYPTODEFS} \
	${CRACKINCPATH} ${CRACKDEFS}
LIBS=	${ADDLIBS} ${CRYPTOLIBS} ${CRACKLIBS}
TAGSFILE=	tags
CC=	cc
INSTALL=	install
LIBDIRS=	${CRYPTOLIBDIRS} ${CRACKLIBDIRS}

all::
	if [ x"${DESDIR}" != x ]; then \
	    CRYPTOLIBS="-ldes"; \
	    if [ "${DESDIR}" != "/usr" ]; then \
	      CRYPTOLIBDIRS="-L${DESDIR}/lib"; \
	      CRYPTOINCPATH="-I${DESDIR}/include"; \
	    fi; \
	    CRYPTODEFS="-DUAM_RNDNUM"; \
	fi; \
	if [ x"${CRYPTODIR}" != x ]; then \
	    CRYPTOLIBS="-lcrypto"; \
	    if [ "${CRYPTODIR}" != "/usr" ]; then \
	      CRYPTOLIBDIRS="-L${CRYPTODIR}/lib"; \
	      CRYPTOINCPATH="-I${CRYPTODIR}/include -I${CRYPTODIR}/include/openssl"; \
	    fi; \
	    CRYPTODEFS="-DUAM_RNDNUM"; \
	fi; \
	if [ x"${CRACKDIR}" != x ]; then \
	    CRACKLIBS="-lcrack"; \
	    if [ "${CRACKDIR}" != "/usr" ]; then \
	      CRACKLIBDIRS="-L${CRACKDIR}/lib"; \
	      CRACKINCPATH="-I${CRACKDIR}/include"; \
	    fi; \
	    CRACKDEFS="-DUSE_CRACKLIB -D_PATH_CRACKLIB=\\\"/usr/lib/cracklib_dict\\\""; \
	fi; \
	${MAKE} ${MFLAGS} CC="${CC}" ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" \
	    OPTOPTS="${OPTOPTS}" DESDIR="${DESDIR}" \
	    CRYPTOLIBS="$${CRYPTOLIBS}" CRYPTOLIBDIRS="$${CRYPTOLIBDIRS}" \
	    CRYPTLIB="$${CRYPTLIB}" \
	    CRYPTOINCPATH="$${CRYPTOINCPATH}" CRYPTODEFS="$${CRYPTODEFS}" \
	    CRACKLIBS="$${CRACKLIBS}" \
	    CRACKINCPATH="$${CRACKINCPATH}" CRACKDEFS="$${CRACKDEFS}" \
	    afppasswd

afppasswd.o: afppasswd.c
	${CC} ${CFLAGS} -D_PATH_AFPDPWFILE=\"${RESDIR}/afppasswd\" -c $<
afppasswd : ${OBJ} 
	${CC} ${CFLAGS} -o afppasswd ${OBJ} ${LIBDIRS} ${LIBS}

install : all
	${INSTALL} -c afppasswd ${BINDIR}

clean :
	rm -f a.out core* *.o *.bak *[Ee]rrs tags
	rm -f afppasswd

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

