SRC = cnid_open.c cnid_close.c cnid_add.c cnid_get.c cnid_delete.c \
      cnid_update.c cnid_resolve.c cnid_lookup.c cnid_nextid.c
OBJ = cnid_open.o cnid_close.o cnid_add.o cnid_get.o cnid_delete.o \
      cnid_update.o cnid_resolve.o cnid_lookup.o cnid_nextid.o

INCPATH= -I../../include -I${DB2DIR}/include
CFLAGS= ${DEFS} ${OPTOPTS} ${INCPATH} 

TAGSFILE=	tags
CC=	cc

all : profiled cnidlib 

profiled:
	-mkdir profiled

cnidlib cnidlib_p : ${OBJ}
	@echo "building profiled cnidlib"
	@cd profiled; ar cru ../cnidlib_p ${OBJ}
	@echo "building normal cnidlib"
	@ar cru cnidlib ${OBJ}

.c.o :
	${CC} -p ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -r -o $*.o- $*.o
	mv $*.o- $*.o

clean :
	rm -f *.o profiled/*.o *.bak *[Ee]rrs tags
	rm -f cnidlib cnidlib_p

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
