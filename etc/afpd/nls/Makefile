SRC = makecode.c parsecode.c 
OBJ = makecode.o parsecode.o

INCPATH= -I../../../include 
CFLAGS=	${DEFS} ${OPTOPTS} ${INCPATH}

all:: codepage.h parsecode makecode

codepage.h:
	-ln -s ../codepage.h .

parsecode: parsecode.o
	${CC} ${CFLAGS} ${LDFLAGS} parsecode.o -o parsecode

makecode: makecode.o
	${CC} ${CFLAGS} ${LDFLAGS} makecode.o -o makecode

install: 
	-mkdir ${RESDIR}/nls
	-./makecode
	-${INSTALL} -m 644 maccode.* ${RESDIR}/nls

clean:
	rm -f *.o
	rm -f makecode parsecode maccode.*
