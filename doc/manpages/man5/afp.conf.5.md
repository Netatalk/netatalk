# Name

afp.conf - Netatalk configuration file

# Synopsis

The **afp.conf** file is the configuration file for the **Netatalk** AFP
file server. It holds all AFP-specific configuration and all AFP volume
definitions.

# File Format

The file consists of sections and parameters. A section begins with its
name in square brackets and continues until the next section begins.
Sections contain parameters of the form:

    option = value

The file is line-based: each newline-terminated line is a comment, a
section name, or a parameter.

Parameter names are case sensitive, while section names are not.

The parser splits a parameter at its first equals sign and discards the
whitespace around that sign. Leading, trailing, and internal whitespace
in section and parameter names is irrelevant. The parser discards
leading and trailing whitespace in a parameter value and keeps internal
whitespace verbatim.

The parser ignores lines that begin with a semicolon or a hash character
and lines that contain only whitespace.

A line that ends in a **\\** (backslash) continues on the next line.

A parameter value is a string (no quotes needed), a boolean, or a
number, as for **file perm**. Accepted boolean values are yes/no
(preferred), y/n, true/false, t/f, on/off, enabled/disabled, and 1/0, in
upper or lower case. The server logs a warning for any other boolean
value and applies the option's default. String values keep their case.

# Section Descriptions

Each section other than [Global] describes a volume: a directory to
which the server gives access, plus the access rights it grants to the
user.

[Global] and [Homes] are special sections, described under *Special
Sections*. The following notes apply to ordinary section descriptions.

For volumes the **path** option must specify the directory to share.
The **volume name** option sets the name of the volume.

A volume section without a **path** option is a *vol preset* (volume
preset). Other volume sections select it with the **vol preset** option,
and it supplies their default options. When both a preset and a volume
section set an option, the volume section setting replaces the preset
option. At every configuration load, the server logs an error that the
preset section has no path and skips it as a volume. Its options still
work as the preset.

The server grants no more access than the host system grants the
specified or guest UNIX user.

The following sample section defines an AFP volume. The user has full
access to the path */foo/bar*. Clients see the volume as *Baz*:

    [baz]
        volume name = Baz
        path = /foo/bar

# Special Sections

## The [Global] section

Parameters in [Global] apply to the server as a whole. *Parameters*
below explains the **(G)** and **(V)** markers.

## The [Homes] section

The [Homes] section shares the home directories of the server's UNIX
users. Its one mandatory option is **basedir regex**. Set it to a path
that matches the parent directory of the user homes.

The optional **path** parameter shares only the named subdirectory of
each home directory instead of the entire home directory. With all user
home directories under */home*:

    [Homes]
        path = afp-data
        basedir regex = /home

For a user *john* this results in an AFP home volume with a path of
*/home/john/afp-data*.

If the path goes through a symlink, set the real path it resolves to.
When */home* links to */usr/home*:

    [Homes]
        basedir regex = /usr/home

The optional **home name** parameter changes the Homes volume name,
which is *$u's home* by default. *Variable Substitutions* below lists
the variables it accepts.

    [Homes]
        home name = The home of $u
        basedir regex = /home

For the same user *john* this results in an AFP home volume called
*The home of john*.

Any parameter marked **(H)** below can be used in the [Homes] section.

# Parameters

Each parameter carries a marker that says where it may be set. **(G)**
marks a parameter that goes only in [Global], where it applies to the
server as a whole. **(V)** marks a parameter set in a volume section. A
parameter marked both **(G)** and **(V)** may be set in [Global] as the
default for all volumes. A volume section, or the selected **vol
preset**, may override it.

# Variable Substitutions

You can use variables in volume names and paths. In a path, $b, $d, and
$v expand to nothing.

1. An unknown variable stays as typed, dollar sign included.

2. A known variable with no value expands to nothing.

The variables available for substitution are:

> $b
>
> > Basename of the volume path.
>
> $c
>
> > Client's IP address.
>
> $d
>
> > Volume pathname on the server.
>
> $f
>
> > Full name (the gecos field of the passwd file).
>
> $g
>
> > Group name.
>
> $h
>
> > Hostname.
>
> $i
>
> > Client's IP address, without port.
>
> $s
>
> > Server name (this can be the hostname).
>
> $u
>
> > User name (for a guest login, the guest account).
>
> $v
>
> > Volume name.
>
> $$
>
> > Prints a dollar sign ($).

# Explanation of Global Parameters

## Authentication Options

The options below control how the server authenticates users.

ad domain = *domain* **(G)**

> Appends @DOMAIN to the user name when authenticating. Useful in Active
Directory environments that otherwise would require the user to enter
the full user@domain string.

admin group = *group* **(G)**

> The server treats members of *group* as the superuser when they log in.
Unset by default.

force user = *user* **(G)**

> Makes every connecting user act as this UNIX user on the server. Use it
with care: an incorrect setting can cause security problems.

force group = *group* **(G)**

> Assigns this UNIX group as the default primary group for every user
connecting to the server.

guest account = *name* (default: *nobody*) **(G)**

> The UNIX account guests should use. It must exist on the host system.

k5 keytab = *path* **(G)**

k5 service = *service* **(G)**

k5 realm = *realm* **(G)**

> **k5 keytab** names the keytab file. With **k5 service**, **k5 realm**,
and **fqdn** all set, the Kerberos 5 UAM uses that service principal.
Otherwise it uses the first entry in the keytab.

nt domain = *domain* **(G)**

nt separator = *SEPARATOR* **(G)**

> The server prepends both strings to the login user name and tries to
authenticate the result through the active UAMs. Use with winbind
authentication, for example.

uam list = *uam list* (default: *uams_dhx2.so*) **(G)**

