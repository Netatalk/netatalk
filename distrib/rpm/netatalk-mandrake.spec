#################################################### VERSIONING INFORMATION
%define name    netatalk
%define version 1.5pre6
%define release 1mdk
%define tardir	%{name}-%{version}

################################################# BASIC PACKAGE INFORMATION
Summary: Appletalk and Appleshare/IP services for Linux
Name: %{name}
Version: %{version}
Release: %{release}
Copyright: BSD
Group: Networking/Daemons
Source0: %{name}-%{version}.tar.bz2
URL: http://netatalk.sourceforge.net/
Packager: rufus t firefly <rufus.t.firefly@linux-mandrake.com>
Obsoletes: netatalk-1.4b2+asun netatalk-1.4.99

############################################################## REQUIREMENTS
Requires: cracklib, openssl, tcp_wrappers, pam
BuildRequires: cracklib-devel, openssl-devel, pam-devel

Prefix:    %{_prefix}
BuildRoot: %{_tmppath}/%{name}-buildroot

%description
netatalk is an implementation of the AppleTalk Protocol Suite for Unix/Linux
systems. The current release contains support for Ethertalk Phase I and II,
DDP, RTMP, NBP, ZIP, AEP, ATP, PAP, ASP, and AFP. It provides Appletalk file
printing and routing services on Solaris 2.5, Linux, FreeBSD, SunOS 4.1 and
Ultrix 4. It also supports AFP 2.1 and 2.2 (Appleshare IP).

%package devel
Group: Development/Networking
Summary: Appletalk and Appleshare/IP services for Linux development files
%description devel
netatalk is an implementation of the AppleTalk Protocol Suite for Unix/Linux
systems. The current release contains support for Ethertalk Phase I and II,
DDP, RTMP, NBP, ZIP, AEP, ATP, PAP, ASP, and AFP. It provides Appletalk file
printing and routing services on Solaris 2.5, Linux, FreeBSD, SunOS 4.1 and
Ultrix 4. It also supports AFP 2.1 and 2.2 (Appleshare IP).

This package is required for developing appletalk-based applications.

%changelog

* Thu Apr 12 2001 rufus t firefly <rufus.t.firefly@linux-mandrake.com>
  - v1.5pre6-1mdk
  - pre-release 6 for sourceforge

* Wed Mar 07 2001 rufus t firefly <rufus.t.firefly@linux-mandrake.com>
  - v1.5pre5-1mdk
  - pre-release 5 for sourceforge
  - sync with redhat package

* Mon Dec 18 2000 rufus t firefly <rufus.t.firefly@linux-mandrake.com>
  - v1.5pre3-1mdk
  - pre-release 3 for sourceforge
  - moved away from 1.4.99 ... 

* Wed Nov 08 2000 rufus t firefly <rufus.t.firefly@linux-mandrake.com>
  - v1.4.99-0.20001108mdk
  - pre-release 2 for sourceforge

* Wed Sep 27 2000 rufus t firefly <rufus.t.firefly@linux-mandrake.com>
  - v1.4.99-0.20000927mdk
  - pre-release 1 for sourceforge

%prep
%setup -q -n %{tardir}/

%build
export LD_PRELOAD=
CFLAGS="$RPM_OPT_FLAGS -fomit-frame-pointer -fsigned-char" ./configure \
	--prefix=%{prefix} \
    --with-config-dir=/etc/atalk \
	--with-uams-path=/etc/atalk/uams \
    --with-msg-dir=/etc/atalk/msg \
	--enable-lastdid \
	--enable-redhat \
	--with-cracklib \
	--with-pam \
	--with-shadow \
	--with-tcp-wrappers \
	--with-ssl \
	--enable-pgp-uam
make all

%install
### INSTALL (USING "make install") ###
mkdir -p $RPM_BUILD_ROOT{%{prefix},/etc/atalk/{uams,msg}}
make DESTDIR=$RPM_BUILD_ROOT install-strip

# bzip2 man pages
for i in 1 3 4 5 8; do
	bzip2 -v $RPM_BUILD_ROOT/usr/man/man$i/*.$i
done

%post
### RUN CHKCONFIG ###
/sbin/chkconfig --add atalk
/sbin/ldconfig
# after the first install only
if [ "$1" = 1 ]; then
	# add the ddp lines to /etc/services
	if (grep '[0-9][0-9]*/ddp' /etc/services >/dev/null); then
		cat <<'_EOD1_' >&2
warning: The DDP services appear to be present in /etc/services.
warning: Please check them against services.atalk in the documentation.
_EOD1_
		true
	else
		cat <<'_EOD2_' >>/etc/services
# start of DDP services
#
# Everything between the 'start of DDP services' and 'end of DDP services'
# lines will be automatically deleted when the netatalk package is removed.
#
rtmp		1/ddp		# Routing Table Maintenance Protocol
nbp		2/ddp		# Name Binding Protocol
echo		4/ddp		# AppleTalk Echo Protocol
zip		6/ddp		# Zone Information Protocol

afpovertcp	548/tcp		# AFP over TCP
afpovertcp	548/udp
# end of DDP services
_EOD2_
	fi
fi

%preun
### RUN CHKCONFIG ###
/sbin/chkconfig --del atalk

%postun
# do only for the last un-install
if [ "$1" = 0 ]; then
	# remove the ddp lines from /etc/services
	if (grep '^# start of DDP services$' /etc/services >/dev/null && \
	    grep '^# end of DDP services$'   /etc/services >/dev/null ); then
	  sed -e '/^# start of DDP services$/,/^# end of DDP services$/d' \
	    </etc/services >/tmp/services.tmp$$
	  cat /tmp/services.tmp$$ >/etc/services
	  rm /tmp/services.tmp$$
	else
	  cat <<'_EOD3_' >&2
warning: Unable to find the lines `# start of DDP services` and
warning: `# end of DDP services` in the file /etc/services.
warning: You should remove the DDP services from /etc/services manually.
_EOD3_
	fi
fi

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{tardir}/

%files
%defattr(-,root,root)
%doc [A-Z][A-Z]* ChangeLog doc/[A-Z][A-Z]*
%dir /etc/atalk
%dir /etc/atalk/msg
%config /etc/atalk/Apple*
%config /etc/atalk/*.conf
%config /etc/pam.d/netatalk
%dir /etc/atalk/nls
/etc/atalk/nls/*
%dir /etc/atalk/uams
/etc/atalk/uams/*.so
/etc/rc.d/init.d/atalk
%{prefix}/bin/*
%{prefix}/sbin/*
%{prefix}/man/man*/*

%files devel
%defattr(-,root,root)
%{prefix}/lib/*.a
%dir %{prefix}/include/atalk
%{prefix}/include/atalk/*.h
%dir %{prefix}/include/netatalk
%{prefix}/include/netatalk/*.h
%{prefix}/share/aclocal/netatalk.m4
