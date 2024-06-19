# Quick start

Netatalk can be built by using Meson or GNU Autotools.
The meson build system is the recommended build system for the main
branch, autotools support may be be removed at some point in the future.

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
and https://mesonbuild.com/Builtin-options.html for details of generic Meson
options.

## External software dependencies

Required:

  - libgcrypt version 1.2.3 or later (for DHX2 UAM support)
  - Berkeley DB version 4.6.0 or later

Optional:

  - avahi or mDNSresponder  (for Zeroconf support)
  - cracklib                (for cracklib support)
  - cups                    (for CUPS support)
  - Docbook XSL stylesheets (for manpages & manual documentation)
  - Kerberos V              (for krbV UAM support)
  - libacl                  (for ACL support)
  - libldap                 (for LDAP support)
  - libpam                  (for PAM support)
  - libressl or OpenSSL@1.1 (for Randnum and DHX UAM support.
                             Built-in SSL supportis also available)
  - libtirpc / libquota     (for Quota support)
  - perl                    (for various scripts)
  - tcpwrap                 (for TCP wrapper support)
  - xsltproc                (for manpages and mananual documentation)

More details about dependencies can be found in the documentation at
https://netatalk.io/oldstable/htmldocs/installation

# Compiling with autotools (deprecated)

The rest of this document contains the GNU autotools install
instructions....

## Basic Installation


   The `configure` shell script attempts to guess correct values for
various system-dependent variables used during compilation.  It uses
those values to create a `Makefile` in each directory of the package.
It may also create one or more `.h` files containing system-dependent
definitions.  Finally, it creates a shell script `config.status` that
you can run in the future to recreate the current configuration, a file
`config.cache` that saves the results of its tests to speed up
reconfiguring, and a file `config.log` containing compiler output
(useful mainly for debugging `configure`).

   The simplest way to compile this package is:

  1. `cd` to the directory containing the package's source code and type
     './bootstrap' to create the configure file for your system. Then
     type `./configure` to configure the package.

     Running `configure` takes awhile.  While running, it prints some
     messages telling which features it is checking for.

  2. Type `make` to compile the package.

  3. Optionally, type `make check` to run any self-tests that come with
     the package.

  4. Type `make install` to install the programs and any data files and
     documentation.

  5. You can remove the program binaries and object files from the
     source code directory by typing `make clean`.  To also remove the
     files that `configure` created (so you can compile the package for
     a different kind of computer), type `make distclean`.

## Compilers and Options

   Some systems require unusual options for compilation or linking that
the `configure` script does not know about.  You can give `configure`
initial values for variables by setting them in the environment.  Using
a Bourne-compatible shell, you can do that on the command line like
this:

```
CC=gcc CFLAGS=-O2 LIBS=-lposix ./configure
```

Or on systems that have the `env` program, you can do it like this:

```
env CPPFLAGS=-I/usr/local/include LDFLAGS=-L/usr/local/lib ./configure
```

## Installation Names

   By default, `make install` will install the package's files in
`/usr/local/bin`, `/usr/local/man`, etc.  You can specify an
installation prefix other than `/usr/local` by giving `configure` the
option `--prefix=PATH`.

   You can specify separate installation prefixes for
architecture-specific files and architecture-independent files.  If you
give `configure` the option `--exec-prefix=PATH`, the package will use
PATH as the prefix for installing programs and libraries.
Documentation and other data files will still use the regular prefix.

   In addition, if you use an unusual directory layout you can give
options like `--bindir=PATH` to specify different values for particular
kinds of files.  Run `configure --help` for a list of the directories
you can set and what kinds of files go in them.

## Optional Features

   Netatalk has `--enable-FEATURE` options to pass to `configure`, where
FEATURE indicates an optional part of the package. Run `configure --help`
to see which configuration options are available.
