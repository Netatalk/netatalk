
##########################################################################

SRC = unix.c ofork.c main.c switch.c auth.c volume.c directory.c file.c \
	enumerate.c desktop.c filedir.c fork.c appl.c gettok.c \
	status.c afp_options.c afp_asp.c afp_dsi.c messages.c afp_config.c \
	nfsquota.c codepage.c quota.c uam.c afs.c

OBJ = unix.o ofork.o main.o switch.o auth.o volume.o directory.o file.o \
	enumerate.o desktop.o filedir.o fork.o appl.o gettok.o \
	status.o afp_options.o afp_asp.o afp_dsi.o messages.o afp_config.o \
	nfsquota.o codepage.o quota.o uam.o afs.o

INCPATH=	-I../../include ${AFSINCPATH}
CFLAGS=	${DEFS} ${AFSDEFS} ${CAPDEFS} ${OPTOPTS} ${INCPATH} -DAPPLCNAME
LIBS =	-latalk ${AFSLIBS} ${ADDLIBS} ${TCPWRAPLIBS} ${DB2LIBS} \
	${RPCSVCLIB} ${AFPLIBS} ${PAMLIBS} ${LIBSHARED} 
LIBDIRS=	-L../../libatalk ${AFSLIBDIRS} ${TCPWRAPLIBDIRS} \
	${DB2LIBDIRS} ${PAMLIBDIRS}
TAGSFILE=	tags
CC=	cc
INSTALL=	install

SUBDIRS = nls

all : ${SUBDIRS}
	if [ x"${TCPWRAPDIR}" != x ]; then \
	    TCPWRAPLIBS="-lwrap -lnsl"; \
	    if [ "${TCPWRAPDIR}" != "/usr" ]; then \
	      TCPWRAPLIBDIRS="-L${TCPWRAPDIR}/lib"; \
	    fi; \
	fi; \
	if [ x"${DB2DIR}" != x ]; then \
	    DB2LIBS="-ldb"; \
	    if [ "${DB2DIR}" != "/usr" ]; then \
	      DB2LIBDIRS="-L${DB2DIR}/lib"; \
	      DB2INCPATH="-I${DB2DIR}/include"; \
	    fi; \
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
	    CAPDEFS="-DCAPDIR='\"${CAPDIR}\"'";\
	fi; \
	if [ -f /usr/lib/librpcsvc.a -o -f /lib/librpcsvc.a ]; then \
	    RPCSVCLIB=-lrpcsvc; \
	fi; \
	${MAKE} ${MFLAGS} CC="${CC}" ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" \
	    OPTOPTS="${OPTOPTS}" DESTDIR="${DESTDIR}" DESDIR="${DESDIR}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    TCPWRAPDIR="${TCPWRAPDIR}" DB2DIR="${DB2DIR}" \
	    AFSLIBS="$${AFSLIBS}" AFSLIBDIRS="$${AFSLIBDIRS}" \
	    TCPWRAPLIBS="$${TCPWRAPLIBS}" TCPWRAPLIBDIRS="$${TCPWRAPLIBDIRS}" \
	    DB2LIBS="$${DB2LIBS}" DB2LIBDIRS="$${DB2LIBDIRS}" \
	    LIBSHARED="$${LIBSHARED}" PAMLIBS="$${PAMLIBS}" \
	    PAMLIBDIR="$${PAMLIBDIR}" RPCSVCLIB="$${RPCSVCLIB}" \
	    AFSINCPATH="$${AFSINCPATH}" AFSDEFS="$${AFSDEFS}" \
	    CAPDEFS="$${CAPDEFS}" DB2INCPATH="$${DB2INCPATH}" \
	    afpd

${SUBDIRS}: FRC
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" DEFS="${DEFS}" \
	    OPTOPTS="${OPTOPTS}" DESTDIR="${DESTDIR}" DESDIR="${DESDIR}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    TCPWRAPDIR="${TCPWRAPDIR}" \
	    AFSLIBS="$${AFSLIBS}" AFSLIBDIRS="$${AFSLIBDIRS}" \
	    AFSINCPATH="$${AFSINCPATH}" AFSDEFS="$${AFSDEFS}" 

FRC:

afpd : ${OBJ} ../../libatalk/libatalk.a
	${CC} ${CFLAGS} ${LDFLAGS_EXPORT} -o afpd ${OBJ} ${LIBDIRS} ${LIBS}

afp_options.o : afp_options.c
	${CC} ${CFLAGS} \
	    -D_PATH_AFPDDEFVOL=\"${ETCDIR}/AppleVolumes.default\" \
	    -D_PATH_AFPDSYSVOL=\"${ETCDIR}/AppleVolumes.system\" \
	    -D_PATH_AFPDPWFILE=\"~/.passwd\" \
	    -D_PATH_AFPDCONF=\"${ETCDIR}/afpd.conf\" \
	    -D_PATH_AFPDUAMPATH=\"${RESDIR}/uams/\" \
	    -D_PATH_AFPDNLSPATH=\"${RESDIR}/nls/\" \
	    ${CPPFLAGS} -c $<

afp_config.o: afp_config.c
	${CC} ${CFLAGS} \
	    -DVERSION=\"`cat ../../VERSION`\" \
	    ${CPPFLAGS} -c $< 

install : all
	${INSTALL} -c afpd ${SBINDIR}
	for i in ${SUBDIRS}; \
	do (cd $$i; ${MAKE} ${MFLAGS} CC="${CC}" \
		ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	        SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		DESTDIR="${DESTDIR}" INSTALL="${INSTALL}" \
		install); \
	done

clean :
	for i in ${SUBDIRS}; \
	do (cd $$i; ${MAKE} ${MFLAGS} CC="${CC}" \
		ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	        SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		DESTDIR="${DESTDIR}" INSTALL="${INSTALL}" \
		clean); \
	done
	rm -f a.out core* *.o *.bak *[Ee]rrs tags
	rm -f afpd

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