> Space or comma separated list of User Authentication Methods.
>
> Available UAMs are:
>
> uams_dhx2.so
>
> > Allows Diffie-Hellman eXchange 2 (DHX2) authentication. Provided by
uams_dhx2_pam.so or uams_dhx2_passwd.so.
>
> uams_srp.so
>
> > Allows SRP (Secure Remote Password) authentication. Requires a verifier
directory with one file per numeric uid: the default *afppasswd.srp*
directory or the one set with **srp verifier path**. A user owns an
active verifier file. A root-owned file is a disabled placeholder until
an administrator sets a password with **afppasswd -a**, as
**afppasswd**(1) describes.
>
> uams_gss.so
>
> > Allows Kerberos 5 authentication. Requires a Netatalk build with
Kerberos support and a properly configured Kerberos environment.
>
> uams_guest.so
>
> > Allows guest logins (INSECURE).
>
> uams_clrtxt.so
>
> > Allows logins with unencrypted passwords (INSECURE). Compatible with
Mac OS 9 and earlier. Provided by uams_pam.so or uams_passwd.so.
>
> uams_randnum.so
>
> > Allows Random Number and Two-Way Random Number Exchange authentication
(INSECURE). Requires a separate password file: the default *afppasswd*
file or the one set with **passwd file**, as **afppasswd**(1) describes.
>
> uams_dhx.so
>
> > Allows Diffie-Hellman eXchange (DHX) authentication. It is INSECURE.
Provided by uams_dhx_pam.so or uams_dhx_passwd.so.

uam path = *path* **(G)**

> The directory from which **afpd** loads the UAM modules.

valid shellcheck = *BOOLEAN* (default: *yes*) **(G)**

> Whether to check that the user's login shell is valid, that is, listed
in /etc/shells. Login fails for a user whose shell is not listed, such as
nologin. Set *no* to let such users log in.

## Charset Options

With Mac OS X, Apple introduced AFP3, which uses Unicode names encoded
as Decomposed UTF-8 (UTF8-MAC). Earlier AFP and OS versions used charsets
such as MacRoman and MacCentralEurope.

To serve AFP3 and older clients at the same time, **afpd** must convert
between UTF-8 and Mac charsets. Even macOS clients partly still rely on
the Mac charset. **afpd** cannot detect the charset a pre-AFP3 client
uses, so you must set it with the **mac charset** option. The option's
default should suit most western users.

**afpd** also interacts with the UNIX operating system, so it must
convert from UTF8-MAC or the Mac charset to the UNIX charset. You can
set the UNIX charset with the **unix charset** option. If you use
extended characters in the configuration files for **afpd**, make sure
your terminal matches the **unix charset**.

mac charset = *charset* (default: *MAC_ROMAN*) **(G)**/**(V)**

> Specifies the Mac clients' charset, for example *MAC_ROMAN*. **afpd**
uses it to convert strings and filenames to the clients' charset for
Mac OS 9 and Classic, for authentication and AFP messages (SIGUSR2
messaging).

unix charset = *charset* (default: *UTF8*) **(G)**

> Specifies the server's UNIX charset, for example *ISO-8859-15* or
*EUC-JP*. **afpd** uses it to convert strings to and from the system
locale, for example for authentication, server messages, and volume
names. The value *LOCALE* selects the system locale.

vol charset = *charset* **(G)**/**(V)**

> Specifies the charset of the volume's file names on disk. By default it
is the same as **unix charset**.
>
> ***NOTE:*** Keep the default UTF-8 encoding.

## Password Options

The options below control the password files and whether clients may
save or change passwords.

passwd file = *path* **(G)**

> Sets the path to the Random Number UAM's password file.

passwd minlen = *number* **(G)**

> Sets the minimum password length, if the UAM supports it.

save password = *BOOLEAN* (default: *yes*) **(G)**

> Whether clients may save passwords locally. With *no*, the server sends
this hint to clients, but honoring it is up to the client.

set password = *BOOLEAN* (default: *no*) **(G)**

> Whether clients may change their passwords, if the UAM in use supports
it.

srp verifier path = *path* **(G)**

> Sets the path to the SRP UAM verifier directory. The legacy name **srp
passwd file** still works as a deprecated alias and logs a warning. Its
value names a directory, not the old flat file.

## Network Options

The options below control the addresses and names the server advertises
and listens on.

advertise ssh = *BOOLEAN* (default: *no*) **(G)**

> Allows old Mac OS X clients (10.3.3-10.4) to establish a tunneled AFP
connection through SSH automatically. With *yes* and **fqdn** set, the
server adds an SSH tunnel entry to its FPGetSrvrInfo reply. The tunnel
depends on the client's settings and on a correctly configured, running
**sshd**(8) on the server.
>
> ***NOTE:*** Avoid this option: encrypting every AFP connection through
SSH increases the server's load significantly. Also, Apple's client-side
implementation of this feature in Mac OS X versions before 10.3.4
contained a security flaw.

afp interfaces = *name [name ...]* **(G)**

> Specifies the network interfaces that the server listens on. By default
the server advertises the system's first IP address and listens on all
addresses.
>
> ***NOTE:*** Do not use at the same time as the **afp listen** option.

afp listen = *ip address[:port] [ip address[:port] ...]* **(G)**

> Specifies the IP addresses that the server advertises and listens on.
By default the server advertises the system's first IP address and
listens on all addresses. Write the address in dotted-decimal format for
IPv4 or in hexadecimal format for IPv6.
>
> An IPv6 address with a port uses the URL format [IPv6]:port, with
square brackets around the address.
>
> ***NOTE:*** Do not use at the same time as the **afp interfaces** option.

afp port = *port number* (default: *548*) **(G)**

> Sets the TCP port used for AFP. It is also the default port for an
**afp listen** entry that specifies none.

fqdn = *name[:port]* **(G)**

> Specifies a fully qualified domain name, with an optional port, that
**afpd** advertises but does not automatically listen on. The server
discards a name it cannot resolve. AppleShare clients 3.8.3 and earlier
ignore this option. Unset by default. Use with caution: it adds a second
name resolution step on the client side.

hostname = *name* **(G)**

> The server resolves this name, instead of the system hostname, to the
IP address it advertises. It defaults to the system host name up to the
first period. The server does not use the name for listening. **afp
listen** can override it. To set the name clients see, use **server
name**.

zeroconf = *BOOLEAN* (default: *yes*) **(G)**

> Whether to advertise the server through Zeroconf (Bonjour), so Macs find
it without an address, when the build includes Avahi or mDNSResponder.

## Transfer Tuning

The options below tune how **afpd** moves file data to and from clients.

The **server quantum** should cover at least one bandwidth-delay product
(link speed times round-trip time) of the network path, or the ZFS
recordsize if larger. That is usually no more than 4 MiB. A
small quantum favors small-file transfers. A large quantum achieves
maximum throughput on large files at the expense of small ones, so tune
for the workload: larger is not always better. For file servers, the
recommendation is ZFS **recordsize** = **server quantum** = **splice
size**, with the default 1 MiB good for most use cases. The
**dsireadbuf** readahead should cover a few hundred milliseconds to one
second of inbound line rate. Raising it costs only memory.

