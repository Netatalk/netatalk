# Solaris specific defines, passed to subdirectories.
# To use Sun CC, uncomment the CC and KFLAGS variables.

# uncomment for 64-bit sparc kernel
#SPARC64=true

# solaris has -lcrypt, but we shouldn't use it. this may only be true 
# for solaris 2.5.1
USE_CRYPTLIB=no

# use gcc (gcc-2.95 on 64-bit sparc)
CC=		gcc
KCFLAGS=	-D_KERNEL -Wall -Wstrict-prototypes ${KGCCFLAGS} # -mcpu=ultrasparc
OPTOPTS=	-O
CSHAREDFLAGS=   -fPIC
LDSHARED=	gcc
LDSHAREDFLAGS=  -shared

# use Sun CC (for a 64-bit kernel, uncomment " -xarch=v9 -xregs=no%appl ") 
#CC=		cc
#KCFLAGS=	-D_KERNEL # -xarch=v9 -xregs=no%appl 
#OPTOPTS=	-fast -xO5 -xstrconst
#CSHAREDFLAGS=   -KPIC
#LDSHARED=       cc
#LDSHAREDFLAGS=  -G
#LDFLAGS_EXPORT=

LIBSHARED=      -ldl

# -D_ISOC9X_SOURCE is handled by OSVERSION. basically, it's not needed
# with 2.5.1.
# add -DHAVE_IFNAMEINDEX if you're using solaris 8.
DEFS=	-DNO_STRUCT_TM_GMTOFF -D__svr4__ -DSOLARIS -I../../sys/generic \
	${OSDEFS} ${MACHINEDEFS}
INSTALL=	/usr/ucb/install
AFPLIBS=
ADDLIBS=	-lsocket -lnsl

# Local build stuff.

SRC= linkage.c tpi.c dlpi.c ioc.c if.c aarp.c ddp.c sock.c rt.c
OBJ= linkage.o tpi.o dlpi.o ioc.o if.o aarp.o ddp.o sock.o rt.o

INCPATH=	-I../../include -I../netatalk
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH} ${KCFLAGS} 

ALL=	../../libatalk ../../include ../../bin ../../etc ../../man

oops:
	@echo "Read README again.  Don't type 'make' here."
	@exit 1

all :	kernel ${ALL}

kernel :	FRC
	@case `uname -p` in \
	    i386) \
		PROCESSOR=i386; \
		KGCCFLAGS=""; \
		;; \
	    sparc) \
		PROCESSOR=sparc; \
		KGCCFLAGS="-mno-app-regs -munaligned-doubles \
			-fpcc-struct-return" \
		;; \
	    *) echo "Unknown processor type..."; exit 1 \
		;; \
	esac; \
	if [ x"${OSVERSION}" != x"5.5.1" ]; then \
		OSDEFS=-D_ISOC9X_SOURCE; \
	fi; \
	echo "Making $@ for $$PROCESSOR..."; \
	${MAKE} ${MFLAGS} \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}"\
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    MANDIR="${MANDIR}" DESTDIR="${DESTDIR}" AFSDIR="${AFSDIR}" \
	    KRBDIR="${KRBDIR}" OSDEFS="$${OSDEFS}" \
	    MACHINEDEFS="$${MACHINEDEFS}" \
	    AFPLIBS="${AFPLIBS}" KGCCFLAGS="$${KGCCFLAGS}" ddp

FRC: 

ddp :	${OBJ}
	ld -r -o ddp ${OBJ}

linkage.o : linkage.c
	${CC} ${CFLAGS} -DVERSION=\"`cat ../../VERSION`\" -c linkage.c

../../bin ../../etc:	../../libatalk

