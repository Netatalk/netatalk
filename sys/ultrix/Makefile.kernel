# Ultrix specific defines, passed to subdirectories.
# i believe that the current setup will break with this.
#DEFS=
DEFS=-I../../sys/generic -DUSE_OLD_RQUOTA
OPTOPTS=
#CC=	cc 
CC = gcc 
CSHAREDFLAGS=   -fPIC
LDSHARED=	gcc
LDSHAREDFLAGS=  -shared
LDFLAGS_EXPORT=-rdynamic

INSTALL=	install

LIBSHARED=      -ldl
AFPLIBS= 
ADDLIBS=

INCPATH = -I../../include -I../netatalk
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH}

ALL=	../../libatalk ../../include ../../bin ../../etc ../../man

oops:
	@echo "Read README again.  Don't type 'make' here."
	@exit 1

all:	 ${ALL}

../../bin ../../etc:	../../libatalk

${ALL}:	FRC
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" \
	    ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    DESTDIR="${DESTDIR}" AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" \
	    AFPLIBS="${AFPLIBS}" LDSHARED="${LDSHARED}" \
	    LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	    LDSHAREDFLAGS="${LDSHAREDFLAGS}" CSHAREDFLAGS="${CSHAREDFLAGS}" \
	    LIBSHARED="${LIBSHARED}" \
	    all

FRC: kpatch-4.3 kpatch-4.4

kpatch-4.3:
	-ln -s kpatch-4.2 kpatch-4.3
kpatch-4.4:
	-ln -s kpatch-4.2 kpatch-4.4

install :
	-mkdir ${DESTDIR}
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} CC="${CC}" \
		ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	        SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		DESTDIR="${DESTDIR}" MANDIR="${MANDIR}" \
		AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" AFPLIBS="${AFPLIBS}" \
		LDSHARED="${LDSHARED}" LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	  	LDSHAREDFLAGS="${LDSHAREDFLAGS}" \
		CSHAREDFLAGS="${CSHAREDFLAGS}" LIBSHARED="${LIBSHARED}" \
		INSTALL="${INSTALL}" $@); \
	done
	rm -f ${ETCDIR}/rc.atalk
	sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
	    < ../../distrib/initscripts/rc.atalk.bsd > ${ETCDIR}/rc.atalk
	@echo
	@echo "Install is done.  Don't forget to add lines from"
	@echo "services.atalk to /etc/services and to call rc.atalk
	@echo "in /etc/rc.  Next, install the kernel patches."
	@echo "See README.ULTRIX for more information."

kpatch :
	@echo "WARNING!!!  This patches your kernel!!!"
	@echo -n "(hit control-c with in 10 seconds, to stop)"
	@sleep 10
	@echo
	@echo
	@if grep -s -w atalk /sys/conf/files; then \
	    echo "You already have a version of netatalk installed."; \
	    echo "You will have to remove this old version.  See"; \
	    echo "README.ULTRIX for specific instructions."; \
	    exit 1; \
	else \
	    case `/bin/uname -r` in \
		4.1) echo -n "Applying 4.1 patches..."; \
		    patch -s -d /sys -p0 < kpatch-4.1;  \
		    echo " done."; \
		    ;; \
		4.2|4.3|4.4) echo -n "Applying 4.2/4.3/4.4 patches..."; \
		    patch -s -d /sys -p0 < kpatch-4.2;  \
		    echo " done."; \
		    ;; \
		*) echo "Unknown release of Ultrix"; exit 1; \
		    ;; \
	    esac; \
	fi
	@echo
	@echo "Next, install the netatalk kernel files.  See README.ULTRIX"
	@echo "for specific instructions."

kinstall :
	@echo "Copying netatalk to kernel building area..."
	-mkdir /sys/net/netatalk
	cp ../netatalk/*.[ch] *.[ch] /sys/net/netatalk
	@echo "Done."
	@echo
	@echo "Next, make a new kernel.  See README.ULTRIX"
	@echo "for specific instructions."

clean :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

sysclean :

depend :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" depend); \
	done
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