server quantum = *number* (default: *1048576*) **(G)**

> The DSI server quantum in bytes: the file data carried by a single
write or read request, excluding frame overhead. The default is 1 MiB,
the maximum is 268435456 (256 MiB), and the minimum is 32000. Values
below the minimum fall back to the default. The server caps values above
the maximum at the maximum. The server rounds the session value up, by
at most 256 KiB, to a 4 KiB multiple whose write frame fills its final
TCP segment. It logs the result at session open, and the result never
falls below the configured value.

dsireadbuf = *number* (default: *32*) **(G)**

> Scale factor for the size of the DSI/TCP readahead buffer. Values below
6 rise to 6, and the maximum is 1024. The buffer size is this factor
times the DSI server quantum (default 1 MiB). The server reduces the
factor to cap the product at 1 GiB.
>
> ***NOTE:*** Each **afpd** child process allocates this buffer, so large
values consume a large amount of memory (buffer size \* number of
clients).

tcprcvbuf = *number* **(G)**

> Tries to set the TCP receive buffer SO_RCVBUF, in bytes, with
setsockopt(). Often the operating system restricts an application's
ability to set this value.

tcpsndbuf = *number* **(G)**

> Tries to set the TCP send buffer SO_SNDBUF, in bytes, with
setsockopt(). Often the operating system restricts an application's
ability to set this value.

use sendfile = *BOOLEAN* (default: *yes*) **(G)**

> Whether to use the sendfile syscall to send file data to clients. It
lets the kernel copy file data to the network without passing it through
**afpd**. Builds for Linux, FreeBSD, Solaris, and illumos have it. A file
whose filesystem refuses sendfile goes through a buffer instead.

recvfile = *BOOLEAN* (default: *no*) **(G)**

> Whether to use splice() on Linux to receive file data from clients
inside the kernel. On a platform without splice(), the server ignores the
option and logs a warning.

splice size = *number* (default: *1048576*) **(G)**

> Maximum number of bytes one splice() call moves when **recvfile** is
*yes*. The range is 4096 to 8388608 (8 MiB), and values outside it fall
back to the default. The server rounds a value that is not a power of two
up to the next one. The kernel may grant a pipe smaller than requested:
**fs.pipe-max-size** limits it to 1 MiB by default.
>
> ***NOTE:*** With more than 50 users connected at the same time and
**recvfile** *yes*, either raise the kernel setting
**fs.pipe-user-pages-soft** or reduce **splice size**. The kernel allows
only 64 MiB in total by default. Beyond that limit, users fall back to
**recvfile** off.

## CNID Database Backend Options

The CNID database maps each file and directory to the numeric ID Mac
clients use. These options control how the server stores and accesses
it.

cnid mysql host = *MySQL server address* **(G)**

> Name or address of a MySQL server for the mysql CNID backend.
>
> On UNIX systems, **localhost** (or leaving it unset) makes the MySQL
> client library prefer UNIX socket connections. These perform better
> than TCP for a local database. **127.0.0.1** forces a TCP connection to
> the loopback interface.

cnid mysql user = *MySQL user* **(G)**

> MySQL user for authentication with the server.

cnid mysql pw = *password* **(G)**

> Password for the MySQL server.

cnid mysql db = *database name* **(G)**

> Name of the CNID database. If it does not exist, the server tries to
> create it, which requires the CREATE DATABASE privilege for the
> specified user.

cnid scheme = *mysql* | *sqlite* (default: *sqlite*) **(G)**/**(V)**

> Sets the CNID backend for the volumes. The *mysql* backend may be absent
from your Netatalk installation. Run **afpd -v** to list the available
backends. For a name that is not a backend, **afpd** serves the volume
with *sqlite* and logs a warning. For a backend your build omits,
**afpd** does not open the volume. A **netatalk \-\-single-user** server
requires *sqlite*. The **dbd**(1) and **nad**(1) commands refuse a scheme
your installation cannot serve until you correct *afp.conf*.
>
> *mysql*: connects to a MySQL (or MariaDB) database instance provisioned
for Netatalk. Requires database administration and gives you full
control over CNID data storage.
>
> *sqlite*: uses the SQLite embedded database library and needs no
external database or daemon.

vol dbpath = *path* **(G)**/**(V)**

> Sets the path where the server stores the database information.
Specify a writable location, even if the volume is read-only. The
default is a subdirectory named after the volume under
*$localstatedir/netatalk/CNID*. A **(G)** value without a variable gets
the volume name appended as a subdirectory. The server uses a volume or
preset value as given. For a server run as root, the *sqlite* backend
creates the directory if needed and sets mode 1777 on it. It sets the
database files to 0666, so that **afpd** sessions running as the
connected user and **nad** can share them. A **netatalk \-\-single-user**
server keeps them owner-only (0700 and 0600). Keep the path outside any
volume, on a filesystem that supports POSIX record locks. The server
locks the *sqlite* database and the desktop database icons as it writes
them.

## Directory Cache Tuning

**afpd** keeps file and directory metadata in a directory cache to spare
disk lookups. These options size and tune it.

dircache size = *number* (default: *65536*) **(G)**

> The number of entries the directory cache can hold. An entry is a
directory or, optionally, a file. Range: 1024 (1K) to 1048576 (1M). A
value below the range applies the default, and one above it is capped,
each with a warning. Raising the value improves the hit ratio and uses
more memory.

dircache validation freq = *number* (default: *100*) **(G)**

> How often **afpd** re-checks a cached entry against the disk, to catch
changes by other processes. A value of 1 validates on every access. A
value above 1 validates less frequently. The server rejects a value
outside 1-100 with a warning and applies the default. The default assumes
the server is the only process writing the volume. If another process
modifies a volume, share it with **multi protocol = yes** or set 1
explicitly. **multi protocol** defaults this option to 1 together with
the other coherency settings. An explicit value above 1 takes precedence
over the **multi protocol** default, with a logged warning (see **multi
protocol**).

dircache mode = *lru* | *arc* (default: *arc*) **(G)**

> Which entries the cache drops when it is full. **lru** drops the least
recently used entry: stable and memory-efficient. **arc** (Adaptive
Replacement Cache) tunes itself, with 10-50% better hit ratios and 2×
better on sequential scans. ARC can use up to 2× the memory of **lru**:
it may keep evicted entries as ghosts for
instant re-promotion and drops cold ghosts. Example: a 64K-entry cache
uses ~12 MB, plus up to ~12 MB of ghosts under **arc**.
>
> **Recommendation**: Use **lru** for memory-constrained systems. Raise
**dircache size** (max entries) to improve the hit ratio.

