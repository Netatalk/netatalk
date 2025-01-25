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

Please see `meson_options.txt` for full details of all Netatalk-specific options,
and the [Meson documentation](https://mesonbuild.com/Builtin-options.html)
for details of generic Meson options.

## External software dependencies

Required:

  - Berkeley DB version 4.6.0 or later (often packaged as `bdb` or sometimes `db`)
  - libevent version 2
  - libgcrypt version 1.2.3 or later
  - meson version 0.61.2 or later
  - ninja (often packaged as `ninja-build`)

Required for Spotlight support:

  - D-Bus (also used by avahi and afpstats)
  - talloc
  - tracker version 0.12 or later; or tinysparql and localsearch version 3.8.0 or later
  - bison
  - flex

Optional:

  - avahi or mDNSresponder           (for Zeroconf support)
  - cmark or cmark-gfm               (for generating documentation)
  - cracklib and cracklib dictionary (for password strength check in afppasswd)
  - GLib 2 and GIO                   (for afpstats support)
  - Kerberos V                       (for krbV UAM support)
  - libacl                           (for ACL support)
  - libldap                          (for LDAP support)
  - libpam                           (for PAM support)
  - libtirpc or libquota             (for Quota support)
  - Perl                             (for admin scripts)
  - tcpwrap                          (for TCP wrapper support)
  - [UnicodeData.txt](https://www.unicode.org/Public/UNIDATA/UnicodeData.txt)                  (for regenerating Unicode lookup tables)

More details about dependencies can be found in the
[Installation chapter](https://netatalk.io/stable/htmldocs/installation)
of the Netatalk manual.
