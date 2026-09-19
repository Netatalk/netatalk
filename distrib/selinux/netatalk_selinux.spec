# vim: sw=4:ts=4:et

# Keep this list in sync with netatalk.sh. Missing optional files are normal.
%define relabel_files() \
for path in /usr/sbin/netatalk /usr/sbin/afpd /usr/sbin/cnid_metad /usr/sbin/cnid_dbd /etc/netatalk /var/lib/netatalk /var/lock/netatalk /run/lock/netatalk /var/log/netatalk.log /var/log/netatalk; do \
    if [ -e "$path" ]; then \
        restorecon -R "$path" || exit 1; \
    fi; \
done; \

%define selinux_policyver 41.44-1

Name: netatalk_selinux
Version: 1.1.0
Release: 1%{?dist}
Summary: SELinux policy module for Netatalk with SQLite CNID

License: GPLv2+
URL: https://netatalk.io/
Source0: netatalk.pp
Source1: netatalk.if
Source2: netatalk_selinux.8

Requires: policycoreutils, policycoreutils-python-utils, libselinux-utils
Requires(post): selinux-policy-base >= %{selinux_policyver}, policycoreutils, libselinux-utils
Requires(postun): policycoreutils, libselinux-utils
BuildArch: noarch

%description
This package confines Netatalk using SQLite CNID and CNID Spotlight.
The deprecated cnid_metad and cnid_dbd daemons and the Localsearch
private session bus are excluded from the supported configuration.

%install
install -d %{buildroot}%{_datadir}/selinux/packages
install -m 644 %{SOURCE0} %{buildroot}%{_datadir}/selinux/packages/
install -d %{buildroot}%{_datadir}/selinux/devel/include/contrib
install -m 644 %{SOURCE1} %{buildroot}%{_datadir}/selinux/devel/include/contrib/
install -d %{buildroot}%{_mandir}/man8
install -m 644 %{SOURCE2} %{buildroot}%{_mandir}/man8/netatalk_selinux.8

%post
semodule -n -i %{_datadir}/selinux/packages/netatalk.pp || exit 1
if selinuxenabled; then
    load_policy || exit 1
    %relabel_files
fi
exit 0

%postun
if [ "$1" -eq 0 ]; then
    semodule -n -r netatalk || exit 1
    if selinuxenabled; then
        load_policy || exit 1
        %relabel_files
    fi
fi
exit 0

%files
%attr(0644,root,root) %{_datadir}/selinux/packages/netatalk.pp
%{_datadir}/selinux/devel/include/contrib/netatalk.if
%{_mandir}/man8/netatalk_selinux.8*

%changelog
* Sat Sep 19 2026 Daniel Markstedt <daniel@mindani.net> 1.1-1
- Confine SQLite CNID backend and CNID Spotlight backend
- Exclude deprecated daemons and Localsearch Spotlight backend
- Relabel all policy paths on installation and removal
- Added --build and --audit modes to the helper script
- Tightened permissions and path labeling to improve policy enforcement
- Installation and removal now fail clearly when policy loading
  or relabeling fails

* Fri Jun 27 2025 Daniel Markstedt <daniel@mindani.net> 1.0-1
- Initial version