dircache rfork budget = *number* (default: *32768*) **(G)**

> Total memory budget in KiB for caching resource fork data in the
directory cache. Set to 0 to disable resource fork caching. Resource
fork caching keeps small resource fork contents in memory, so repeated
client reads of a resource fork (FPRead) do not touch the disk. The
cache covers Mac OS resource forks, for example custom
icons and other Finder resources. It covers them in either storage the
**ea** option selects: AppleDouble sidecar files or Extended Attributes.
It does not cache other Extended Attributes.
>
> By default the cache excludes volumes with **multi protocol = yes**,
because another process can change resource forks behind **afpd**'s
back.
Setting this option explicitly lifts the exclusion, with a logged
warning (see **multi protocol**).
>
> Maximum: 10485760 KiB (10 GiB) total.

dircache rfork maxsize = *number* (default: *1024*) **(G)**

> Maximum size in KiB of a single cached resource fork entry. The cache
skips resource forks larger than this value even when the total budget
has room.
>
> Has no effect on volumes excluded from the resource fork cache, such as
**multi protocol = yes** volumes (see **dircache rfork budget**).
>
> Maximum: 10240 KiB (10 MiB) per entry.

***NOTE:*** The Configuration chapter of the manual has more on
directory cache tuning.

## Miscellaneous Options

The options below fit no other group.

afpstats = *BOOLEAN* (default: *no*) **(G)**

> Whether to provide AFP runtime statistics (connected users, open
volumes) via a UNIX socket. The **afpstats**(1) command reads them.

afpstats group = *group* **(G)**

> Sets the group owner of the afpstats UNIX socket. **afpd** creates the
socket with mode 0660, so only root and members of this group can access
the runtime statistics. If unset, the socket keeps the group the
operating system assigns when **afpd** creates it.

close vol = *BOOLEAN* (default: *no*) **(G)**

> Whether a configuration reload closes a volume it removes, even when
clients may have it open.

disconnect time = *number* (default: *24*) **(G)**

> Keeps a disconnected AFP session for *number* hours before dropping it.
A client that reconnects within that time resumes its session.

sleep time = *number* (default: *10*) **(G)**

> Keeps a sleeping client's AFP session for *number* hours before
disconnecting it.

extmap file = *path* **(G)**

> Sets the path to the file that defines file extension type/creator
mappings.

force xattr with sticky bit = *BOOLEAN* (default: *no*) **(G)**/**(V)**

> With *yes*, **afpd** writes a directory's metadata xattr as root. On a
directory with the sticky bit set, only the owner may write xattrs, so
the write otherwise may fail even with write access.

ignored attributes = *all* | *nowrite* | *nodelete* | *norename* **(G)**/**(V)**

> Which of the attributes that forbid writing, deleting, or renaming a
file or directory the server ignores. *all* includes all three.
>
> On macOS, a Finder lock on a file or directory, or the BSD uchg flag set
in the Terminal, uses all three attributes. To ignore the Finder lock and
the BSD uchg flag, set *ignored attributes = all*.

max connections = *number* (default: *200*) **(G)**

> Sets the maximum number of clients that can connect to the server at
the same time.

mimic model = *model* **(G)**

> Specifies a custom icon for the mounted AFP volume on macOS
clients. The default lets macOS choose. Requires a Netatalk build with
Zeroconf. Examples: **Laptop**, **RackMount**, and **Tower**.
>
> The complete set of model codes a macOS client supports is in its
system property files, such as
*/System/Library/CoreServices/CoreTypes.bundle/Contents/Info.plist*
through macOS 26 Tahoe. Open the file in Xcode and inspect the *Exported
Type Identifiers* key for the com.apple.* device types and their model
codes.

server name = *name* **(G)**

> Specifies a human-readable name that uniquely describes the AFP server.
Defaults to **hostname**. A Netatalk
build with Zeroconf support also registers this name as the service name
and advertises it as UTF-8, up to 63 octets (bytes) long.

signature = *STRING* **(G)**

> Specifies a server signature of at most 16 characters. The option suits
clustered environments, for example to provide fault isolation. By
default, **afpd** generates a random signature and saves it in the file
**afp_signature.conf**. A **netatalk \-\-single-user** server keeps that
file under **vol dbpath**.

strict locking = *BOOLEAN* (default: *no*) **(G)**

> Takes POSIX byte-range locks on file data for every AFP read and write,
so they conflict with other POSIX lockers such as Samba. Pair it with
Samba's own **strict locking = yes**. With *no*, AFP locks are invisible
to other processes. **afp read locks** is a deprecated synonym. Volumes
that other processes modify should set **multi protocol = yes**, which
defaults this option to *yes* together with the other coherency
settings. An explicit *no* takes precedence, with a logged warning (see
**multi protocol**).

solaris share reservations = *BOOLEAN* (default: *yes*) **(G)**

> Uses share reservations on Solaris and illumos, where the build finds
the fshare_t locks. The Solaris CIFS server uses them too, so this makes
a lock-coherent multi protocol server.

veto message = *BOOLEAN* (default: *no*) **(G)**

> Whether the server sends a client an AFP message when it tries to open
a vetoed name (see **veto files**). The message names the file and the
directory.

volnamelen = *number* (default: *80*) **(G)**

> Maximum length of a UTF8-MAC volume name for Mac OS X. The limit
depends on the client version:

    73: limit of Mac OS X 10.1
    80: limit of Mac OS X 10.4/10.5 (default)
    255: limit of recent Mac OS X

> Korean (Hangul) names reach the limit soonest. This does not affect
Mac OS 9 and earlier, because a Mac charset volume name never exceeds 27
bytes.

vol preset = *name* **(G)**/**(V)**

> Takes default options from section *name*, for all volumes when set in
[Global] or for one volume when set in its section.

## Search Options

The options below control Netatalk's file search: Spotlight (Finder
search) and Catalog Search.

dbus daemon = *path* **(G)**

> Sets the path to the dbus-daemon binary that starts a private D-Bus for
the *localsearch* backend. Use it when the compile-time default path
does not match the runtime environment.

search db = *BOOLEAN* (default: *yes*) **(V)**

> Uses the fast CNID database name search instead of a slow recursive
filesystem search when a client searches for an exact UTF-8 file name.
Other searches walk the volume.
>
> ***NOTE:*** Accurate results need a consistent CNID database. Access to
the volume through other file sharing protocols, or local filesystem
access on the host without **nad**(1), makes the CNID database
inaccurate. **dbd**(1) then must correct it.

spotlight = *BOOLEAN* (default: *yes*) **(G)**/**(V)**

