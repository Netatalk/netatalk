TARGETS=	atalk netatalk

INSTALL=	install

all :

install : all
	-mkdir ${DESTDIR}
	-mkdir ${INCDIR}
	for i in ${TARGETS}; do \
	    rm -rf ${INCDIR}/$$i ; \
	    mkdir ${INCDIR}/$$i ; \
	    cp $$i/*.h ${INCDIR}/$$i ; \
	done

clean :

tags :

depend :

# DO NOT DELETE THIS LINE

