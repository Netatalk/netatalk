SRC = dsi_attn.c dsi_close.c dsi_cmdreply.c dsi_getsess.c \
	dsi_getstat.c dsi_init.c dsi_opensess.c dsi_read.c \
	dsi_tcp.c dsi_tickle.c dsi_write.c dsi_stream.c 
OBJ = dsi_attn.o dsi_close.o dsi_cmdreply.o dsi_getsess.o \
	dsi_getstat.o dsi_init.o dsi_opensess.o dsi_read.o \
	dsi_tcp.o dsi_tickle.o dsi_write.o dsi_stream.o 

INCPATH=	-I../../include ${TCPWRAPINCPATH}
CFLAGS = ${DEFS} ${OPTOPTS} ${INCPATH} ${TCPWRAPDEFS}
TAGSFILE=	tags
CC=	cc

all : 
	if [ x"${TCPWRAPDIR}" != x ]; then \
	  TCPWRAPDEFS="-DTCPWRAP"; \
	  if [ "${TCPWRAPDIR}" != "/usr" ]; then \
	    TCPWRAPINCPATH="-I${TCPWRAPDIR}/include"; \
	  fi; \
	fi; \
	${MAKE} ${MFLAGS} CC="${CC}" DEFS="${DEFS}" \
	OPTOPTS="${OPTOPTS}" \
	TCPWRAPINCPATH="$${TCPWRAPINCPATH}" TCPWRAPDEFS="$${TCPWRAPDEFS}" \
	dsilib

profiled:
	-mkdir profiled

dsilib dsilib_p : profiled ${OBJ}
	@echo "building profiled dsilib"
	@cd profiled; ar cru ../dsilib_p ${OBJ}
	@echo "building normal dsilib"
	@ar cru dsilib ${OBJ}

.c.o :
	${CC} -p ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- $*.o

clean :
	rm -f *.o profiled/*.o *.bak *[Ee]rrs tags
	rm -f dsilib dsilib_p

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