> Whether to enable Spotlight (Finder search) compatible searches.
>
> ***NOTE:*** macOS searches network volumes through *Finder search* only,
not through the *Spotlight* widget in the menu bar, which searches local
volumes only. This is macOS client behavior, not a Netatalk limitation.
>
> A volume is searchable only when its effective **spotlight** setting is
*yes* and a supported **spotlight backend** is available for that volume.

spotlight attributes = *COMMA SEPARATED STRING* (default: *EMPTY*) **(G)**

> A list of Spotlight query attributes the *localsearch* backend accepts.
By default every attribute the LocalSearch SPARQL mapper knows is
searchable. A list limits searches to the attributes it names. Example:

    spotlight attributes = *,kMDItemTextContent

> This option does not affect the *cnid* or *xapian* backends.

spotlight backend = *backend* (default: *cnid*) **(G)**/**(V)**

> Search backend for the Spotlight (Finder search) feature.
>
> Depending on your Netatalk configuration, the available backends may
include *cnid*, *localsearch*, and *xapian*. *cnid* matches file names
from the CNID database.
>
> If the build includes *localsearch*, that backend adds full text search
and file metadata search.
>
> Run **netatalk -v** to list the backends in your installation.
>
> ***NOTE:*** The *xapian* backend is *EXPERIMENTAL*. Avoid it in
production.

spotlight expr = *BOOLEAN* (default: *yes*) **(G)**

> Whether the *localsearch* backend allows Spotlight logic expressions
when translating Spotlight RAW queries to SPARQL.
>
> This option does not affect the *cnid* or *xapian* backends.

spotlight results limit = *number* (default: *10000*) **(G)**

> Maximum number of Spotlight (Finder search) results returned per query.
Set *0* to remove the limit. The server raises a nonzero value below
*100* to *100*, the smallest result buffer the CNID search accepts. It
rejects a value above *16000000*, or one that is negative or unparsable,
logs it, and applies the default.
>
> Raise it if the limit truncates searches on large volumes. The limit
costs memory and query time in proportion to its size.
>
> Before it applied to all backends, this option had the name **sparql
results limit**. The old name still works and logs a deprecation warning.

## Logging Options

The options below control which messages the server logs, and where.

log file = *logfile* **(G)**

> Writes logs to *logfile*. If unset, the server logs to syslog with the
daemon facility. The server creates new log files with mode **0640**,
restricted further by the process umask, and preserves the permissions
of existing log files.

log level = *type:level [type:level ...]* | *type:level,[type:level, ...]* **(G)**

> Space or comma separated list of one or more log type and log level
pairs.
>
> The default is **default:note**.
>
> For each pair, the log level is the minimum severity the server logs
for that log type.
>
> The *default* log type applies to all messages. Without a log level for
*default*, the server logs only the log types with an explicit log level
and ignores all others.
>
> For example, with **log level = default:warn** the server logs every
message with severity *warn* or above, that is, *warn*, *error*, and
*severe*. It ignores anything below *warn*, such as *note*, *info*, and
*debug*.
>
> Available log types: *default*, *logger*, *cnid*, *afpdaemon*, *dsi*,
*atalkdaemon*, *papdaemon*, *uams*, *fce*, *ad*, *spotlight*.
>
> Available log levels, in falling order of severity: *severe*, *error*,
*warn*, *note*, *info*, *debug*, *debug6*, *debug7*, *debug8*, *debug9*,
*maxdebug*.
>
> ***NOTE:*** Both log type and log level are case insensitive.

log microseconds = *BOOLEAN* (default: *yes*) **(G)**

> Logs timestamps with microsecond accuracy. With *no*, timestamps record
whole seconds only. Applies only when **log file** is set.

## Filesystem Change Events (FCE)

**afpd** sends a UDP datagram to each configured listener for the FCE
events below.

- file modification (**fmod**)

- file deletion (**fdel**)

- directory deletion (**ddel**)

- file creation (**fcre**)

- directory creation (**dcre**)

- file move or rename (**fmov**)

- directory move or rename (**dmov**)

- login (**login**)

- logout (**logout**)

fce listener = *host[:port]* **(G)**

> Sends FCE events to *host*. The default *port* is 12250 when none is
specified. The server reads the option once, so afp.conf configures one
listener.

fce version = *1* | *2* (default: *1*) **(G)**

> FCE protocol version. Version 2 adds the source path, pid, and user name
to the events.

fce events = *fmod,fdel,ddel,fcre,dcre,fmov,dmov,login,logout* (default: all) **(G)**

> Specifies which FCE events the server sends, separated by commas or
spaces. By default all nine are active. The list replaces the default
set, and an unknown name drops out.

fce coalesce = *all* | *delete* | *create* **(G)**

> Merges bursts of related events on the same path within a short window.
With *create*, the server also drops each fcre event, because an fmod
always follows it.

fce holdfmod = *seconds* (default: *60*) **(G)**

> How long, in seconds, the server waits for further changes to a file
before sending one fmod event for them. Some applications open, modify,
and close a file several times for one save.

fce sendwait = *milliseconds* (default: *0*) **(G)**

> A delay in milliseconds between FCE events. Use it if listeners miss
FCE events when you create or delete a large number of files at once.
The burst of events such an operation triggers can overflow the UDP
buffer and cause packet loss. Must be a number between 0 and 999.

fce ignore names = *NAME[,NAME2,...]* (default: *.DS_Store*) **(G)**

> Comma-separated list of file names that generate no FCE events.

fce ignore directories = *PATH[,PATH2,...]* (default: empty) **(G)**

> Comma-separated list of directories that generate no FCE events. Each
is an absolute path on the host filesystem. A listed directory covers
itself and everything below it, deleted files included. Paths compare
after symbolic links resolve, so a directory listed through a symbolic
link matches its target.

fce notify script = *path* **(G)**

> Script that the server runs for every FCE event. Netatalk installs
**fce_ev_script**(8) as a notification helper under its libexec directory.
By default it writes each event to syslog.

## Options for ACL handling

These options control how the server maps ACLs for clients and which
LDAP directory it queries for the UUIDs that ACLs need.

chmod request = *preserve* (default) | *ignore* | *simple* **(G)**/**(V)**

