SRC = ad_open.c ad_flush.c ad_read.c ad_write.c ad_size.c ad_mmap.c \
      ad_lock.c ad_date.c ad_attr.c ad_sendfile.c
OBJ = ad_open.o ad_flush.o ad_read.o ad_write.o ad_size.o ad_mmap.o \
      ad_lock.o ad_date.o ad_attr.o ad_sendfile.o

INCPATH=	-I../../include
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH}
TAGSFILE=	tags
CC=	cc

all : profiled adoublelib

profiled:
	-mkdir profiled

adoublelib adoublelib_p : ${OBJ}
	@echo "building profiled adoublelib"
	@cd profiled; ar cru ../adoublelib_p ${OBJ}
	@echo "building normal adoublelib"
	@ar cru adoublelib ${OBJ}

.c.o :
	${CC} -p ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- $*.o

clean :
	rm -f *.o profiled/*.o *.bak *[Ee]rrs tags
	rm -f adoublelib adoublelib_p

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

