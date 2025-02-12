# Installation

> **WARNING**

> Before upgrading to Netatalk 4 from an earlier version, please read the
[Upgrade](Upgrading.html) chapter in this manual.

## How to obtain Netatalk

Please have a look at the [Netatalk homepage](https://netatalk.io) for
the most up to date information on where to find the latest version of
the software.

### Binary packages

Binary packages of Netatalk are included in the package repositories of
some Linux, BSD, and Solaris-like distributions. Installing Netatalk
through this channel will give you the most seamless experience, with
managed updates when new package versions are available.

You might also want to have a look at 3rd party package repositories for
your operating system, such as [rpmfind](https://rpmfind.net/) for Red
Hat based Linux distributions, [OpenCSW](https://www.opencsw.org/) for
Solaris and its descendants, and [Homebrew](https://brew.sh/) or
[MacPorts](https://www.macports.org/) for macOS.

### Source packages

#### Tarballs

Prepackaged tarballs with stable releases of the Netatalk source code
are available on the [Netatalk releases page on
GitHub](https://github.com/Netatalk/netatalk/releases).

#### Git

Downloading the Git repository can be done quickly and easily:

1.  Make sure you have Git installed. **which git** should produce a path
    to git.

        $ which git
        /usr/bin/git

2.  Now get the source:

        $ git clone https://github.com/Netatalk/netatalk.git netatalk-code
        Cloning into 'netatalk-code'...
        remote: Enumerating objects: 41592, done.
        ...
        Resolving deltas: 100% (32227/32227), done.

    This will create a local directory called *netatalk-code* containing
    a complete and fresh copy of the whole Netatalk source tree from the
    Git repository.

3.  If you don't specify a branch or tag, you will get the bleeding edge
    development code. In order to get the latest stable Netatalk 3.1
    code, for instance, check out the branch named
    "branch-netatalk-3-1":

        $ git checkout branch-netatalk-3-1

4.  In order to keep your repository copy updated, occasionally run:

        $ git pull

## Compiling Netatalk

### Prerequisites

#### Required third-party software

- Berkeley DB

  The default dbd CNID backend for netatalk uses Berkeley DB to store
  unique file identifiers. At the time of writing you need at least
  version 4.6.

  The recommended version is 5.3, the final release under the permissive
  Sleepycat license, and therefore the most widely distributed version.

- iniparser

  The iniparser library is used to parse the configuration files.
  At least version 3.0 is required, while 4.0 or later is recommended.

- libevent

  Internal event callbacks in the netatalk service controller daemon are
  built on libevent version 2.

- Libgcrypt

  The [Libgcrypt](https://gnupg.org/software/libgcrypt/) library
  supplies the encryption for the standard User Authentication Modules
  (UAMs). They are: DHX2, DHCAST128 (a.k.a. DHX) and RandNum.

#### Optional third-party software

Netatalk can use the following third-party software to enhance its
functionality.

- ACL and LDAP

  LDAP is an open and industry-standard user directory protocol that
  works in tandem with the advanced permissions scheme of ACL. On some
  operating systems ACL and LDAP libraries are built in to the system,
  while on others you have to install supporting packages to enable this
  functionality.

- Avahi or mDNSresponder for Bonjour

  Mac OS X 10.2 and later uses Bonjour (a.k.a. Zeroconf) for automatic
  service discovery. Netatalk can advertise AFP file sharing and Time
  Machine volumes by using Avahi or mDNSResponder.

  When using Avahi, D-Bus is also required, and the Avahi library must
  have been built with D-Bus support.

- cmark or cmark-gfm

  Netatalk's documentation is authored in Markdown format.
  The man page sources consist of standards-compliant CommonMark,
  while the rest of the documentation is authored in GitHub-Flavored
  Markdown.

  You can use cmark to generate roff man pages from the Markdown sources,
  and cmark-gfm to generate all documentation, including HTML pages.
  If you have access to the latter, you don't need the former.

- CrackLib

  When using the Random Number UAMs and netatalk's own **afppasswd**
  password manager, CrackLib can help protect against setting weak
  passwords for authentication with netatalk.

  The CrackLib dictionary, which is sometimes distributed separately in
  a runtime package, is also a requirement.

- D-Bus

  D-Bus provides a mechanism for sending messages between processes,
  used by multiple Netatalk features: Spotlight, Zeroconf with Avahi,
  and the **afpstats** tool.

- GLib and GIO

  Used by the **afpstats** tool to interface with D-Bus.

- iconv

  iconv provides conversion routines for many character encodings.
  Netatalk uses it to provide charsets it does not have built in
  conversions for, like ISO-8859-1. On glibc systems, Netatalk can use
  the glibc provided iconv implementation. Otherwise you can use the GNU
  libiconv implementation.

- Kerberos V

  Kerberos v5 is a client-server based authentication protocol invented
  at the Massachusetts Institute of Technology. With the Kerberos
  library, netatalk can produce the GSS UAM library for authentication
  with existing Kerberos infrastructure.

- MySQL or MariaDB

  By leveraging a MySQL-compatible client library, netatalk can be built
  with a MySQL CNID backend that is highly scalable and reliable. The
  administrator has to provide a separate database instance for use with
  this backend.

- PAM

  PAM provides a flexible mechanism for authenticating users. PAM was
  invented by SUN Microsystems. Linux-PAM
  is a suite of shared libraries that enable the local system
  administrator to choose how applications authenticate users.

- Perl

  Netatalk's administrative utility scripts rely on the Perl runtime,
  version 5.8 or later. The required Perl modules include
  *IO::Socket::IP* (asip-status) and *Net::DBus* (afpstats).

- po4a

  With the help of po4a, Netatalk's documentation can be translated
  into other languages. It uses gettext to extract translatable
  strings from source files and merge them with the translations
  stored in PO files.

- TCP wrappers

  Wietse Venema's network logger, also known as TCPD or LOG_TCP.

  Security options are: access control per host, domain and/or service;
  detection of host name spoofing or host address spoofing; booby traps
  to implement an early-warning system.

- Tracker, or TinySPARQL / LocalSearch

  Netatalk uses [Tracker](https://tracker.gnome.org) or its later
  incarnation
  TinySPARQL/[LocalSearch](https://gnome.pages.gitlab.gnome.org/localsearch/)
  as the metadata backend for Spotlight
  search indexing. The minimum required version is 0.7 as this was the
  first version to support
  [SPARQL](https://gnome.pages.gitlab.gnome.org/tracker/).

- talloc / bison / flex

  Samba's talloc library, a Yacc parser such as bison, and a lexer like
  flex are also required for Spotlight.

- UnicodeData.txt

  The build system uses Perl and the Unicode Character Database to
  generate Netatalk's Unicode character conversion sources.

### Configure and build Netatalk

Instructions on how to use the build system to configure and build
netatalk source code are documented in the
[Install Quick Start](/install.html) guide.

For examples of concrete steps to compile on specific operating systems,
refer to the [Compile Netatalk from Source](Compilation.html) appendix in this
manual, which is automatically generated from the CI build scripts.

## Starting and stopping Netatalk

The Netatalk distribution comes with several operating system specific
startup script templates that are tailored according to the options
given to the build system before compiling. Currently, templates are
provided for systemd, openrc, in addition to platform specific scripts
for popular Linux distributions, BSD variants, Solaris descendants, and
macOS.

When building from source, the Netatalk build system will attempt to
detect which init style is appropriate for your platform. You can also
configure the build system to install the specific type of startup
script(s)  you want by specifying the
**with-init-style** option. For the syntax, please refer to the build
system's help text.

Since new Linux, \*BSD, and Solaris-like distributions appear at regular
intervals, and the startup procedure for the other systems mentioned
above might change as well, it is a good idea to not blindly install a
startup script but to confirm first that it will work on your system.

If you use Netatalk as part of a fixed setup, like a Linux distribution,
an RPM or a BSD package, things will probably have been arranged
properly for you. The previous paragraphs therefore apply mostly for
people who have compiled Netatalk themselves.

The following daemon need to be started by whatever startup script
mechanism is used:

- netatalk

In the absence of a startup script, you can also launch this daemon
directly (as root), and kill it with SIGTERM when you are done with it.

Additionally, make sure that the configuration file *afp.conf* is in the
right place. You can inquire netatalk where it is expecting the file to
be by running the **netatalk -V** command.

If you want to run AppleTalk services, you also need to start the
**atalkd** daemon, plus the optional **papd**, **timelord**, and **a2boot**
daemons. See the [AppleTalk](AppleTalk.html) chapter in this manual for more
information.