> What the server does with a client's chmod() request when ACLs are
present.
>
> - **ignore** - The server ignores UNIX chmod() requests. Use it to give
the parent directory's ACL inheritance full control over new items.
>
> - **preserve** - Applies the chmod() but keeps the ZFS ACEs for named
users and groups, or the POSIX ACL group mask.
>
> - **simple** - Performs the chmod() as requested, without extra steps.

map acls = *none* | *rights* | *mode* (default: *rights*) **(G)**

> How the server maps a file's ACL into the access rights and UNIX mode it
reports to clients.
>
> none
>
> > No ACL mapping.
>
> rights
>
> > Maps effective permissions to the AFP user access rights (UARights)
the client displays.
>
> mode
>
> > Additionally maps ACLs to the UNIX mode of the filesystem object.

To display ACLs on the client, you must set up both client and server in
one authentication domain. That means a directory service such as LDAP,
Open Directory, or Active Directory. On macOS, ACLs are bound to UUIDs,
not just to uids or gids. Netatalk must therefore map every filesystem
uid and gid to a UUID before it can translate the server's ACLs for the
client.

Netatalk can query a directory server with LDAP. Active Directory and
Open Directory already provide a UUID attribute for users and groups.
With another directory server, for example OpenLDAP, you reuse an unused
attribute or add a new one.

Configure these LDAP options for Netatalk:

ldap auth method = *none* | *simple* **(G)**

> none
>
> > Anonymous bind.
>
> simple
>
> > Bind with **ldap auth dn** and **ldap auth pw**.

ldap auth dn = *dn* **(G)**

> Distinguished Name of the user for simple bind.

ldap auth pw = *password* **(G)**

> Password for simple bind.

ldap uri = *ldap://somehost:1234/* **(G)**

> Specifies the LDAP URI of the server to connect to. The URI scheme may
be ldap for LDAP over TCP, ldapi for LDAP over IPC, or ldaps for LDAP
over TLS. The LDAP library must support the scheme. Only ACL support
needs this option, to look up UUIDs in LDAP.
>
> **afpldaptest**(1) checks the syntax of your config.

ldap userbase = *base dn* **(G)**

> DN of the user container in LDAP.

ldap userscope = *scope* **(G)**

> Search scope for user search: **base** | **one** | **sub**

ldap groupbase = *base dn* **(G)**

> DN of the group container in LDAP.

ldap groupscope = *scope* **(G)**

> Search scope for group search: **base** | **one** | **sub**

ldap uuid attr = *dn* **(G)**

> Name of the LDAP attribute with the UUIDs.
>
> ***NOTE:*** The server uses this attribute for both users and groups.

ldap name attr = *dn* **(G)**

> Name of the LDAP attribute with the user's short name.

ldap group attr = *dn* **(G)**

> Name of the LDAP attribute with the group's short name.

ldap uid attr = *dn* **(G)**

> Accepted by the parser and unused: no code reads the value.

ldap uuid string = *STRING* **(G)**

> Format of the uuid string in the directory. A series of x and -, where
every x denotes a value 0-9a-f and every - is a separator.
The format must contain exactly 32 x characters and be at most 63 characters
long, including separators. Invalid formats fall back to the default.
>
> Default: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

ldap uuid encoding = *string* (default) | *ms-guid* **(G)**

> Format of the UUID in the LDAP attribute. The default, string, passes
through the ASCII UUID that Open Directory and most other LDAP stores
return. With ms-guid, the server converts the internal UUID
representation to and from the binary format of the objectGUID attribute
on Active Directory objects. Active Directory environments also need
**ldap user filter** and **ldap group filter**.

ldap user filter = *STRING* (default: empty) **(G)**

> LDAP filter that matches user objects. Active Directory needs it,
because users and groups share one subtree there.
>
> Recommended setting for Active Directory: **objectClass=user**.

ldap group filter = *STRING* (default: empty) **(G)**

> LDAP filter that matches group objects. Active Directory needs it,
because users and groups share one subtree there.
>
> Recommended setting for Active Directory: **objectClass=group**.

## Classic Options

Options for features relevant only to Classic Mac OS or Apple II
clients. The *Charset Options* section above also applies to them.

appletalk = *BOOLEAN* (default: *no*) **(G)**

> Whether to serve AFP over AppleTalk. Requires an operating system that
supports the AppleTalk networking protocol and a Netatalk build with
AppleTalk support.

ddp address = *ddp address* **(G)**

> Specifies the DDP address of the server. The default is to auto-assign
an address (0.0). This is only useful if you are running AppleTalk on
more than one interface.

ddp zone = *ddp zone* **(G)**

> Specifies the AppleTalk zone to register the server in. The default is
to register the server in the default zone of the last interface
configured by the system.

