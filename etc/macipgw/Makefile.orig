#	AppleTalk MacIP Gateway
#
# (c) 2013, 1997 Stefan Bethke. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice unmodified, this list of conditions, and the following
#    disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

PROG=		macipgw
DESTDIR?=	/usr/pkg
BINDIR?=	bin
MANDIR=		man/man

BINOWN?=	root
BINGRP?=	wheel
MANOWN?=	root
MANGRP?=	wheel

SRCS=	main.c macip.c atp_input.c nbp_lkup_async.c util.c
OBJS=	main.o macip.o atp_input.o nbp_lkup_async.o util.o
MAN=	macipgw.8
MANGZ=	${MAN}.gz

SRCS+=	tunnel_bsd.c
OBJS+=	tunnel_bsd.o
#SRCS+=	tunnel_linux.c
#OBJS+=	tunnel_linux.o

CFLAGS+=	-g -Wall -I/usr/local/include -I/usr/pkg/include -O2
CFLAGS+=	-DDEBUG

LDADD=	-latalk
LDFLAGS+=	-L/usr/local/lib -L/usr/pkg/lib

CLEANFILES+=	${PROG}
CLEANFILES+=	${OBJS}
CLEANFILES+=	${MANGZ}

all: ${PROG} ${MANGZ}

${PROG}:	${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -o ${PROG} ${OBJS} ${LDADD}

${MANGZ}:	${MAN}
	gzip -9 <${MAN} >${MANGZ}

clean:
	rm -f ${CLEANFILES} 

install: ${PROG} ${MANGZ}
	install -c -m 555 -o ${BINOWN} -g ${BINGRP} -s ${PROG} ${DESTDIR}/${BINDIR}
	install -c -m 444 -o ${MANOWN} -g ${MANGRP} macipgw.8.gz ${DESTDIR}/${MANDIR}8
