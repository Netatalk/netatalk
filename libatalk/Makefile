ALL=	asp dsi atp nbp adouble util compat netddp 
# cnid
LIBS=	asp/asplib atp/atplib nbp/nbplib adouble/adoublelib \
	dsi/dsilib util/utillib compat/compatlib netddp/netddplib 
# cnid/cnidlib
PLIBS=	asp/asplib_p atp/atplib_p nbp/nbplib_p adouble/adoublelib_p \
	dsi/dsilib_p util/utillib_p compat/compatlib_p  \
	netddp/netddplib 
# cnid/cnidlib_p
TAGSFILE=tags
CC=cc
INSTALL=install
RANLIB=ranlib

all:	libatalk.a

libatalk.a: ${LIBS}
	rm -rf tmp
	-mkdir tmp
#	touch tmp/________64ELEL_
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib); done
	(cd tmp; ar cr libatalk.a  *.o; ${RANLIB} libatalk.a)
	mv tmp/libatalk.a libatalk.a
	${RANLIB} libatalk.a
	rm -rf tmp tmp_p
	-mkdir tmp_p
#	touch tmp_p/________64ELEL_
	for i in ${ALL}; do (cd tmp_p; ar x ../$$i/$${i}lib_p); done
	(cd tmp_p; ar cr libatalk_p.a  *.o; ${RANLIB} libatalk_p.a)
	mv tmp_p/libatalk_p.a libatalk_p.a
	${RANLIB} libatalk_p.a
	rm -rf tmp_p

${ALL}: FRC
	if [ ! -f /usr/lib/librpcsvc.a -a ! -f /lib/librpcsvc.a ]; then \
	    RPCSVCDEFS=-DNEED_RQUOTA; \
	fi; \
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" DEFS="${DEFS}" \
	    RPCSVCDEFS="$${RPCSVCDEFS}" \
	    OPTOPTS="${OPTOPTS}" TCPWRAPDIR="${TCPWRAPDIR}" DB2DIR="${DB2DIR}"

asp/asplib:	asp
dsi/dsilib:     dsi
atp/atplib:	atp
nbp/nbplib:	nbp
adouble/adoublelib:	adouble
util/utillib:	util
compat/compatlib:	compat
cnid/cnidlib:	cnid
netddp/netddplib:	netddp

FRC:

tags:
	for i in ${ALL}; do \
	    (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" \
		TAGSFILE=../${TAGSFILE} tags); \
	done

install: all
	-mkdir ${LIBDIR}
	${INSTALL} -c libatalk.a ${LIBDIR}/libatalk.a
	(cd ${LIBDIR}; ranlib ${LIBDIR}/libatalk.a)
	${INSTALL} -c libatalk_p.a ${LIBDIR}/libatalk_p.a
	(cd ${LIBDIR}; ranlib ${LIBDIR}/libatalk_p.a)

clean:
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done
	rm -f *[Ee]rrs libatalk.a libatalk_p.a
	rm -rf tmp tmp_p

depend:
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS=${DEFS} depend); \
	done