legacy icon = *icon* **(G)**/**(V)**

> Sets the shared volume icon that the Finder displays in Classic Mac OS.
If the volume already has a custom icon, the server ignores this option.
Available icon styles:
>
> - **daemon**
> - **declogo**
> - **fileserver**
> - **globe**
> - **nas**
> - **sdcard**
> - **sunlogo**
> - **viking**

legacy volume size = *BOOLEAN* (default: *no*) **(V)**

> Limits disk size reporting to 2 GB for legacy clients. Use it for older
Macintoshes running System 7.1 or earlier with newer AppleShare clients.

login message = *message* **(G)**

> Sets a message that Classic Mac OS clients see when they log in to the
server. Write it in **unix charset**. It may contain extended characters.

prodos = *BOOLEAN* (default: *no*) **(V)**

> Whether to support ProDOS. Set *yes* only for volumes you expect to
netboot an Apple II from. It sets the volume's boot flag and reports at
most 32 MB free.

## Debug Parameters

Use these options for debugging only. Do not enable them in production
environments.

tickleval = *number* (default: *30*) **(G)**

> Seconds between the keepalive packets (tickles) **afpd** sends to a
client.

timeout = *number* (default: *4*) **(G)**

> Number of tickles to send before timing out a connection. With the
defaults, a connection times out after 2 minutes. A client that stops
sending in the middle of a request times out the same way and can
reconnect.

client polling = *BOOLEAN* (default: *no*) **(G)**

> With *yes*, **afpd** does not advertise server notifications, so
connected clients poll it every 10 seconds for changes in open server
windows.

# Explanation of Volume Parameters

## Home Section Parameters

The parameters below apply to the home volume only.

basedir regex = *regex* **(H)**

> Regular expression that matches the parent directory of the user homes.
A plain path is a valid expression, for example **basedir regex =
/home**. If the path goes through a symlink, set the real path it
resolves to.

home name = *name* **(H)**

> Name of each user's home volume. The default is *$u's home*. The name
must contain *$u*. A name without it logs a warning and the default
applies.

## Volume Sections Parameters

Each volume should at least define **path** and **volume name**.

path = *path* **(V)**

> The directory to share, as an absolute path.

volume name = *STRING* (default: section name in lowercase) **(V)**

> The name of the shared AFP volume. When not set, it defaults to the
volume's section name in lowercase.
>
> No two volumes may have the same name. The volume name cannot contain
the ':' character. The server mangles a volume name that is too long. A
Mac charset volume name can have at most 27 bytes. The **volnamelen**
parameter limits a UTF8-MAC volume name.

vol size limit = *size in MiB* **(V)**

> Limits the volume size the server reports, in MiB, so Time Machine
cannot fill the real disk with backups. Example: **vol size limit =
1000** limits the reported disk space to 1 GB.
>
> ***IMPORTANT:*** The server estimates used space from the Time Machine
sparsebundles only. It multiplies the band size in each sparsebundle's
Info.plist XML file by the number of band files in bands/. Other content
on the volume is not counted, so store none there.

valid users = *user* / *@group* **(V)**

> Lists the users and groups allowed to access the volume, delimited by
spaces or commas. A @ prefix marks a group. Example:

    valid users = user @group

invalid users = *user* / *@group* **(V)**

> Lists users and groups denied access to the volume, in the same format
as **valid users**.

hosts allow = *IP host address/IP netmask bits* [ ... ] **(V)**

> Allows only the listed hosts and networks and rejects all others. Write
the network address in dotted-decimal format for IPv4 or in hexadecimal
format for IPv6.
>
> Example: hosts allow = 10.1.0.0/16 10.2.1.100 2001:0db8:1234::/48

hosts deny = *IP host address/IP netmask bits* [ ... ] **(V)**

> Rejects the listed hosts and networks and allows all others.
>
> Example: hosts deny = 192.168.100/24 10.1.1.1 2001:db8::1428:57ab

ea = *sys* | *samba* | *ad* | *none* (default: auto detect) **(G)**/**(V)**

> How the server stores Extended Attributes and Mac OS Resource Forks.
>
> By default, the server tries **sys** (filesystem Extended Attributes).
If that fails, it falls back to **ad** (AppleDouble sidecars). The auto
detection sets a marker EA on the shared directory itself, as root. A
read-only volume skips that test and assumes support, so auto detection
gives it **sys**. Set this option explicitly on a read-only volume whose
filesystem has no Extended Attributes.
>
> sys
>
> > Uses filesystem Extended Attributes.
>
> samba
>
> > Uses filesystem Extended Attributes, but appends a NUL byte to each
xattr. That keeps the volume compatible with Samba's vfs_streams_xattr.
It requires filesystem Extended Attribute support. A volume whose
filesystem cannot store them fails to load.
>
> > This option selects the storage format only. A volume that other
processes modify should also set **multi protocol = yes**, which applies
the coherency defaults that safe concurrent access requires.
>
> ad
>
> > Uses AppleDouble v2 metadata stored as files in *.AppleDouble* sidecar
directories. Recommended only when the host's filesystem does not
support Extended Attributes.
>
> none
>
> > No Extended Attributes support.
>
> ***WARNING:*** Do not use **samba** on a volume previously set to
**sys**. This may lead to data loss. To convert, copy the files between
volumes from a client.

