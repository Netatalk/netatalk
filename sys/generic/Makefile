# generic system. here are some useful defines.
# system lookalikes:    -DBSD4_4 -D__svr4__ -DMACOSX_SERVER
# statfs/quota support: -DNO_QUOTA_SUPPORT -DUSE_QUOTA_H -DUSE_UFS_QUOTA_H
#                       -DHAVE_2ARG_DBTOB -DHAVE_DQB_BTIMELIMIT
#                       -DUSE_OLD_RQUOTA (really old rquota support)
#                       -DUSE_VFS_H -DUSE_STATFS_H -DUSE_STATVFS_H
#                       -DUSE_MNTTAB_H -DUSE_MNTENT_H -DUSE_MOUNT_H
# integer sizes:        -D_ISOC9X_SOURCE -DHAVE_64BIT_LONGS -DHAVE_32BIT_LONGS
# other 64-bit issues:  -DTRY_64BITOFF_T (for byte locks)
# afp/ddp:              -DNO_DDP
# to get sys/cdefs.h:   -I../../sys/generic
# misc:                 -DUSE_GETHOSTID -DNEED_GETUSERSHELL -DHAVE_BROKEN_CPP
#                       -DNEED_RQUOTA -DNO_STRUCT_TM_GMTOFF
#                       -DHAVE_IFNAMEINDEX
#
# plug-in uam support needs to access the run-time library
# loader. here are the relevant options for that:
# -DDLSYM_PREPEND_UNDERSCORE
# -DNO_DLFCN_H (you'll need to make libatalk/util/module.c do the right thing)
#
# here's some flags appropriate for gnu gcc.
# CSHAREDFLAGS=-fPIC
# LDSHARED=gcc
# LDSHAREDFLAGS=-shared
# LDFLAGS_EXPORT=-rdynamic
# LIBSHARED=-ldl
#
# the following combinations are not guaranteed to do anything, but
# you might have luck with them. make sure that you comment out
# TCPWRAPDIR, PAMDIR, or DESDIR/CRYPTODIR if you don't have the
# respective parts installed.
#
# digital unix: 
#   -DHAVE_64BIT_LONGS -DNO_DDP -DUSE_OLD_RQUOTA -DUSE_MOUNT_H \
#   -DUSE_UFS_QUOTA_H -I../../sys/generic
#  LIBSHARED=-ldl
#  LDSHAREDFLAGS=-shared -expect_unresolved '*' -s
#  uncomment the two touch lines in libatalk/Makefile to get around a
#  problem with ar. use gcc.
#
# i think this will work for machten, but i'm not sure:
#   -DBSD4_4 -DNO_QUOTA_SUPPORT -DNO_DDP
#
# sgi irix:
#   -DUSE_MNTENT_H -DHAVE_DQB_BTIMELIMIT -DUSE_QUOTA_H -DNO_DDP \
#   -DNEED_GETUSERSHELL -DUSE_STATVFS_H -DUSE_OLD_RQUOTA -DNO_STRUCT_TM_GMTOFF
#   change RANLIB in libatalk/Makefile from 'ranlib' to 'echo'
#
# aix:
#   -DHAVE_32BIT_LONGS -DHAVE_IFNAMEINDEX -DNO_DDP -DUSE_OLD_RQUOTA \
#   -I../../sys/generic
#   ADDLIBS=-lbsd
#   use cc.
#
DEFS=-DHAVE_64BIT_LONGS -DNO_DDP -DUSE_OLD_RQUOTA -DUSE_MOUNT_H \
   -DUSE_UFS_QUOTA_H -I../../sys/generic
OPTOPTS=	-O2 -fomit-frame-pointer
#CC =   cc
CC=	gcc
CSHAREDFLAGS=-fPIC
LDSHARED=gcc
LDSHAREDFLAGS=-Wl,-E
INSTALL=	install
AFPLIBS=
#ADDLIBS=-lbsd
LIBSHARED=-ldl

# some oses have -lcrypt but shouldn't use it. uncomment the following
# line if that's the case.
#USE_CRYPTLIB=no

ALL=	../../libatalk ../../include ../../bin ../../etc ../../man

oops:
	@echo "Read README again.  Don't type 'make' here."
	@exit 1

all:	${ALL}

../../bin ../../etc:	../../libatalk

${ALL}:	FRC
	cd $@; ${MAKE} ${MFLAGS} CC="${CC}" \
	    ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	    SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	    ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
	    DESTDIR="${DESTDIR}" AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" \
	    AFPLIBS="${AFPLIBS}" LDSHARED="${LDSHARED}" \
	    LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	    LDSHAREDFLAGS="${LDSHAREDFLAGS}" CSHAREDFLAGS="${CSHAREDFLAGS}" \
	    LIBSHARED="${LIBSHARED}" USE_CRYPTLIB="${USE_CRYPTLIB}" \
	    all

FRC:

install :
	-mkdir ${DESTDIR} ${SBINDIR} ${BINDIR} ${ETCDIR} ${LIBDIR}
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} CC="${CC}" \
		ADDLIBS="${ADDLIBS}" DEFS="${DEFS}" OPTOPTS="${OPTOPTS}" \
	        SBINDIR="${SBINDIR}" BINDIR="${BINDIR}" RESDIR="${RESDIR}" \
	        ETCDIR="${ETCDIR}" LIBDIR="${LIBDIR}" INCDIR="${INCDIR}" \
		AFSDIR="${AFSDIR}" KRBDIR="${KRBDIR}" AFPLIBS="${AFPLIBS}" \
		LDSHARED="${LDSHARED}" LDFLAGS_EXPORT="${LDFLAGS_EXPORT}" \
	  	LDSHAREDFLAGS="${LDSHAREDFLAGS}" \
		CSHAREDFLAGS="${CSHAREDFLAGS}" LIBSHARED="${LIBSHARED}" \
		INSTALL="${INSTALL}" $@); \
	done
	rm -f ${ETCDIR}/rc.atalk
	sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
		-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
		-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
		-e s@:INCDIR:@${INCDIR}@ \
	    < ../../distrib/scripts/rc.atalk.bsd > ${ETCDIR}/rc.atalk
	chmod 744 ${ETCDIR}/rc.atalk 
	if [ -f ${ETCDIR}/afpd.conf ]; then \
		echo "Retaining old afpd.conf file.";  \
	else \
		sed -e s@:DESTDIR:@${DESTDIR}@ -e s@:SBINDIR:@${SBINDIR}@ \
			-e s@:BINDIR:@${BINDIR}@ -e s@:RESDIR:@${RESDIR}@ \
			-e s@:ETCDIR:@${ETCDIR}@ -e s@:LIBDIR:@${LIBDIR}@ \
			-e s@:INCDIR:@${INCDIR}@ \
			< ../../config/afpd.conf > ${ETCDIR}/afpd.conf; \
	fi
	@echo
	@echo "Install is done.  Don't forget to add lines from"
	@echo "services.atalk to /etc/services and to call rc.atalk"
	@echo "in /etc/rc.  See README and README.GENERIC for more"
	@echo "information."

clean :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

depend :
	for i in ${ALL}; \
	    do (cd $$i; ${MAKE} ${MFLAGS} DEFS="${DEFS}" depend); \
	done

# DO NOT DELETE THIS LINE