${ALL}:	FRC
	if [ x"${OSVERSION}" != x"5.5.1" ]; then \
	  OSDEFS=-D_ISOC9X_SOURCE; \
	fi; \
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" \
	    ADDLIBS="${ADDLIBS}" DEFS="${DEFS} $${OSDEFS}" \
	    OPTOPTS="${OPTOPTS}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    DESTDIR="${DESTDIR}" AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" \
	    AFPLIBS="${AFPLIBS}" LDSHARED="${LDSHARED}" \
	    LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	    LDSHAREDFLAGS="${LDSHAREDFLAGS}" CSHAREDFLAGS="${CSHAREDFLAGS}" \
	    LIBSHARED="${LIBSHARED}" USE_CRYPTLIB="${USE_CRYPTLIB}" \
	    OSDEFS="$${OSDEFS}" MACHINEDEFS="$${MACHINEDEFS}" \
	    all

FRC:

kuninstall : FRC
	rm -f /etc/rc2.d/S79atalk /etc/rc0.d/K79atalk
	if [ x"${SPARC64}" != x ] ; then \
		${RM} /usr/kernel/drv/sparcv9/ddp; \
		${RM} /usr/kernel/strmod/sparcv9/ddp; \
	else \
		${RM} /usr/kernel/drv/ddp; \
		${RM} /usr/kernel/strmod/ddp; \
	fi
	${RM} /usr/kernel/drv/ddp.conf
	-rem_drv ddp
	sync;sync;sync

kinstall : kernel kuninstall
	if [ x"${SPARC64}" != x ]; then \
		${INSTALL} -c ddp /usr/kernel/drv/sparcv9/ddp; \
		ln /usr/kernel/drv/sparcv9/ddp /usr/kernel/strmod/sparcv9/ddp; \
	else \
		${INSTALL} -c ddp /usr/kernel/drv/ddp; \
		ln /usr/kernel/drv/ddp /usr/kernel/strmod/ddp; \
	fi
	${INSTALL} -c ddp.conf /usr/kernel/drv/ddp.conf
	add_drv -m '* 0666 root sys' ddp
	sync;sync;sync
	if [ -f /etc/init.d/atalk ]; then \
		echo "Preserving existing /etc/init.d/atalk settings."; \
	else \
	sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
	    < ../../distrib/initscripts/rc.atalk.sysv > /etc/init.d/atalk; \
	fi
	chmod 744 /etc/init.d/atalk
	-ln -s ../init.d/atalk /etc/rc2.d/S79atalk
	-ln -s ../init.d/atalk /etc/rc0.d/K79atalk

install :
	-mkdir ${DESTDIR} ${SBINDIR} ${BINDIR} ${ETCDIR} ${LIBDIR}
	sed -e s@:BINDIR:@${BINDIR}@ < ../../lp2pap.sh > ${RESDIR}/lp2pap.sh
	chmod 744 ${RESDIR}/lp2pap.sh
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} CC="${CC}" \
		ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	        SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" \
		AFPLIBS="${AFPLIBS}" \
		LDSHARED="${LDSHARED}" LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	  	LDSHAREDFLAGS="${LDSHAREDFLAGS}" \
		CSHAREDFLAGS="${CSHAREDFLAGS}" LIBSHARED="${LIBSHARED}" \
		OSDEFS="$${OSDEFS}" MACHINEDEFS="$${MACHINEDEFS}" \
		INSTALL="${INSTALL}" $@); \
	done
	if [ -f ${ETCDIR}/afpd.conf ]; then \
		echo "Retaining old afpd.conf file.";  \
	else \
		sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
			-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
			-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
			-e s@:INCDIR:@${INCDIR}@ \
			< ../../config/afpd.conf > ${ETCDIR}/afpd.conf; \
	fi
	@echo
	@echo "Install is done.  Don't forget to add lines from"
	@echo "services.atalk to /etc/services and to call rc.atalk"
	@echo "in /etc/rc.  See README and README.SOLARIS for more"
	@echo "information."

clean : sysclean
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

klean sysclean :
	rm -f a.out core* *.o *.bak *[Ee]rrs tags
	rm -f ddp

depend :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" depend); \
	done

# DO NOT DELETE THIS LINE

