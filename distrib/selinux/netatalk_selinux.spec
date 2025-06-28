# vim: sw=4:ts=4:et


%define relabel_files() \
restorecon -R /usr/sbin/netatalk; \

%define selinux_policyver 41.44-1

Name:   netatalk_selinux
Version:	1.0
Release:	1%{?dist}
Summary:	SELinux policy module for netatalk

Group:	System Environment/Base
License:	GPLv2+
URL:		http://HOSTNAME
Source0:	netatalk.pp
Source1:	netatalk.if
Source2:	netatalk_selinux.8


Requires: policycoreutils-python-utils, libselinux-utils
Requires(post): selinux-policy-base >= %{selinux_policyver}, policycoreutils-python-utils
Requires(postun): policycoreutils-python-utils
BuildArch: noarch

%description
This package installs and sets up the  SELinux policy security module for netatalk.

%install
install -d %{buildroot}%{_datadir}/selinux/packages
install -m 644 %{SOURCE0} %{buildroot}%{_datadir}/selinux/packages
install -d %{buildroot}%{_datadir}/selinux/devel/include/contrib
install -m 644 %{SOURCE1} %{buildroot}%{_datadir}/selinux/devel/include/contrib/
install -d %{buildroot}%{_mandir}/man8/
install -m 644 %{SOURCE2} %{buildroot}%{_mandir}/man8/netatalk_selinux.8
install -d %{buildroot}/etc/selinux/targeted/contexts/users/


%post
semodule -n -i %{_datadir}/selinux/packages/netatalk.pp

if [ $1 -eq 1 ]; then

fi
if /usr/sbin/selinuxenabled ; then
    /usr/sbin/load_policy
    %relabel_files
fi;
exit 0

%postun
if [ $1 -eq 0 ]; then

    semodule -n -r netatalk
    if /usr/sbin/selinuxenabled ; then
       /usr/sbin/load_policy
       %relabel_files
    fi;
fi;
exit 0

%files
%attr(0600,root,root) %{_datadir}/selinux/packages/netatalk.pp
%{_datadir}/selinux/devel/include/contrib/netatalk.if
%{_mandir}/man8/netatalk_selinux.8.*


%changelog
* Fri Jun 27 2025 Daniel Markstedt <daniel@mindani.net> 1.0-1
- Initial version

