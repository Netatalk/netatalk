
##########################################################################

SRC = uams_pam.c uams_passwd.c uams_randnum.c uams_guest.c uams_dhx_pam.c \
      uams_dhx_passwd.c
OBJ = uams_pam.o uams_passwd.o uams_randnum.o uams_guest.o uams_dhx_pam.o \
      uams_dhx_passwd.o
SHAREDOBJS = uams_pam.so uams_passwd.so uams_randnum.so uams_guest.so \
	     uams_dhx_pam.so uams_dhx_passwd.so

CFLAGS=	-I../../include ${CSHAREDFLAGS} ${DEFS} ${AFSDEFS} ${OPTOPTS} 
TAGSFILE=	tags
CC=	cc
INSTALL=	install

SUBDIRS=

all:: 
	if [ x"${KRBDIR}" != x ]; then \
	    KRBLIBS="-lkrb -ldes"; \
	    KRBLIBDIRS="-L${KRBDIR}/lib"; \
	    KRBINCPATH="-I${KRBDIR}/include"; \
	    KRBDEFS="-DKRB"; \
	fi; \
	if [ x"${AFSDIR}" != x ]; then \
	    AFSLIBS="-lkauth -lprot -lubik -lauth -lsys -lrxkad -lrx -laudit \
		-llwp -lcmd -lcom_err ${AFSDIR}/lib/afs/util.a -ldes"; \
	    AFSLIBDIRS="-L${AFSDIR}/lib -L${AFSDIR}/lib/afs"; \
	    AFSINCPATH="-I${AFSDIR}/include"; \
	    AFSDEFS="-DAFS"; \
	fi; \
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
	    CRYPTODEFS="-DUAM_RNDNUM -DUAM_DHX"; \
	fi; \
	if [ x"${PAMDIR}" != x ]; then \
	    PAMLIBS="-lpam"; \
	    if [ "${PAMDIR}" != "/usr" ]; then \
	      PAMLIBDIRS="-L${PAMDIR}/lib"; \
	      PAMINCPATH="-I${PAMDIR}/include"; \
	    fi; \
	    PAMDEFS="-DUSE_PAM"; \
	fi; \
	if [ x"${CRACKDIR}" != x ]; then \
	    CRACKLIBS="-lcrack"; \
	    if [ "${CRACKDIR}" != "/usr" ]; then \
	      CRACKLIBDIRS="-L${CRACKDIR}/lib"; \
	      CRACKINCPATH="-I${CRACKDIR}/include"; \
	    fi; \
	    CRACKDEFS="-DUSE_CRACKLIB -D_PATH_CRACKLIB=\\\"/usr/lib/cracklib_dict\\\""; \
	fi; \
	if [ -f /lib/libcrypt.a -o -f /usr/lib/libcrypt.a ] ; then \
	   if [ x"${USE_CRYPTLIB}" != x"no" ]; then \
	    CRYPTLIB="-lcrypt"; \
	   fi; \
        fi; \
	if [ ! -f /usr/include/crypt.h -a ! -f /usr/local/include/crypt.h ]; then \
	    CRYPTDEFS="-DNO_CRYPT_H"; \
	fi; \
	${MAKE} ${MFLAGS} CC="${CC}" ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" \
	    OPTOPTS="${OPTOPTS}" DESDIR="${DESDIR}" \
	    PAMDIR="${PAMDIR}" \
	    KRBLIBS="$${KRBLIBS}" KRBLIBDIRS="$${KRBLIBDIRS}" \
	    KRBINCPATH="$${KRBINCPATH}" KRBDEFS="$${KRBDEFS}" \
	    AFSLIBS="$${AFSLIBS}" AFSLIBDIRS="$${AFSLIBDIRS}" \
	    CRYPTOLIBS="$${CRYPTOLIBS}" CRYPTOLIBDIRS="$${CRYPTOLIBDIRS}" \
	    PAMLIBS="$${PAMLIBS}" PAMLIBDIRS="$${PAMLIBDIRS}" \
	    CRYPTLIB="$${CRYPTLIB}" CRYPTDEFS="$${CRYPTDEFS}" \
	    CRYPTOINCPATH="$${CRYPTOINCPATH}" CRYPTODEFS="$${CRYPTODEFS}" \
	    PAMINCPATH="$${PAMINCPATH}" PAMDEFS="$${PAMDEFS}" \
	    AFSINCPATH="$${AFSINCPATH}" AFSDEFS="$${AFSDEFS}" \
	    CRACKLIBS="$${CRACKLIBS}" \
	    CRACKINCPATH="$${CRACKINCPATH}" CRACKDEFS="$${CRACKDEFS}" \
	    ${SHAREDOBJS} ${SUBDIRS}