multi protocol = *BOOLEAN* (default: *no*) **(G)**/**(V)**

> Declares that other processes modify the volume. Examples are another
file sharing protocol such as Samba/SMB or NFS, and local access on the
server by scripts, cron jobs, or shell users.
>
> With *yes*, the server defaults change to the values safe concurrent
access requires. **strict locking** defaults to *yes*, and **dircache
validation freq** defaults to *1*. The resource fork cache excludes the
volume, and on Solaris and illumos an explicit **solaris share
reservations = no** logs a warning.
Explicit settings always take precedence. Every explicit setting that
weakens coherency (all processes seeing the same file state) logs a
verbose warning describing the risk.
>
> Pair it with **ea = samba**, so the other accessors can read the
volume's Extended Attributes. The server logs a note at startup when a
multi protocol volume uses a different ea format.
>
> **strict locking** and **dircache validation freq** are server-wide
settings, so one multi protocol volume applies them to every volume. A
volume that sets **multi protocol = no** still gets them. Only the
exclusion from the resource fork cache is per-volume. Volumes load with
the configuration present at startup, so a change to this option takes
effect for sessions that connect afterwards.
>
> With *no* (the default), the server assumes it is the only process
writing its volumes and serves them from its caches for maximum
performance.

mac charset = *charset* **(V)**

> Specifies the Mac client charset for this volume, for example
*MAC_ROMAN* or *MAC_CYRILLIC*. If not set, the global setting applies.
You need it only for volumes whose Mac charset differs from the one set
in [Global].

casefold = *option* **(V)**

> Controls whether and how the server changes the case of filenames. The
available options are:
>
> **tolower** - Lowercases names in both directions.
>
> **toupper** - Uppercases names in both directions.
>
> **xlatelower** - Client sees lowercase, server sees uppercase.
>
> **xlateupper** - Client sees uppercase, server sees lowercase.

password = *password* **(V)**

> Sets a volume password of at most 8 characters, the size of the AFP
password field. Clients must enter it to mount the volume. Use ASCII
characters.

file perm = *mode* **(V)**

directory perm = *mode* **(V)**

> Adds (bitwise OR) this mode to the permissions the client requests.
**file perm** applies to files only, and **directory perm** applies to
directories only. These options can grant permission bits, but cannot
remove bits the client requests or set an absolute mode. For example, a
client requesting mode **0644** with **file perm = 0222** results in mode
**0666**.
>
> The server applies the modes when the client changes permissions. They
are not initial creation modes. **umask** masks permission bits. The
server ignores these options with **unix priv = no**.
>
> **Example**: Volume for a collaborative workgroup

    file perm = 0660
    directory perm = 0770

umask = *mode* **(V)**

> Sets a permission mask that removes bits from the modes it applies to.
The server ignores this option with **unix priv = no**.

preexec = *command* **(V)**

> Shell command the server runs through /bin/sh when a client mounts the
volume.

postexec = *command* **(V)**

> Shell command the server runs through /bin/sh when the volume closes.

rolist = *users/groups* **(V)**

> Gives the listed users and groups read-only access to the volume, in
the **valid users** format.

rwlist = *users/groups* **(V)**

> Gives the listed users and groups read/write access to the volume, in
the **valid users** format.

veto files = *vetoed names* **(V)**

> Hides files and directories whose path matches one of the
'/'-delimited vetoed names. The veto string must always end with a '/',
for example **veto files = veto1/** or **veto files = veto1/veto2/**.

volume uuid = *UUID* **(V)**

> Specifies a custom UUID for the volume, in the standard format, for
example **550E8400-E29B-41D4-A716-446655440000**. If not specified, the
server generates a random UUID for the volume at startup. It saves the
UUID in the file **afp_voluuid.conf** in the shared state directory, or
under **vol dbpath** for a **netatalk \-\-single-user** server.
>
> Time Machine clients identify a backup volume by this UUID. In most
circumstances you should let the server manage it. Set it when **afpd**
runs unprivileged, without **netatalk \-\-single-user**, and cannot write
to the shared state directory.

## Volume options

Every option below takes a boolean value.

acls = *BOOLEAN* (default: *yes*) **(V)**

> Whether to flag volumes as supporting ACLs. Defaults to *yes* when the
build includes ACL support.

case sensitive = *BOOLEAN* (default: *yes*) **(V)**

> Whether to advertise volumes as supporting case-sensitive filenames.
If the host filesystem is case-insensitive, set *no*.

cnid dev = *BOOLEAN* (default: *yes*) **(V)**

> Whether to store the unique device number of each file or directory in
the CNID backend. The server uses it to detect a file moved across
devices and to update the CNID database accordingly.
>
> With this option, make sure the device number of the volume is
consistent across reboots. Persistent device naming, for example by
UUID, in /etc/fstab or the system's mount configuration achieves that.
Otherwise the server rebuilds the CNID database on every reboot.

convert appledouble = *BOOLEAN* (default: *no*) **(V)**

> Whether to convert AppleDouble v2 metadata files to filesystem Extended
Attributes as clients access them.
>
> This helps when upgrading from Netatalk v2 to a later version, but
slows every filesystem access a little. If you
run **dbd**(1) on the volumes after upgrading Netatalk, you do not need
this option. **dbd**(1) then does the conversion, not the server on the
fly.

delete veto files = *BOOLEAN* (default: *no*) **(V)**

> Whether the server deletes the vetoed files and directories inside a
directory a client deletes (see **veto files**). With *no* (the default),
the delete fails if the directory contains any non-vetoed files or
directories. This is usually what you want.
>
> With *yes*, the server tries to delete every file and directory within
the vetoed directory recursively.

follow symlinks = *BOOLEAN* (default: *no*) **(V)**

> Whether **afpd** follows symlinks on the server. The default *no*
matches Mac OS X's AFP server.
>
> When a client creates a symlink through AFP, the server validates the
target. It rejects absolute paths, paths with **..** components,
relative paths that resolve outside the volume root, and targets on a
different filesystem device than the volume.
>
> ***NOTE:*** This option is security-sensitive. Set *yes* only if you
understand the security implications of letting clients follow symlinks
on the server.

invisible dots = *BOOLEAN* (default: *no*) **(V)**

> Makes dot files invisible.
>
> ***WARNING:*** This option has an unwanted side effect with macOS
applications that save to a temporary dot file and rename it into place.
The saved file ends up invisible. The option is useful only to hide files
that begin with a dot on Mac OS 9. It is useless on macOS, where both the
Finder and the Terminal hide files that begin with a dot anyway.

network ids = *BOOLEAN* (default: *yes*) **(V)**

> Whether the server supports network IDs, the UUIDs that name users and
groups in ACLs. With *no*, the client does not use the ACL AFP functions.

preexec close = *BOOLEAN* (default: *no*) **(V)**

> Whether a non-zero exit code from **preexec** closes the volume
immediately, so clients cannot see or mount it.

read only = *BOOLEAN* (default: *no*) **(V)**

> Makes the volume read-only for all users. With **ea** unset, the
auto detection assumes Extended Attribute support and uses **sys**.

stat vol = *BOOLEAN* (default: *yes*) **(V)**

> Whether the server checks that the volume path exists and is readable
before listing the volume. Set *no* for automounted volumes or ones a
**preexec** script creates.

time machine = *BOOLEAN* (default: *no*) **(V)**

> Whether to advertise the volume as a Time Machine backup destination.
>
> ***NOTE:*** Time Machine supports static volume paths only. If the
volume path contains '$u', the server does not advertise the volume to
Time Machine clients.

unix priv = *BOOLEAN* (default: *yes*) **(V)**

> Whether to use AFP3 UNIX privileges. They let Mac clients see and set
UNIX owner, group, and permissions. This should stay *yes* for macOS
clients. **file perm**, **directory perm**, and **umask** depend on it.

# Examples

## Example: Modern Mac clients

This example enables Spotlight (Finder search) and AFP stats in a
Netatalk build with Spotlight and AFP stats support. The **mimic model**
option makes the server look like an Xserve.

Each user's home volume path is */home/$u/afp-data*.

    [Global]
    afpstats = yes
    afpstats group = netatalk
    spotlight = yes
    mimic model = RackMac

    [Homes]
    basedir regex = /home
    path = afp-data

## Example: Classic Mac clients

This example enables AppleTalk in a Netatalk build with AppleTalk
support. The Random Number and Cleartext authentication modules support
older Mac clients. The **legacy icon** option gives the volume a custom
icon when mounted on Classic Mac OS clients.

**legacy volume size** is for Macs running System 7.1 or earlier, and
**prodos** for volumes an Apple II netboots from. Their entries under
*Classic Options* give the limits.

    [Global]
    appletalk = yes
    uam list = uams_dhx.so uams_dhx2.so uams_randnum.so uams_clrtxt.so
    legacy icon = daemon

    [mac]
    volume name = Mac Files
    path = /srv/mac
    legacy volume size = yes

    [apple2]
    volume name = Apple II Files
    path = /srv/apple2
    prodos = yes

# See Also

**afpd**(8), **afppasswd**(1), **afp_signature.conf**(5), **extmap.conf**(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
