SRC=	atp_bufs.c atp_close.c atp_open.c atp_packet.c atp_rreq.c \
	atp_rresp.c atp_rsel.c atp_sreq.c atp_sresp.c
OBJ=	atp_bufs.o atp_close.o atp_open.o atp_packet.o atp_rreq.o \
	atp_rresp.o atp_rsel.o atp_sreq.o atp_sresp.o

INCPATH=	-I../../include
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH} # -DDROP_ATPTREL -DEBUG -DDROPPACKETS -g
TAGSFILE=	tags
CC=	cc

all: profiled atplib

profiled:
	-mkdir profiled

atplib atplib_p : ${OBJ}
	@echo "building profiled atplib"
	@cd profiled; ar cru ../atplib_p ${OBJ}
	@echo "building normal atplib"
	@ar cru atplib ${OBJ}

.c.o :
	${CC} -p ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- $*.o

clean :
	rm -f *.o profiled/*.o *.bak *[Ee]rrs tags
	rm -f atplib atplib_p

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

