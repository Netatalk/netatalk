SRC=	mktemp.c getusershell.c strcasecmp.c strstr.c flock.c strdup.c \
	inet_aton.c rquota_xdr.c
OBJ=	mktemp.o getusershell.o strcasecmp.o strstr.o flock.o strdup.o \
	inet_aton.o rquota_xdr.o

INCPATH=	-I../../include
CFLAGS=	${DEFS} ${RPCSVCDEFS} ${OPTOPTS} ${INCPATH} 
TAGSFILE=	tags
CC=	cc

all : profiled compatlib

profiled:
	-mkdir profiled

compatlib compatlib_p : ${OBJ}
	@echo "building profiled compatlib"
	@cd profiled; ar cru ../compatlib_p ${OBJ}
	@echo "building normal compatlib"
	@ar cru compatlib ${OBJ}

.c.o :
	${CC} -p ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- $*.o

clean :
	rm -f *.o profiled/*.o *.bak *[Ee]rrs tags
	rm -f compatlib compatlib_p

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