.SUFFIXES: .o .so

.c.so:
	${CC} ${CFLAGS} -c $<
	${LDSHARED} ${LDSHAREDFLAGS} -o $@ $*.o 

uams_passwd.so: uams_passwd.c
	${CC} ${CFLAGS} ${CRYPTDEFS} -c $<
	${LDSHARED} ${LDSHAREDFLAGS} -o $@ $*.o ${CRYPTLIB}

uams_pam.so: uams_pam.c
	${CC} ${CFLAGS} ${PAMINCPATH} ${PAMDEFS} -c $<
	${LDSHARED} ${LDSHAREDFLAGS} -o $@ $*.o ${PAMLIBDIRS} ${PAMLIBS}

uams_randnum.so: uams_randnum.c
	${CC} ${CFLAGS} ${CRYPTOINCPATH} ${CRYPTODEFS} ${CRACKINCPATH} \
	${CRACKDEFS} -c $<
	${LDSHARED} ${LDSHAREDFLAGS} -o $@ $*.o ${CRYPTOLIBDIRS} \
	${CRYPTOLIBS} ${CRACKLIBS}

uams_dhx_passwd.so: uams_dhx_passwd.c
	${CC} ${CFLAGS} ${CRYPTDEFS} ${CRYPTOINCPATH} ${CRYPTODEFS} -c $<
	${LDSHARED} ${LDSHAREDFLAGS} -o $@ $*.o ${CRYPTOLIBDIRS} ${CRYPTOLIBS}

uams_dhx_pam.so: uams_dhx_pam.c
	${CC} ${CFLAGS} ${PAMINCPATH} ${CRYPTOINCPATH} ${CRYPTODEFS} \
	${PAMDEFS} -c $<
	${LDSHARED} ${LDSHAREDFLAGS} -o $@ $*.o ${CRYPTOLIBDIRS} \
	${CRYPTOLIBS} ${PAMLIBDIRS} ${PAMLIBS}

# this is getting pushed into a subdirectory
uams_kerberos.so: uams_kerberos.c	
	${CC} ${CFLAGS} ${KRBINCPATH} ${KRBDEFS} -c $<
	${LDSHARED} ${LDSHAREDFLAGS} -o $@ $< ${KRBLIBDIRS} ${KRBLIBS} 

install : all
	-for i in ${SUBDIRS}; \
	do (cd $$i; ${MAKE} ${MFLAGS} CC="${CC}" \
		ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	        SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		DESTDIR="${DESTDIR}" INSTALL="${INSTALL}" \
		install); \
	done
	-mkdir ${RESDIR}/uams
	${INSTALL} -c *.so ${RESDIR}/uams
	-rm ${RESDIR}/uams/uams_clrtxt.so ${RESDIR}/uams/uams_dhx.so
	if [ x"${PAMDIR}" != x ]; then \
		(cd ${RESDIR}/uams; ln -s uams_pam.so uams_clrtxt.so; \
		 ln -s uams_dhx_pam.so uams_dhx.so); \
	else \
		(cd ${RESDIR}/uams; ln -s uams_passwd.so uams_clrtxt.so; \
		 ln -s uams_dhx_passwd.so uams_dhx.so); \
	fi;

clean :
	rm -f a.out core* *.o *.bak *[Ee]rrs tags
	rm -f *.so

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

