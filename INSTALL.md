# Quick start

Netatalk uses the Meson build system with Ninja as the backend.

Meson only supports out-of-tree builds, and must be passed a directory to put
built and generated sources into. We'll call that directory "build" here. It's
recommended to create a separate build directory for each configuration you
might want to use.

To build from a release tarball:

```
tar xjf netatalk-*.tar.xz                               # unpack the sources
cd netatalk-*                                           # change to the toplevel directory
```

To build from the Netatalk GitHub repo:

```
git clone https://github.com/netatalk/netatalk.git      # clone the repository
cd netatalk                                             # change to the repo directory
```

Install required software dependencies:

See the following section of this file for a list of required and optional software dependencies.

Then:

```
meson setup build                                       # configure the build
meson compile -C build                                  # build Netatalk

# Become root and install

sudo meson install -C build                             # install Netatalk
```

To uninstall Netatalk:

```
sudo ninja -C build uninstall                           # install Netatalk
```

To test (requires the `-Dwith-tests=true` flag at configure time):

```
cd build && meson test
```

## Configuration flags

When using Meson, to review the options which Meson chose, run:

```
meson configure
```

With additional arguments the `meson setup build` command can be used to
configure Netatalk according to user preference. All generic options passed to
this command take the form `-Doption=value`.

For example, to install Netatalk in a specific location:

```
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

## External software dependencies

### Required

| Package      | Details |
|--------------|---------|
| Berkeley DB  | v4.6.0 or later (often packaged as `bdb` or sometimes `db`) |
| iniparser    | v3.1 or later |
| libevent     | v2.0 or later |
| libgcrypt    | v1.2.3 or later |

### Required to Build

| Package | Details |
|---------|---------|
| meson   | v0.61.2 or later |
| ninja   | Often packaged as `ninja-build` |

### Required for Spotlight Support

| Package    | Details |
|------------|---------|
| D-Bus      | Also used by avahi and afpstats |
| talloc     |  |
| tracker **OR** localsearch | v0.12 or later|
| bison      |  |
| flex       |  |

### Optional

| Package      | Details |
|--------------|---------|
| avahi **OR** mDNSresponder | For Zeroconf support |
| cmark **OR** cmark-gfm     | For generating documentation |
| cracklib and cracklib dictionary | For password strength check in afppasswd |
| GLib 2 and GIO             | For afpstats support |
| Kerberos V                 | For krbV UAM support |
| libacl                     | For ACL support |
| libldap                    | For LDAP support |
| libpam                     | For PAM support |
| libtirpc **OR** libquota   | For Quota support |
| Perl                       | For admin scripts |
| po4a                       | For localization of documentation |
| tcpwrap                    | For TCP wrapper support |
| [UnicodeData.txt](https://www.unicode.org/Public/UNIDATA/UnicodeData.txt) | For regenerating Unicode lookup tables |

# See also

The Netatalk manual has further resources to aid building and installation.

- [Installation chapter](https://netatalk.io/manual/en/Installation): detailed descriptions of each dependency
- [Compilation chapter](https://netatalk.io/manual/en/Compilation): concrete build steps for each supported operating system
