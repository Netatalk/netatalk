TARGETS= afile achfile 
MANSRC= afile.1 achfile.1 acleandir.1

INCPATH=	-I../../include -I../../system
BINDIR=		${PREFIX}/bin
MANDIR=		${PREFIX}/man

CFLAGS=		 -g ${INCPATH}
CC=	gcc

all: ${TARGETS}

afile	: afile.c
	$(CC) ${CFLAGS} -o afile afile.c

achfile	: achfile.c
	$(CC) ${CFLAGS} -o achfile achfile.c

install : all
	@install -d ${BINDIR}
	install  $(TARGETS) ${BINDIR}
	install acleandir.rc ${BINDIR}/acleandir

install.man : $(MANSRC)	
	@install -d ${MANDIR}/man1
	install  $(MANSRC) ${MANDIR}/man1

clean	:
	rm -f a.out core* *.o *.bak *[Ee]rrs 
	rm -f afile achfile
