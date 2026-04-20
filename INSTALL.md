# Netatalk Installation Quick Start

This guide covers installing Netatalk from source on UNIX-like operating systems.

Netatalk uses the [Meson](https://mesonbuild.com/) build system with
[Ninja](https://ninja-build.org/) as the backend.

Meson only supports out-of-tree builds, and must be passed a directory to put
built and generated sources into. We'll call that directory "build" here. It's
recommended to create a separate build directory for each configuration you
might want to use.

## Get the source code

To build from a release tarball, download and unpack the tarball.

```shell
tar xjf netatalk-*.tar.xz                               # unpack the sources
cd netatalk-*                                           # change to the toplevel directory
```

To build from the Netatalk GitHub repo, make sure you have Git installed,
then clone the source tree.

```shell
git clone https://github.com/netatalk/netatalk.git      # clone the repository
cd netatalk                                             # change to the repo directory
```

## Install software dependencies

At this point, you need to install the software dependencies for building
and running Netatalk.

This list is not exhaustive, and you may need to install additional software
depending on your platform and configuration. See the
[Compilation](https://netatalk.io/compilation)
documentation for more details.

### Required

These are the libraries that are hard requirements for netatalk.

| Package      | Details |
|--------------|---------|
| bstring      | built as subproject when shared library not found |
| iniparser    | v3.1 or later |
| libevent2 **OR** libev |  |
| libgcrypt    | v1.2.3 or later |

### CNID Backend Requirements

At least one CNID backend is required for netatalk to function.

| Package      | Backend | Details |
|--------------|---------|---------|
| Berkeley DB  | dbd     | v4.6.0 or later (often packaged as `bdb` or sometimes `db`) |
| mysql-client **OR** mariadb-client | mysql |  |
| sqlite3      | sqlite  |  |

### Required for Build Environment

In order to build netatalk from source code, the following components are
required at the bare minimum.

| Package    | Details |
|------------|---------|
| C compiler | clang or gcc are recommended |
| meson      | v0.61.2 or later |
| ninja      | often packaged as `ninja-build` |

### Optional Spotlight Support

| Package    | Details |
|------------|---------|
| bison      | or compatible Yacc parser |
| D-Bus      | also used by avahi and afpstats |
| DConf      | sometimes packaged as `dconf-cli` |
| GLib 2     | gsettings is used to determine the localsearch schema |
| flex       | or compatible lexer |
| localsearch **OR** tracker | v3.0 or later |
| talloc     |  |
| tinysparql |  |

### Optional Features

| Package      | Details |
|--------------|---------|
| avahi **OR** mDNSresponder | Zeroconf support |
| cmark **OR** cmark-gfm **OR** pandoc | generating documentation |
| cracklib and cracklib dictionary | password strength check in afppasswd |
| D-Bus daemon               | afpstats support (also transient dependency for avahi and localsearch) |
| GLib 2 and GIO             | afpstats support |
| Kerberos V                 | krbV UAM support |
| libacl                     | ACL support |
| libldap                    | LDAP support |
| libpam                     | PAM support |
| libtirpc **OR** libquota   | Quota support |
| Perl                       | admin scripts |
| po4a                       | localization of documentation |
| tcpwrap                    | TCP wrapper support |
| [UnicodeData.txt](https://www.unicode.org/Public/UNIDATA/UnicodeData.txt) | regenerating Unicode lookup tables |

## Build the software

Use the `meson` command to compile and install Netatalk.

```shell
meson setup build                                       # configure the build
meson compile -C build                                  # build Netatalk

# Become root and install

sudo meson install -C build                             # install Netatalk
```

To uninstall Netatalk:

```shell
sudo ninja -C build uninstall                           # uninstall Netatalk
```

To test (requires the `-Dwith-tests=true` flag at configure time):

```shell
cd build && meson test
```

## Configuration flags

When using Meson, to review the options which Meson chose, run:

```shell
meson configure
```

With additional arguments the `meson setup build` command can be used to
configure Netatalk according to user preference. All generic options passed to
this command take the form `-Doption=value`.

For example, to install Netatalk in a specific location:

```shell
meson setup build -Dprefix=/tmp/install
```

By default meson enables all Netatalk features if their dependencies are available.
However, many features can be enabled or disabled at configure time.

If meson cannot find the some required dependencies at configure time then the
user can also specify the path a library if it is installed in an unconventional
location.

Please see [meson_options.txt](https://github.com/Netatalk/netatalk/blob/main/meson_options.txt)
for full details of all Netatalk-specific options,
and the [Meson documentation](https://mesonbuild.com/Builtin-options.html)
for details of generic Meson options.

## See also

The Netatalk manual has further resources to aid building and installation.

- [Installation chapter](https://netatalk.io/manual/en/Installation): detailed descriptions of each dependency
- [Compilation chapter](https://netatalk.io/compilation): concrete build steps for each supported operating system
