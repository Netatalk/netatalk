Summary: AppleTalk networking programs
Name: netatalk
Version: 1.4b2+asun2.1.4
Release: pre39
Packager: iNOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
Copyright: BSD
Group: Networking
Source0: ftp://ftp.cobaltnet.com/pub/users/asun/testing/pre-asun2.1.4-36.tar.gz
Patch0: netatalk-asun.makefile.patch
Requires: pam >= 0.56
BuildRoot: /var/tmp/atalk

%description
This package enables Linux to talk to Macintosh computers via the
AppleTalk networking protocol. It includes a daemon to allow Linux
to act as a file server over AppleTalk or IP for Mac's.

%package devel
Summary: Headers and static libraries for Appletalk development
Group: Development/Libraries

%description devel
This packge contains the header files, and static libraries for building
Appletalk networking programs.

%prep
%setup
%patch0 -p1

%build
make OPTOPTS="$RPM_OPT_FLAGS -fomit-frame-pointer -fsigned-char" OSVERSION=2.0

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/atalk
mkdir -p $RPM_BUILD_ROOT/etc/pam.d
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
for i in 0 1 2 3 4 5 6; do
	mkdir -p $RPM_BUILD_ROOT/etc/rc.d/rc$i.d
done
mkdir -p $RPM_BUILD_ROOT/usr/lib/atalk

make install INSTALL_PREFIX=$RPM_BUILD_ROOT

for i in aecho getzones megatron nbplkup nbprgstr nbpunrgstr pap \
	papstatus psorder; do
	strip $RPM_BUILD_ROOT/usr/bin/$i
done
for i in afpd atalkd psf psa papd; do
	strip $RPM_BUILD_ROOT/usr/sbin/$i
done

install -m644 config/AppleVolumes.system $RPM_BUILD_ROOT/etc/atalk/AppleVolumes.system
install -m644 config/AppleVolumes.default $RPM_BUILD_ROOT/etc/atalk/AppleVolumes.default
install -m644 config/atalkd.conf $RPM_BUILD_ROOT/etc/atalk/atalkd.conf
install -m644 config/papd.conf $RPM_BUILD_ROOT/etc/atalk/papd.conf

# This is not necessary because chkconfig will make the links.
for i in 0 1 2 6; do
	ln -sf ../init.d/atalk $RPM_BUILD_ROOT/etc/rc.d/rc$i.d/K35atalk
done
for i in 3 4 5; do
	ln -sf ../init.d/atalk $RPM_BUILD_ROOT/etc/rc.d/rc$i.d/S91atalk
done

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/chkconfig --add atalk
ldconfig
# Do only for the first install
if [ "$1" = 1 ] ; then
  # Add the ddp lines to /etc/services
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

%postun
# Do only for the last un-install
if [ "$1" = 0 ] ; then
  /sbin/chkconfig --del atalk
  # remove the ddp lines from /etc/services
  if (grep '^# start of DDP services$' /etc/services >/dev/null && \
      grep '^# end of DDP services$' /etc/services >/dev/null ); then
    sed -e '/^# start of DDP services$/,/^# end of DDP services$/d' \
	</etc/services >/tmp/services.tmp$$
    cat /tmp/services.tmp$$ >/etc/services
    rm /tmp/services.tmp$$
  else
    cat <<'_EOD3_' >&2
warning: Unable to find the lines `# start of DDP services' and
warning: `# end of DDP services' in the file /etc/services.
warning: You should remove the DDP services from /etc/service manually.
_EOD3_
  fi
fi

%files
%doc BUGS CHANGES CONTRIBUTORS COPYRIGHT ChangeLog INSTALL/ README* TODO VERSION contrib/ services.atalk
%dir /etc/atalk
%config /etc/atalk/AppleVolumes.default
%config /etc/atalk/AppleVolumes.system
%config /etc/atalk/netatalk.conf
%config /etc/atalk/afpd.conf
%config /etc/atalk/atalkd.conf
%config /etc/atalk/papd.conf
%config /etc/rc.d/init.d/atalk
%config /etc/pam.d/netatalk
/etc/rc.d/rc0.d/K35atalk
/etc/rc.d/rc1.d/K35atalk
/etc/rc.d/rc2.d/K35atalk
/etc/rc.d/rc3.d/S91atalk
/etc/rc.d/rc4.d/S91atalk
/etc/rc.d/rc5.d/S91atalk
/etc/rc.d/rc6.d/K35atalk
/usr/sbin/afpd
/usr/sbin/atalkd
/usr/sbin/papd
/usr/sbin/psa
/usr/sbin/etc2ps
/usr/sbin/psf
/usr/bin/adv1tov2
/usr/bin/aecho
/usr/bin/afppasswd
/usr/bin/binheader
/usr/bin/getzones
/usr/bin/hqx2bin
/usr/bin/macbinary
/usr/bin/megatron
/usr/bin/nadheader
/usr/bin/nbplkup
/usr/bin/nbprgstr
/usr/bin/nbpunrgstr
/usr/bin/pap
/usr/bin/papstatus
/usr/bin/psorder
/usr/bin/single2bin
/usr/bin/unbin
/usr/bin/unhex
/usr/bin/unsingle
%dir /usr/lib/atalk
/usr/lib/atalk/filters/
/usr/lib/atalk/nls/
/usr/lib/atalk/pagecount.ps
/usr/lib/atalk/uams/
/usr/man/man1/aecho.1
/usr/man/man1/getzones.1
/usr/man/man1/hqx2bin.1
/usr/man/man1/macbinary.1
/usr/man/man1/megatron.1
/usr/man/man1/nbp.1
/usr/man/man1/nbplkup.1
/usr/man/man1/nbprgstr.1
/usr/man/man1/pap.1
/usr/man/man1/papstatus.1
/usr/man/man1/psorder.1
/usr/man/man1/single2bin.1
/usr/man/man1/unbin.1
/usr/man/man1/unhex.1
/usr/man/man1/unsingle.1
/usr/man/man3/atalk_aton.3
/usr/man/man3/nbp_name.3
/usr/man/man4/atalk.4
/usr/man/man8/afpd.8
/usr/man/man8/atalkd.8
/usr/man/man8/papd.8
/usr/man/man8/psf.8

