# Root of installation. Subdirectories will be ${DESTDIR}/etc,
# ${DESTDIR}/bin, and ${DESTDIR}/lib.
DESTDIR=/usr/local/atalk

# for system-level binaries
SBINDIR=$(DESTDIR)/sbin
# for user-level binaries
BINDIR=$(DESTDIR)/bin
# for program libraries (*.a)
LIBDIR=$(DESTDIR)/lib
# for machine-independent resources (pagecount.ps, etc.)
RESDIR=$(DESTDIR)/etc
# for configuration files (AppleVolumes.system, etc.)
ETCDIR=$(DESTDIR)/etc
# for include files
INCDIR=$(DESTDIR)/include
# Root of man pages.  Subdirectories will be
# ${MANDIR}/man1, ${MANDIR}/man4, and ${MANDIR}/man8.
MANDIR=$(DESTDIR)/man

#INSTALL_PREFIX=
#SBINDIR=${INSTALL_PREFIX}/usr/sbin
#BINDIR=${INSTALL_PREFIX}/usr/bin
#LIBDIR=${INSTALL_PREFIX}/usr/lib
#RESDIR=${INSTALL_PREFIX}/usr/lib/atalk
#ETCDIR=${INSTALL_PREFIX}/etc/atalk
#INCDIR=${INSTALL_PREFIX}/usr/include
#MANDIR=${INSTALL_PREFIX}/usr/man

# Location of the Berkeley v2 db library and include files. 
# NOTE: leave this commented out for now. it's a placeholder for a future
# feature.
#DB2DIR=/usr/local/BerkeleyDB

# Location of the Diffie-Hellman library and include files. Uncomment
# this out if you want DHX as an allowable UAM for afpd. Currently,
# this is set up expecting libcrypto from the openssl project. As a
# result, this option will enable all of the encrypted authentication
# methods (including the Randnum Exchange ones). DHX expects cast.h,
# dh.h, and bn.h in $CRYPTODIR/include with -lcrypto in
# $CRYPTODIR/lib. NOTE: os x server will complain if you use both
# randnum exchange and DHX.
CRYPTODIR=/usr/local/ssl

# Location of the DES library and include files. Uncomment this out if
# you want Randnum Exchange and 2-Way Randnum Exchange as allowable
# UAMs for afpd. We expect libdes.a in $DESDIR/lib and des.h in
# $DESDIR/include.  This option will get overridden by CRYPTODIR.
#DESDIR=/usr/local

# Location of the tcp wrapper library and include files. Comment this out
# if you don't want tcp wrapper support. having tcp wrapper support is
# highly recommended.
TCPWRAPDIR=/usr

# Location of PAM support library and include files. Uncomment this if
# you want to enable PAM support.
#PAMDIR=/usr

# Location of cracklib support library and include files. This is used
# in the password changing routines. Uncomment this out if you want to
# enable support.
#CRACKDIR=/usr

 
# Location of the AFS and Kerberos libraries and include files.  Uncomment
# and edit these if you want to include AFS or Kerberos support in afpd
# or Kerberos support in papd.
#AFSDIR=/usr/local/afs
#KRBDIR=/usr/local/kerberos

# Directory to store node addresses and login names for CAP style
# authenticated printing.  CAP style authenticated printing requires
# that a user mount an appletalk share before they can print.  Afpd
# stores the username in a file named after the Appletalk address which
# papd reads to determine if the user is allowed to print.  These files
# will be stored in the directory below.  Unfortunately, because afpd
# drops privledges, this directory must be writable by any user which
# connects to the server.  Usually, this means public write access (777
# permissions). Uncomment and edit the path if you want CAP style 
# authenticated printing support in afpd and papd.
#CAPDIR=/var/spool/capsec

##########################################################################
all install depend clean tags kernel kinstall kpatch:	FRC
	@case `uname -rs` in \
	    "SunOS 4"*) ARCH=sunos \
		;; \
	    "SunOS 5"*) ARCH=solaris \
		;; \
	    ULTRIX*) ARCH=ultrix \
		;; \
	    Linux*) ARCH=linux \
		;; \
	    FreeBSD*) ARCH=freebsd \
		;; \
	    NetBSD*) ARCH=netbsd \
		;; \
	    OpenBSD*) ARCH=openbsd \
	        ;; \
	    Rhapsody*) ARCH=osx \
		;; \
	    *) ARCH=generic \
		;; \
	esac; \
	echo "Making $@ for $$ARCH..."; \
	cd sys/$$ARCH && ${MAKE} ${MFLAGS} \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}"\
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    DESTDIR="${DESTDIR}" MANDIR="${MANDIR}" \
	    TCPWRAPDIR="${TCPWRAPDIR}" PAMDIR="${PAMDIR}" DB2DIR="${DB2DIR}" \
	    AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" DESDIR="${DESDIR}" \
	    CRYPTODIR="${CRYPTODIR}" CRACKDIR="${CRACKDIR}" \
	    CAPDIR="${CAPDIR}" \
	    OSVERSION="`uname -r`" MACHINETYPE="`uname -m`" \
	    $@

FRC:	include/netatalk

include/netatalk:
	-ln -s ../sys/netatalk include/netatalk

SYS=sunos ultrix solaris
VERSION=`date +%y%m%d`
DISTDIR=../netatalk-${VERSION}

sysclean : FRC
	for i in ${SYS}; \
	    do (cd sys/$$i; ${MAKE} ${MFLAGS} sysclean); \
	done

dist : sysclean clean
	mkdir ${DISTDIR}
	tar cfFFX - EXCLUDE . | (cd ${DISTDIR}; tar xvf - )
	chmod +w ${DISTDIR}/Makefile ${DISTDIR}/include/atalk/paths.h \
		${DISTDIR}/sys/solaris/Makefile
	echo ${VERSION} > ${DISTDIR}/VERSION
