# Compile Netatalk from Source

Below are instructions on how to compile Netatalk from source for specific operating systems.
Before starting, please read through the [Install Quick Start](https://netatalk.io/install) guide first.
You need to have a local clone of Netatalk's source code before proceeding.

Please note that these steps are automatically generated from the CI jobs,
and may not always be optimized for standalone execution.

## Alpine Linux

Install dependencies

```
apk add acl-dev avahi-compat-libdns_sd avahi-dev bison build-base cracklib cracklib-dev cracklib-words cups cups-dev curl db-dev dbus-dev flex gcc iniparser-dev krb5-dev libevent-dev libgcrypt-dev libtirpc-dev libtracker linux-pam-dev localsearch mariadb-dev meson ninja openldap-dev openrc pandoc perl pkgconfig rpcsvc-proto-dev talloc-dev tinysparql-dev
```

Configure

```
meson setup build -Dbuildtype=release -Dwith-appletalk=true -Dwith-cups-pap-backend=true -Dwith-dbus-sysconf-path=/usr/share/dbus-1/system.d -Dwith-tests=true -Dwith-testsuite=true
```

Build

```
meson compile -C build
```

Run integration tests

```
cd build && meson test && cd ..
```

Install

```
meson install -C build
```

Check netatalk capabilities

```
netatalk -V
afpd -V
```

Uninstall

```
ninja -C build uninstall
```

## Arch Linux

Install dependencies

```
pacman -Sy --noconfirm avahi bison cmark-gfm cracklib cups db flex gcc iniparser localsearch mariadb-clients meson ninja perl pkgconfig rpcsvc-proto talloc tinysparql
```

Configure

```
meson setup build -Dbuildtype=release -Dwith-appletalk=true -Dwith-cups-pap-backend=true -Dwith-dbus-sysconf-path=/usr/share/dbus-1/system.d -Dwith-init-hooks=false -Dwith-tests=true -Dwith-testsuite=true
```

Build

```
meson compile -C build
```

Run integration tests

```
cd build && meson test && cd ..
```

Install

```
meson install -C build
```

Check netatalk capabilities

```
netatalk -V
afpd -V
```

Uninstall

```
ninja -C build uninstall
```

## Debian Linux

Install dependencies

```
apt-get update
apt-get install --assume-yes --no-install-recommends bison cmark-gfm cracklib-runtime file flex libacl1-dev libavahi-client-dev libcrack2-dev libcups2-dev libdb-dev libdbus-1-dev libevent-dev libgcrypt20-dev libglib2.0-dev libiniparser-dev libkrb5-dev libldap2-dev libmariadb-dev libpam0g-dev libtalloc-dev libtirpc-dev libtracker-sparql-3.0-dev libwrap0-dev meson ninja-build po4a quota systemtap-sdt-dev tcpd tracker tracker-miner-fs
```

Configure

```
meson setup build -Dbuildtype=release -Dwith-appletalk=true -Dwith-cups-pap-backend=true -Dwith-dbus-sysconf-path=/usr/share/dbus-1/system.d -Dwith-docs-l10n=true -Dwith-init-hooks=false -Dwith-init-style=debian-sysv,systemd -Dwith-pkgconfdir-path=/etc/netatalk -Dwith-tests=true -Dwith-testsuite=true
```

Build

```
meson compile -C build
```

Run integration tests

```
cd build && meson test && cd ..
```

Install

```
meson install -C build
```

Check netatalk capabilities

```
netatalk -V
afpd -V
```

Uninstall

```
ninja -C build uninstall
```

## Fedora Linux

Install dependencies

```
dnf --setopt=install_weak_deps=False --assumeyes install avahi-devel bison chkconfig cracklib-devel cups-devel dbus-devel flex glib2-devel iniparser-devel krb5-devel libacl-devel libdb-devel libgcrypt-devel libtalloc-devel mariadb-connector-c-devel meson ninja-build openldap-devel pam-devel pandoc perl perl-Net-DBus quota-devel systemd systemtap-sdt-devel tracker tracker-devel
```

Configure

```
meson setup build -Dbuildtype=release -Dwith-appletalk=true -Dwith-cups-pap-backend=true -Dwith-dbus-sysconf-path=/usr/share/dbus-1/system.d -Dwith-init-hooks=false -Dwith-tests=true -Dwith-testsuite=true
```

Build

```
meson compile -C build
```

Run integration tests

```
cd build && meson test && cd ..
```

Install

```
sudo meson install -C build
```

Check netatalk capabilities

```
netatalk -V
afpd -V
```

Uninstall

```
sudo ninja -C build uninstall
```

## Ubuntu Linux

Install dependencies

```
sudo apt-get update
sudo apt-get install --assume-yes --no-install-recommends bison cmark-gfm cracklib-runtime file flex libacl1-dev libavahi-client-dev libcrack2-dev libcups2-dev libdb-dev libdbus-1-dev libevent-dev libgcrypt20-dev libglib2.0-dev libiniparser-dev libkrb5-dev libldap2-dev libmariadb-dev libpam0g-dev libtalloc-dev libtirpc-dev libtracker-sparql-3.0-dev libwrap0-dev meson ninja-build quota systemtap-sdt-dev tcpd tracker tracker-miner-fs
```

Configure

```
meson setup build -Dbuildtype=release -Dwith-appletalk=true -Dwith-cups-pap-backend=true -Dwith-dbus-sysconf-path=/usr/share/dbus-1/system.d -Dwith-init-hooks=false -Dwith-tests=true -Dwith-testsuite=true
```

Build

```
meson compile -C build
```

Run distribution tests

```
cd build && meson dist && cd ..
```

Install

```
sudo meson install -C build
```

Check netatalk capabilities

```
netatalk -V
afpd -V
```

Start netatalk

```
sudo systemctl start netatalk
sleep 1
asip-status localhost
```

Stop netatalk

```
sudo systemctl stop netatalk
```

Uninstall

```
sudo ninja -C build uninstall
```

## macOS

Install dependencies

```
brew update
brew install cmark-gfm cracklib iniparser mariadb meson openldap
```

Configure

```
meson setup build -Dbuildtype=release -Dwith-tests=true -Dwith-testsuite=true
```

Build

```
meson compile -C build
```

Run integration tests

```
cd build && meson test && cd ..
```

Install

```
sudo meson install -C build
```

Check netatalk capabilities

```
netatalk -V
afpd -V
```

Start netatalk

```
sudo netatalkd start
sleep 1
asip-status localhost
```

Stop netatalk

```
sudo netatalkd stop
```

Uninstall

```
sudo ninja -C build uninstall
```

## DragonflyBSD

Install required packages

```
pkg install -y avahi bison cmark db5 iniparser libevent libgcrypt meson mysql80-client openldap26-client perl5 pkgconf py39-gdbm py39-sqlite3 py39-tkinter talloc tracker3
```

Configure, compile, install, run, and uninstall

```
set -e
meson setup build -Dbuildtype=release -Dwith-appletalk=true -Dwith-tests=true -Dwith-testsuite=true
meson compile -C build
meson install -C build
netatalk -V
afpd -V
ninja -C build uninstall
```

## FreeBSD

Install required packages

```
pkg install -y avahi bison cmark db5 flex iniparser libevent libgcrypt localsearch meson mysql91-client openldap26-client p5-Net-DBus perl5 pkgconf talloc
```

Configure, compile, install, run, and uninstall

```
set -e
meson setup build -Dbuildtype=release -Dpkg_config_path=/usr/local/libdata/pkgconfig -Dwith-tests=true -Dwith-testsuite=true
meson compile -C build
cd build
meson test
cd ..
meson install -C build
netatalk -V
afpd -V
/usr/local/etc/rc.d/netatalk start
sleep 1
asip-status localhost
/usr/local/etc/rc.d/netatalk stop
/usr/local/etc/rc.d/netatalk disable
ninja -C build uninstall
```

## NetBSD

Install required packages

```
export PKG_PATH="http://ftp.NetBSD.org/pub/pkgsrc/packages/NetBSD/$(uname -p)/$(uname -r|cut -f '1 2' -d.)/All/"
pkg_add bison cmark db5 flex gcc13 gnome-tracker heimdal iniparser libcups libevent libgcrypt meson mysql-client p5-Net-DBus perl pkg-config talloc
```

Configure, compile, install, run, and uninstall

```
set -e
meson setup build -Dbuildtype=release -Dwith-appletalk=true -Dwith-cups-pap-backend=true -Dwith-dtrace=false -Dwith-tests=true -Dwith-testsuite=true
meson compile -C build
cd build
meson test
cd ..
meson install -C build
netatalk -V
afpd -V
service netatalk onestart
sleep 1
asip-status localhost
service netatalk onestop
ninja -C build uninstall
```

## OpenBSD

Install required packages

```
pkg_add -I avahi bison cmark db-4.6.21p7v0 dbus gcc-11.2.0p14 heimdal iniparser libevent libgcrypt libtalloc mariadb-client meson openldap-client-2.6.8v0 p5-Net-DBus pkgconf tracker3
```

Configure, compile, install, run, and uninstall

```
set -e
meson setup build -Dbuildtype=release -Dpkg_config_path=/usr/local/lib/pkgconfig -Dwith-gssapi-path=/usr/local/heimdal -Dwith-kerberos-path=/usr/local/heimdal -Dwith-pam=false -Dwith-tests=true -Dwith-testsuite=true
meson compile -C build
meson install -C build
netatalk -V
afpd -V
rcctl -d start netatalk
sleep 1
asip-status localhost
rcctl -d stop netatalk
rcctl -d disable netatalk
ninja -C build uninstall
```

## OmniOS

Install required packages

```
pkg install build-essential pkg-config
curl -O https://pkgsrc.smartos.org/packages/SmartOS/bootstrap/bootstrap-trunk-x86_64-20240116.tar.gz
tar -zxpf bootstrap-trunk-x86_64-20240116.tar.gz -C /
export PATH=/opt/local/sbin:/opt/local/bin:/usr/gnu/bin:/usr/bin:/usr/sbin:/sbin:$PATH
pkgin -y install avahi cmark gnome-tracker iniparser libevent libgcrypt meson mysql-client talloc
```

Configure, compile, install, run, and uninstall

```
set -e
export PATH=/opt/local/sbin:/opt/local/bin:/usr/gnu/bin:/usr/bin:/usr/sbin:/sbin:$PATH
meson setup build --prefix=/opt/local -Dbuildtype=release -Dpkg_config_path=/opt/local/lib/pkgconfig -Dwith-dbus-sysconf-path=/usr/share/dbus-1/system.d -Dwith-ldap-path=/opt/local -Dwith-tests=true -Dwith-testsuite=true
meson compile -C build
cd build
meson test
cd ..
meson install -C build
netatalk -V
afpd -V
sleep 1
svcadm enable svc:/network/netatalk:default
sleep 1
asip-status localhost
svcadm disable svc:/network/netatalk:default
ninja -C build uninstall
```

## Solaris

Install required packages

```
pkg install bison cmake flex gcc libevent libgcrypt ninja pkg-config python/pip
pip install meson
curl -O https://gitlab.com/iniparser/iniparser/-/archive/v4.2.5/iniparser-v4.2.5.tar.gz
tar xzf iniparser-v4.2.5.tar.gz
cd iniparser-v4.2.5
mkdir build
cd build
cmake ..
make all
make install
```

Configure, compile, install, run, and uninstall

```
set -e
export PATH=/usr/local/sbin:/usr/local/bin:$PATH
meson setup build --prefix=/usr/local -Dbuildtype=release -Dpkg_config_path=/usr/lib/amd64/pkgconfig -Dwith-dbus-sysconf-path=/usr/share/dbus-1/system.d -Dwith-iniparser-path=/usr/local -Dwith-tests=true -Dwith-testsuite=true
meson compile -C build
cd build
meson test
cd ..
meson install -C build
netatalk -V
afpd -V
sleep 1
svcadm -v enable svc:/network/netatalk:default
sleep 1
asip-status localhost
svcadm -v disable svc:/network/netatalk:default
ninja -C build uninstall
```