%files devel
/usr/lib/libatalk.a
/usr/lib/libatalk_p.a
/usr/include/atalk/
/usr/include/netatalk/

%changelog
* Thu Jul 22 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- /etc/atalk/netatalk.config -> /etc/atalk/netatalk.conf
  Many parts of patch are merged into the original source code.
* Tue Jul 13 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- AppleVolumes.system is merged into the original source code.
  /etc/atalk/config -> /etc/atalk/netatalk.config.
  Merge original rc.atalk.redhat and /etc/rc.d/init.d/atalk.
  Remove last sample line of patched afpd.conf.
* Fri Jul 9 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.4-30]
* Sun Jun 20 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.4-28]
* Thu Jun 3 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.4-22]
* Wed May 19 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.4-15]
  Make BerkleyDB=/usr.
* Sun May 2 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.4-11]
  Integrate three patches into netatalk-asun.makefile.patch.
  Change /etc/uams dir to /usr/lib/atalk/uams.
  Add configuration line to /etc/atalk/afpd.conf and remove needless 
  variables from /etc/atalk/config and /etc/rc.d/init.d/atalk.
* Wed Apr 21 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.4-9]
  Move %chengelog section last.
* Wed Mar 31 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- Comment out -DNEED_QUOTA_WRAPPER in sys/linux/Makefile.
* Sat Mar 20 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- Correct symbolic links to psf.
  Remove asciize function from nbplkup so as to display Japanese hostname.
* Thu Mar 11 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- Included MacPerl 5 script ICDumpSuffixMap which dumps suffix mapping
  containd in Internet Config Preference.
* Tue Mar 2 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [asun2.1.3]
* Mon Feb 15 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.2-8]
* Sun Feb 7 1999 iNOUE Koich! <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.2-6]
* Mon Jan 25 1999 iNOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.2-3]
* Thu Dec 17 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- [pre-asun2.1.2]
  Remove crlf patch. It is now a server's option.
* Thu Dec 3 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use stable version source netatalk-1.4b2+asun2.1.1.tar.gz
  Add uams directory
* Sat Nov 28 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use pre-asun2.1.1-3 source.
* Mon Nov 23 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use pre-asun2.1.1-2 source.
* Mon Nov 16 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Fix rcX.d's symbolic links.
* Wed Oct 28 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use pre-asun2.1.0a-2 source. Remove '%exclusiveos linux' line.
* Sat Oct 24 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use stable version source netatalk-1.4b2+asun2.1.0.tar.gz.
* Mon Oct 5 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use pre-asun2.1.0-10a source.
* Thu Sep 19 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use pre-asun2.1.0-8 source. Add chkconfig support.
* Sat Sep 12 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Comment out -DCRLF. Use RPM_OPT_FLAGS.
* Mon Sep 8 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use pre-asun2.1.0-7 source. Rename atalk.init to atalk.
* Mon Aug 22 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use pre-asun2.1.0-6 source.
* Mon Jul 27 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use pre-asun2.1.0-5 source.
* Tue Jul 21 1998 INOUE Koichi <inoue@ma.ns.musashi-techa.c.jp>
- Use pre-asun2.1.0-3 source.
* Tue Jul 7 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Add afpovertcp entries to /etc/services
- Remove BuildRoot in man8 pages
* Mon Jun 29 1998 INOUE Koichi <inoue@ma.ns.musashi-tech.ac.jp>
- Use modified sources 1.4b2+asun2.1.0 produced by Adrian Sun
  <asun@saul9.u.washington.edu> to provide an AppleShareIP file server
- Included AppleVolumes.system file maintained by Johnson
  <johnson@stpt.usf.edu>
* Mon Aug 25 1997 David Gibson <D.Gibson@student.anu.edu.au>
- Used a buildroot
- Use RPM_OPT_FLAGS
- Moved configuration parameters/files from atalk.init to /etc/atalk
- Separated devel package
- Built with shared libraries
* Sun Jul 13 1997 Paul H. Hargrove <hargrove@sccm.Stanford.EDU>
- Updated sources from 1.3.3 to 1.4b2
- Included endian patch for Linux/SPARC
- Use all the configuration files supplied in the source.  This has the
  following advantages over the ones in the previous rpm release:
	+ The printer 'lp' isn't automatically placed in papd.conf
	+ The default file conversion is binary rather than text.
- Automatically add and remove DDP services from /etc/services
- Placed the recommended /etc/services in the documentation
- Changed atalk.init to give daemons a soft kill
- Changed atalk.init to make configuration easier

* Wed May 28 1997 Mark Cornick <mcornick@zorak.gsfc.nasa.gov>
Updated for /etc/pam.d
