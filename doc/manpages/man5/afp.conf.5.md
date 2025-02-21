# Name

afp.conf - Netatalk configuration file

# Synopsis

The **afp.conf** file is the configuration file for the **Netatalk** AFP
file server.

All AFP-specific configurations and AFP volume definitions are done via
this file.

# File Format

The file consists of sections and parameters. A section begins with the
name of the section in square brackets and continues until the next
section begins. Sections contain parameters of the form:

        option = value

The file is line-based - that is, each newline-terminated line
represents either a comment, a section name or a parameter.

Parameter names are case sensitive, while section names are not.

Only the first equals sign in a parameter is significant. Whitespace
before or after the first equals sign is discarded. Leading, trailing
and internal whitespace in section and parameter names is irrelevant.
Leading and trailing whitespace in a parameter value is discarded.
Internal whitespace within a parameter value is retained verbatim.

Any line beginning with a semicolon (“;”) or a hash (“\#”) character is
ignored, as are lines containing only whitespace.

Any line ending in a “ **\\** ” is continued on the next line in the
customary UNIX fashion.

The values following the equals sign in parameters are all either a
string (no quotes needed) or a boolean, which may be given as yes/no,
1/0 or true/false. Case is not significant in boolean values, but is
preserved in string values. Some options such as **file perm** take
numeric values.

The parameter **include = path** allows you to include one config file
inside another. The file is included literally, as though typed in
place. Nested includes are not supported.

# Section Descriptions

Each section in the configuration file (except for the \[Global\]
section) describes a shared resource (known as a “volume”).
The parameters within the section define the volume attributes and options.

There are two special sections, \[Global\] and \[Homes\], which are
described under *special sections*. The following notes apply to
ordinary section descriptions.

A volume consists of a directory to which access is being given plus a
description of the access rights which are granted to the user of the
service. For volumes the **path** option must specify the directory to
share.

The name of the volume is defined via the **name** option.
When absent, the volume name is the name of the section, expressed in
lowercase.

Any volume section without **path** option is considered a *vol preset*
which can be selected in other volume sections via the **vol preset**
option and constitutes defaults for the volume. For any option specified
both in a preset *and* in a volume section the volume section setting
completely substitutes the preset option.

The access rights granted by the server are masked by the access rights
granted to the specified or guest UNIX user by the host system. The
server does not grant more access than the host system grants.

The following sample section defines an AFP volume. The user has full
access to the path */foo/bar*. The share is accessed via the share name
*Baz Volume*:

    [baz]
        name = Baz Volume
        path = /foo/bar

# Special Sections

## The \[Global\] section

Parameters in this section apply to the server as a whole. Parameters
denoted by a (G) below are must be set in this section.

## The \[Homes\] section

This section enable sharing of the UNIX server user home directories.
The one mandatory option is **basedir regex**. It should be set to a path
which matches the parent directory of the user homes.

Specifying the optional **path** parameter makes it so that only
the subdirectory **path** is shared, rather than the entire home directory.
The following example illustrates this. Given all user home directories
are stored under */home*:

    [Homes]
        path = afp-data
        basedir regex = /home

For a user *john* this results in an AFP home volume with a path of
*/home/john/afp-data*.

If **basedir regex** contains symlink, set the canonicalized absolute
path. When */home* links to */usr/home*:

    [Homes]
        basedir regex = /usr/home

The optional parameter **home name** can be used to change
the Homes volume name, which is *$u's home* by default. See
below for more information on VARIABLE SUBSTITUTIONS.

    [Homes]
        home name = The home of $u
        basedir regex = /home

For the same user *john* this results in an AFP home volume called
*The home of john*.

Any parameter denoted by a **(H)** below can be used in the Homes section.

# Parameters

Parameters define the specific attributes of sections.

Some parameters are specific to the \[Global\] section (e.g., **log
type**). All others are permissible only in volume sections. The letter
*G* in parentheses indicates that a parameter is specific to the
\[Global\] section. The letter *V* indicates that a parameter can be
specified in a volume specific section.

# Variable Substitutions

You can use variables in volume names. The use of variables in paths is
limited to $u.

1.  if you specify an unknown variable, it will not get converted.

2.  if you specify a known variable, but that variable doesn't have a
    value, it will get ignored.

The variables which can be used for substitutions are:

$b

> basename

$c

> client's ip address

$d

> volume pathname on server

$f

> full name (contents of the gecos field in the passwd file)

$g

> group name

$h

> hostname

$i

> client's ip, without port

$s

> server name (this can be the hostname)

$u

> user name (if guest, it is the user that guest is running as)

$v

> volume name

$$

> prints dollar sign ($)

# Explanation of Global Parameters

## Authentication Options

ad domain = <domain\> **(G)**

> Append @DOMAIN to username when authenticating. Useful in Active
Directory environments that otherwise would require the user to enter
the full user@domain string.

admin auth user = <user\> **(G)**

> Specifying e.g. "**admin auth user = root**" whenever a normal user login
fails, afpd will try to authenticate as the specified **admin auth user**.
If this succeeds, a normal session is created for the original
connecting user. Said differently: if you know the password of
**admin auth user**, you can authenticate as any other user.

admin group = <group\> **(G)**

> Allows users of a certain group to be seen as the superuser when they
log in. This option is disabled by default.

force user = <user\> **(G)**

> This specifies a UNIX user name that will be assigned as the default
user for all users connecting to this server. This is useful for sharing
files. You should also use it carefully as using it incorrectly can
cause security problems.

force group = <group\> **(G)**

> This specifies a UNIX group name that will be assigned as the default
primary group for all users connecting to this server.

k5 keytab = <path\> **(G)**; k5 service = <service\> **(G)**; k5 realm = <realm\> **(G)**

> These are required if the server supports the Kerberos 5 authentication
UAM.

nt domain = <domain\> **(G)**; nt separator = <SEPARATOR\> **(G)**

> Use for e.g. winbind authentication, prepends both strings before the
username from login and then tries to authenticate with the result
through the available and active UAM authentication modules.

save password = <BOOLEAN\> (default: *yes*) **(G)**

> Enables or disables the ability of clients to save passwords locally.

set password = <BOOLEAN\> (default: *no*) **(G)**

> Enables or disables the ability of clients to change their passwords via
chooser or the "connect to server" dialog.

uam list = <uam list\> **(G)**

> Space or comma separated list of UAMs. (The default is "uams_dhx.so
uams_dhx2.so").

The most commonly used UAMs are:

uams_guest.so

> allows guest logins

uams_clrtxt.so

> (uams_pam.so or uams_passwd.so) Allow logins with passwords transmitted
in the clear. Compatible with Mac OS 9 and earlier.

uams_randnum.so

> allows Random Number and Two-Way Random Number Exchange for
authentication (requires a separate file containing the passwords,
either the default *afppasswd* file or the one specified via
"**passwd file**"). See **afppasswd**(1) for details.
Compatible with Mac OS 9 and earlier.

uams_dhx.so

> (uams_dhx_pam.so or uams_dhx_passwd.so) Allow Diffie-Hellman eXchange
(DHX) for authentication.

uams_dhx2.so

> (uams_dhx2_pam.so or uams_dhx2_passwd.so) Allow Diffie-Hellman eXchange
2 (DHX2) for authentication.

uam_gss.so

> Allow Kerberos V for authentication (optional)

uam path = <path\> **(G)**

> Sets the default path for UAMs for this server.

## Charset Options

With OS X Apple introduced the AFP3 protocol. One of the big changes
was, that AFP3 uses Unicode names encoded as Decomposed UTF-8
(UTF8-MAC). Previous AFP/OS versions used charsets like MacRoman,
MacCentralEurope, etc.

To be able to serve AFP3 and older clients at the same time, **afpd**
needs to be able to convert between UTF-8 and Mac charsets. Even OS X
clients partly still rely on the mac charset. As there's no way, **afpd**
can detect the codepage a pre AFP3 client uses, you have to specify it
using the **mac charset** option. The default is MacRoman, which should be
fine for most western users.

As **afpd** needs to interact with UNIX operating system as well, it needs
to be able to convert from UTF8-MAC / Mac charset to the UNIX charset.
By default **afpd** uses *UTF8*. You can set the UNIX charset using the
**unix charset** option. If you're using extended characters in the
configuration files for **afpd**, make sure your terminal matches the
**unix charset**.

mac charset = <charset\> **(G)**/**(V)**

> Specifies the Mac clients charset, e.g. *MAC_ROMAN*. This is used to
convert strings and filenames to the clients codepage for OS9 and
Classic, i.e. for authentication and AFP messages (SIGUSR2 messaging).
This will also be the default for the volumes **mac charset**. Defaults to
*MAC_ROMAN*.

unix charset = <charset\> **(G)**

> Specifies the servers unix charset, e.g. *ISO-8859-15* or *EUC-JP*. This
is used to convert strings to/from the systems locale, e.g. for
authentication, server messages and volume names. If *LOCALE* is set,
the systems locale is used. Defaults to *UTF8*.

vol charset = <charset\> **(G)**/**(V)**

> Specifies the encoding of the volumes filesystem. By default, it is the
same as **unix charset**.

> **NOTE**

> It is highly recommended to stick to the default UTF-8 encoding.

## Password Options

passwd file = <path\> **(G)**

> Sets the path to the Randnum UAM passwd file for this server.

passwd minlen = <number\> **(G)**

> Sets the minimum password length, if supported by the UAM

## Network Options

advertise ssh = <BOOLEAN\> (default: *no*) **(G)**

> Allows old Mac OS X clients (10.3.3-10.4) to automagically establish a
tunneled AFP connection through SSH. If this option is set, the server's
answers to client's FPGetSrvrInfo requests contain an additional entry.
It depends on both client's settings and a correctly configured and
running **sshd**(8) on the server to let things work.

> **NOTE**

> Setting this option is not recommended since globally encrypting AFP
connections via SSH will increase the server's load significantly. On
the other hand, Apple's client side implementation of this feature in
MacOS X versions prior to 10.3.4 contained a security flaw.

afp interfaces = <name \[name ...\]\> **(G)**

> Specifies the network interfaces that the server should listen on. The
default is to advertise the first IP address of the system, while
listening for any incoming request.

> **NOTE**

> Do not use at the same time as the **afp listen** option.

afp listen = <ip address\[:port\] \[ip address\[:port\] ...\]\> **(G)**

> Specifies the IP address that the server should advertise **and**
listens to. The default is advertise the first IP address of the system,
but to listen for any incoming request. The network address may be
specified either in dotted-decimal format for IPv4 or in hexadecimal
format for IPv6.

IPv6 address + port combination must use URL the format using square
brackets \[IPv6\]:port

> **NOTE**

> Do not use at the same time as the **afp interfaces** option.

afp port = <port number\> **(G)**

> Allows a different TCP port to be used for AFP. The default is 548. Also
sets the default port applied when none specified in an **afp listen**
option.

appletalk = <BOOLEAN\> (default: *no*) **(G)**

> Enables support for AFP-over-Appletalk. This option requires that your
operating system supports the AppleTalk networking protocol.

cnid listen = <ip address\[:port\] \[ip address\[:port\] ...\]\> **(G)**

> Specifies the IP address that the CNID server should listen on. The
default is **localhost:4700**.

ddp address = <ddp address\> **(G)**

> Specifies the DDP address of the server. The default is to auto-assign
an address (0.0). This is only useful if you are running AppleTalk on
more than one interface.

ddp zone = <ddp zone\> **(G)**

> Specifies the AppleTalk zone to register the server in. The default is
to register the server in the default zone of the last interface
configured by the system.

disconnect time = <number\> **(G)**

> Keep disconnected AFP sessions for <number\> hours before dropping them.
Default is 24 hours.

dsireadbuf = <number\> **(G)**

> Scale factor that determines the size of the DSI/TCP readahead buffer,
default is 12. This is multiplies with the DSI server quantum (default
1MiB) to give the size of the buffer. Increasing this value might
increase throughput in fast local networks for volume to volume copies.
*Note*: This buffer is allocated per afpd child process, so specifying
large values will eat up large amount of memory (buffer size \* number
of clients).

fqdn = <name\[:port\]\> **(G)**

> Specifies a fully-qualified domain name, with an optional port. This is
discarded if the server cannot resolve it. This option is not honored by
AppleShare clients <= 3.8.3. This option is disabled by default. Use
with caution as this will involve a second name resolution step on the
client side. Also note that afpd will advertise this name:port
combination but not automatically listen to it.

hostname = <name\> **(G)**

> Sets a custom AFP server name to be displayed in the client.
When absent, the fallback server name is the host system's hostname
up until the first period.

max connections = <number\> **(G)**

> Sets the maximum number of clients that can simultaneously connect to
the server (default is 200).

server quantum = <number\> **(G)**

> This specifies the DSI server quantum. The default value is 0x100000 (1
MiB). The maximum value is 0xFFFFFFFFF, the minimum is 32000. If you
specify a value that is out of range, the default value will be set. Do
not change this value unless you're absolutely sure, what you're doing

sleep time = <number\> **(G)**

> Keep sleeping AFP sessions for <number\> hours before disconnecting
clients in sleep mode. Default is 10 hours.

tcprcvbuf = <number\> **(G)**

> Try to set TCP receive buffer using setsockopt(). Often OSes impose
restrictions on the applications ability to set this value.

tcpsndbuf = <number\> **(G)**

> Try to set TCP send buffer using setsockopt(). Often OSes impose
restrictions on the applications ability to set this value.

recvfile = <BOOLEAN\> (default: *no*) **(G)**

> Whether to use splice() on Linux for receiving data.

splice size = <number\> (default: *64k*) **(G)**

> Maximum number of bytes spliced.

use sendfile = <BOOLEAN\> (default: *yes*) **(G)**

> Whether to use sendfile syscall for
sending file data to clients.

zeroconf = <BOOLEAN\> (default: *yes*) **(G)**

> Whether to use automatic Zeroconf service
registration if Avahi or mDNSResponder were compiled in.

## Miscellaneous Options

afp read locks = <BOOLEAN\> (default: *no*) **(G)**

> Whether to apply locks to the byte region read in FPRead calls. The AFP
spec mandates this, but it's not really in line with UNIX semantics and
is a performance hug.

afpstats = <BOOLEAN\> (default: *no*) **(G)**

> Whether to provide AFP runtime statistics (connected users, open
volumes) via dbus.

basedir regex = <regex\> **(H)**

> Regular expression which matches the parent directory of the user homes.
If **basedir regex** contains symlink, you must set the canonicalized
absolute path. In the simple case this is just a path i.e.
**basedir regex = /home**

chmod request = <preserve (default) | ignore | simple\> **(G)**/**(V)**

> Advanced permission control that deals with ACLs.

- **ignore** - UNIX chmod() requests are completely ignored, use this
  option to allow the parent directory's ACL inheritance full control
  over new items.

- **preserve** - preserve ZFS ACEs for named users and groups or POSIX ACL
  group mask

- **simple** - just to a chmod() as requested without any extra steps

close vol = <BOOLEAN\> (default: *no*) **(G)**

> Whether to close volumes possibly opened by clients when they're removed
from the configuration and the configuration is reloaded.

cnid mysql host = <MySQL server address\> **(G)**

> name or address of a MySQL server for use with the mysql CNID backend.

cnid mysql user = <MySQL user\> **(G)**

> MySQL user for authentication with the server.

cnid mysql pw = <password\> **(G)**

> Password for MySQL server.

cnid mysql db = <database name\> **(G)**

> Name of an existing database for which the specified user has full
privileges.

cnid server = <ipaddress\[:port\]\> **(G)**/**(V)**

> Specifies the IP address and port of a cnid_metad server, required for
CNID dbd backend. Defaults to localhost:4700. The network address may be
specified either in dotted-decimal format for IPv4 or in hexadecimal
format for IPv6.-

dbus daemon = <path\> **(G)**

> Sets the path to dbus-daemon binary used by the Spotlight feature. Can
be used when the compile-time default path does not match the runtime
environment.

dircachesize = <number\> **(G)**

> Maximum possible entries in the directory cache. The cache stores
directories and files. It is used to cache the full path to directories
and CNIDs which considerably speeds up directory enumeration.

Default size is 8192, maximum size is 131072. Given value is rounded up
to nearest power of 2. Each entry takes about 100 bytes, which is not
much, but remember that every afpd child process for every connected
user has its cache.

extmap file = <path\> **(G)**

> Sets the path to the file which defines file extension type/creator
mappings.

force xattr with sticky bit = <BOOLEAN\> (default: *no*) `(G/V)`

> Writing metadata xattr on directories with the sticky bit set may fail
even though we may have write access to a directory, because if the
sticky bit is set only the owner is allowed to write xattrs.

By enabling this option Netatalk will write the metadata xattr as root.

guest account = <name\> **(G)**

> Specifies the user that guests should use (default is nobody). The name
must be a valid user on the host system.

home name = <name\> **(H)**

> AFP user home volume name. The default is *$u's home*. The name must
contain "*$u*".

ignored attributes = <all | nowrite | nodelete | norename\> **(G)**/**(V)**

> Specify a set of file and directory attributes that shall be ignored by
the server, `all` includes all the other options.

In OS X when the Finder sets a lock on a file/directory or you set the
BSD uchg flag in the Terminal, all three attributes are used. Thus in
order to ignore the Finder lock/BSD uchg flag, add set *ignored
attributes = all*.

legacy icon = <icon\> **(G)**

> Sets the shared volume icon displayed in the Finder in Classic Mac OS.
Note that some versions of Classic Mac OS ignores this icon. Examples of
valid icon styles:

- **daemon**

- **globe**

- **sdcard**

login message = <message\> **(G)**/**(V)**

> Sets a message to be displayed when clients logon to the server. The
message should be in **unix charset**. Extended characters are allowed.

mimic model = <model\> **(G)**

> Specifies a custom icon for the mounted AFP volume on macOS / Mac OS X
clients. Default is to let macOS choose. Requires netatalk to be built
with Zeroconf. Examples:

- **Laptop**

- **RackMount**

- **Tower**

A complete set of supported model codes by a macOS client can be found
by inspecting
*/System/Library/CoreServices/CoreTypes.bundle/Contents/Info.plist* (as
of macOS 14 Sonoma).

signature = <STRING\> **(G)**

> Specify a server signature. The maximum length is 16 characters. This
option is useful for clustered environments, to provide fault isolation
etc. By default, afpd generates a signature and saves it to a file
called **afp_signature.conf** automatically (based on random numbers). See
also asip-status(1).

solaris share reservations = <BOOLEAN\> (default: *yes*) **(G)**

> Use share reservations on Solaris. Solaris CIFS server uses this too, so
this makes a lock coherent multi protocol server.

sparql results limit = <NUMBER\> (default: *UNLIMITED*) **(G)**

> Impose a limit on the number of results queried from Tracker or
LocalSearch via SPARQL queries.

spotlight = <BOOLEAN\> (default: *no*) **(G)**/**(V)**

> Whether to enable Spotlight searches. Note: once the global option is
enabled, any volume that is not enabled won't be searchable at all. See
also *dbus daemon* option.

spotlight attributes = <COMMA SEPARATED STRING\> (default: *EMPTY*) **(G)**

> A list of attributes that are allowed to be used in Spotlight searches.
By default all attributes can be searched, passing a string limits
attributes to elements of the string. Example:

    spotlight attributes = *,kMDItemTextContent

spotlight expr = <BOOLEAN\> (default: *yes*) **(G)**

> Whether to allow the use of logic expression in searches.

veto message = <BOOLEAN\> (default: *no*) **(G)**

> Send optional AFP messages for vetoed files. Then whenever a client
tries to access any file or directory with a vetoed name, it will be
sent an AFP message indicating the name and the directory.

vol dbpath = <path\> **(G)**/**(V)**

> Sets the path where the database information will be stored. You have to
specify a writable location, even if the volume is read only.

vol dbnest = <BOOLEAN\> (default: *no*) **(G)**

> Setting this option to true brings back Netatalk 2 behaviour of storing
the CNID database in a folder called .AppleDB inside the volume root of
each share.

volnamelen = <number\> **(G)**

> Max length of UTF8-MAC volume name for Mac OS X. Note that Hangul is
especially sensitive to this.

    73: limit of Mac OS X 10.1
    80: limit of Mac OS X 10.4/10.5 (default)
    255: limit of recent Mac OS X

Mac OS 9 and earlier are not influenced by this, because Maccharset
volume name is always limited to 27 bytes.

vol preset = <name\> **(G)**/**(V)**

> Use section <name\> as option preset for all volumes (when set in the
\[Global\] section) or for one volume (when set in that volume's
section).

zeroconf name = <name\> **(G)**

> Specifies a human-readable name that uniquely describes registered
services. The zeroconf name is advertised as UTF-8, up to 63 octets
(bytes) in length. Defaults to hostname. Note that netatalk must support
Zeroconf.

## Logging Options

log file = <logfile\> **(G)**

> Write logs to **logfile** on the file system. If not specified, Netatalk
logs to the syslog daemon facility.

log level = <type:level \[type:level ...\]\> **(G)**; log level = <type:level,\[type:level, ...\]\> **(G)**

> Specify that any message of a loglevel up to the given **log level**
should be logged.

By default afpd logs to syslog with a default logging setup equivalent
to **default:note**

logtypes: default, afpdaemon, logger, uamsdaemon

loglevels: severe, error, warn, note, info, debug, debug6, debug7,
debug8, debug9, maxdebug

> **NOTE**

> Both logtype and loglevels are case insensitive.

log microseconds = <BOOLEAN\> (default: *yes*) **(G)**

> Log timestamps with accuracy down to microseconds. If disabled, the
timestamps record only whole seconds. Only takes effect when used in
tandem with the **log file** option.

## Filesystem Change Events (FCE)

Netatalk includes a nifty filesystem change event mechanism where afpd
processes notify interested listeners about certain filesystem event by
UDP network datagrams.

The following FCE events are defined:

- file modification (**fmod**)

- file deletion (**fdel**)

- directory deletion (**ddel**)

- file creation (**fcre**)

- directory creation (**dcre**)

- file move or rename (**fmov**)

- directory move or rename (**dmov**)

- login (**login**)

- logout (**logout**)

fce listener = <host\[:port\]\> **(G)**

> Enables sending FCE events to the specified **host**, default **port** is
12250 if not specified. Specifying multiple listeners is done by having
this option once for each of them.

fce version = <1|2\> **(G)**

> FCE protocol version, default is 1. You need version 2 for the fmov,
dmov, login or logout events.

fce events = <fmod,fdel,ddel,fcre,dcre,fmov,dmov,login,logout\> **(G)**

> Specifies which FCE events are active, default is
**fmod,fdel,ddel,fcre,dcre**.

fce coalesce = <all|delete|create\> **(G)**

> Coalesce FCE events.

fce holdfmod = <seconds\> **(G)**

> This determines the time delay in seconds which is always waited if
another file modification for the same file is done by a client before
sending an FCE file modification event (fmod). For example saving a file
in Photoshop would generate multiple events by itself because the
application is opening, modifying and closing a file multiple times for
every "save". Default: 60 seconds.

fce sendwait = <milliseconds\> **(G)**

> Defines a delay in milliseconds between the emission of each FCE event.
Use this if you are experiencing lost FCE events when creating or
deleting a very large number of files at once. The deluge of events that
such an operation triggers can lead to UDP buffer overflow and
subsequently to packet loss. Has to be a number between 0 and 999.
Default: 0 milliseconds.

fce ignore names = <NAME\[/NAME2/...\]\> **(G)**

> Slash delimited list of filenames for which FCE events shall not be
generated. Default: .DS_Store.

fce ignore directories = <NAME\[,NAME2,...\]\> **(G)**

> Comma delimited list of directories for which FCE events shall not be
generated. Default: empty.

fce notify script = <PATH\> **(G)**

> Script which will be executed for every FCE event, see
contrib/shell_utils/fce_ev_script.sh from the Netatalk sources for an
example script.

## Debug Parameters

These options are useful for debugging only.

tickleval = <number\> **(G)**

> Sets the tickle timeout interval (in seconds). Defaults to 30.

timeout = <number\> **(G)**

> Specify the number of tickles to send before timing out a connection.
The default is 4, therefore a connection will timeout after 2 minutes.

client polling = <BOOLEAN\> (default: *no*) **(G)**

> With this option enabled, afpd won't advertise that it is capable of
server notifications, so that connected clients poll the server every 10
seconds to detect changes in opened server windows. *Note*: Depending on
the number of simultaneously connected clients and the network's speed,
this can lead to a significant higher load on your network!

Do not use this option any longer as present Netatalk correctly supports
server notifications, allowing connected clients to update folder
listings in case another client changed the contents.

## Options for ACL handling

By default, the effective permission of the authenticated user are only
mapped to the mentioned UARights permission structure, not the UNIX
mode. You can adjust this behaviour with the configuration option
**map acls**:

map acls = <none|rights|mode\> **(G)**

> none

> no mapping of ACLs

rights

> effective permissions are mapped to UARights structure. This is the
default.

mode

> ACLs are additionally mapped to the UNIX mode of the filesystem object.

If you want to be able to display ACLs on the client, you must setup
both client and server as part on a authentication domain (directory
service, e.g. LDAP, Open Directory, Active Directory). The reason is, in
OS X ACLs are bound to UUIDs, not just uid's or gid's. Therefore
Netatalk must be able to map every filesystem uid and gid to a UUID so
that it can return the server side ACLs which are bound to UNIX uid and
gid mapped to OS X UUIDs.

Netatalk can query a directory server using LDAP queries. Either the
directory server already provides an UUID attribute for user and groups
(Active Directory, Open Directory) or you reuse an unused attribute (or
add a new one) to you directory server (eg OpenLDAP).

The following LDAP options must be configured for Netatalk:

ldap auth method = <none|simple\> **(G)**

> Authentication method: **none** | **simple**

none

> anonymous LDAP bind

simple

> simple LDAP bind

ldap auth dn = <dn\> **(G)**

> Distinguished Name of the user for simple bind.

ldap auth pw = <password\> **(G)**

> Password for simple bind.

ldap uri = <ldap://somehost:1234/\> **(G)**

> Specifies the LDAP URI of the server to connect to. The URI scheme may
be ldap, ldapi or ldaps, specifying LDAP over TCP, ICP or TLS
respectively (if supported by the LDAP library). This is only needed for
explicit ACL support in order to be able to query LDAP for UUIDs.

You can use **afpldaptest**(1) to syntactically check your config.

ldap userbase = <base dn\> **(G)**

> DN of the user container in LDAP.

ldap userscope = <scope\> **(G)**

> Search scope for user search: **base** | **one** | **sub**

ldap groupbase = <base dn\> **(G)**

> DN of the group container in LDAP.

ldap groupscope = <scope\> **(G)**

> Search scope for group search: **base** | **one** | **sub**

ldap uuid attr = <dn\> **(G)**

> Name of the LDAP attribute with the UUIDs.

Note: this is used both for users and groups.

ldap name attr = <dn\> **(G)**

> Name of the LDAP attribute with the users short name.

ldap group attr = <dn\> **(G)**

> Name of the LDAP attribute with the groups short name.

ldap uuid string = <STRING\> **(G)**

> Format of the uuid string in the directory. A series of x and -, where
every x denotes a value 0-9a-f and every - is a separator.

Default: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

ldap uuid encoding = <string | ms-guid (default: string)\> **(G)**

> Format of the UUID of the LDAP attribute, allows usage of the binary
objectGUID fields from Active Directory. If left unspecified, string is
the default, which passes through the ASCII UUID returned by most other
LDAP stores. If set to ms-guid, the internal UUID representation is
converted to and from the binary format used in the objectGUID attribute
found on objects in Active Directory when interacting with the server.

See also the options **ldap user filter** and **ldap group filter**.

string

> UUID is a string, use with e.g. OpenDirectory.

ms-guid

> Binary objectGUID from Active Directory

ldap user filter = <STRING (default: unused)\> **(G)**

> Optional LDAP filter that matches user objects. This is necessary for
Active Directory environments where users and groups are stored in the
same directory subtree.

Recommended setting for Active Directory: **objectClass=user**.

ldap group filter = <STRING (default: unused)\> **(G)**

> Optional LDAP filter that matches group objects. This is necessary for
Active Directory environments where users and groups are stored in the
same directory subtree.

Recommended setting for Active Directory: **objectClass=group**.

# Explanation of Volume Parameters

## Parameters

The section name defines the volume name. No two volumes may have the
same name. The volume name cannot contain the ':' character. The volume
name is mangled if it is very long. Mac charset volume name is limited
to 27 characters. UTF8-MAC volume name is limited to volnamelen
parameter.

path = <PATH\> **(V)**

> The path name must be a fully qualified path name.

appledouble = <ea|v2\> **(V)**

> Specify the format of the metadata files, which are used for saving Mac
resource forks and extended attributes.
Earlier versions used AppleDouble **v2**, the new default format is **ea**.

> **WARNING**

> The **v2** option is obsolete and should not be used unless
absoluteley necessary. It may go away in the future.

vol size limit = <size in MiB\> **(V)**

> Useful for Time Machine: limits the reported volume size, thus
preventing Time Machine from using the whole real disk space for backup.
Example: "**vol size limit = 1000**" would limit the reported disk space to
1 GB.

> **IMPORTANT**

> This is an approximated calculation taking into
account the contents of Time Machine sparsebundle images. Therefore you
MUST NOT use this volume to store other content when using this option,
because it would NOT be accounted for. The calculation works by reading
the band size from the Info.plist XML file of the sparsebundle, reading
the bands/ directory counting the number of band files, and then
multiplying one with the other.

valid users = <user @group\> **(V)**

> The allow option allows the users and groups that access a share to be
specified. Users and groups are specified, delimited by spaces or
commas. Groups are designated by a @ prefix. Example:

    valid users = user @group

invalid users = <users/groups\> **(V)**

> The deny option specifies users and groups who are not allowed access to
the share. It follows the same format as the "valid users" option.

hosts allow = <IP host address/IP netmask bits \[ ... \]\> **(V)**

> Only listed hosts and networks are allowed, all others are rejected. The
network address may be specified either in dotted-decimal format for
IPv4 or in hexadecimal format for IPv6.

Example: hosts allow = 10.1.0.0/16 10.2.1.100 2001:0db8:1234::/48

hosts deny = <IP host address/IP netmask bits \[ ... \]\> **(V)**

> Listed hosts and nets are rejected, all others are allowed.

Example: hosts deny = 192.168.100/24 10.1.1.1 2001:db8::1428:57ab

cnid scheme = <backend\> **(V)**

> set the CNID backend to be used for the volume, default is
\[@DEFAULT_CNID_SCHEME@\] available schemes: \[@compiled_backends@\]

> **NOTE**

> The "mysql" backend requires the system administrator to configure a
MySQL database instance for use with netatalk.

> **WARNING**

> Do *NOT* use the "last" backend for volumes, because **afpd** relies
heavily on a persistent ID database. Aliases will likely not work and
filename mangling is not supported.

ea = <none|auto|sys|ad|samba\> **(V)**

> Specify how Extended Attributes are
stored. **auto** is the default.

auto

> Try **sys** (by setting an EA on the shared directory itself), fallback to
**ad**. Requires writable volume for performing test. "**read only = yes**"
overwrites **auto** with **none**. Use explicit "**ea = sys|ad**" for
read-only volumes where appropriate.

sys

> Use filesystem Extended Attributes.

samba

> Use filesystem Extended Attributes, but append a 0 byte to each xattr in
order to be compatible with Samba's vfs_streams_xattr.

ad

> Use files in *.AppleDouble* directories.

none

> No Extended Attributes support.

> **WARNING**

> The **samba** option should not be used on a volume that was previously
set to **sys**. This may lead data loss.

mac charset = <charset\> **(V)**

> specifies the Mac client charset for this Volume, e.g. *MAC_ROMAN*,
*MAC_CYRILLIC*. If not specified the global setting is applied. This
setting is only required if you need volumes, where the Mac charset
differs from the one globally set in the \[Global\] section.

casefold = <option\> **(V)**

> The casefold option handles, if the case of filenames should be changed.
The available options are:

**tolower** - Lowercases names in both directions.

**toupper** - Uppercases names in both directions.

**xlatelower** - Client sees lowercase, server sees uppercase.

**xlateupper** - Client sees uppercase, server sees lowercase.

password = <password\> **(V)**

> This option allows you to set a volume password, which can be a maximum
of 8 characters long (using ASCII strongly recommended at the time of
this writing).

file perm = <mode\> **(V)**; directory perm = <mode\> **(V)**

> Add(or) with the client requested permissions: **file perm** is for files
only, **directory perm** is for directories only. Don't use with
"**unix priv = no**".

## Example: Volume for a collaborative workgroup

    file perm = 0660
    directory perm = 0770

umask = <mode\> **(V)**

> set perm mask. Don't use with "**unix priv = no**".

preexec = <command\> **(V)**

> command to be run when the volume is mounted

postexec = <command\> **(V)**

> command to be run when the volume is closed

rolist = <users/groups\> **(V)**

> Allows certain users and groups to have read-only access to a share.
This follows the allow option format.

rwlist = <users/groups\> **(V)**

> Allows certain users and groups to have read/write access to a share.
This follows the allow option format.

veto files = <vetoed names\> **(V)**

> hide files and directories,where the path matches one of the '/'
delimited vetoed names. The veto string must always be terminated with a
'/', e.g. "**veto files = veto1/**", "**veto files = veto1/veto2/**".

## Volume options

Boolean volume options.

acls = <BOOLEAN\> (default: *yes*) **(V)**

> Whether to flag volumes as supporting ACLs. If ACL support is compiled
in, this is yes by default.

case sensitive = <BOOLEAN\> (default: *yes*) **(V)**

> Whether to flag volumes as supporting case-sensitive filenames. If the
filesystem is case-insensitive, set to no. However, it is not fully
verified.

> **NOTE**

> In spite of being case sensitive as a matter of fact, netatalk 3.1.3 and
earlier did not notify kCaseSensitive flag to the client. Starting with
3.1.4, it is notified correctly by default.

cnid dev = <BOOLEAN\> (default: *yes*) **(V)**

> Whether to use the device number in the CNID backends. Helps when the
device number is not constant across a reboot, e.g. cluster, ...

convert appledouble = <BOOLEAN\> (default: *yes*) **(V)**

> Whether automatic conversion from **appledouble = v2** to
**appledouble = ea** is performed when accessing filesystems from clients.
This is generally useful, but costs some performance. It's recommendable
to run **dbd** on volumes and do the conversion with that. Then this
option can be set to no.

delete veto files = <BOOLEAN\> (default: *no*) **(V)**

> This option is used when Netatalk is attempting to delete a directory
that contains one or more vetoed files or directories (see the veto
files option). If this option is set to no (the default) then if a
directory contains any non-vetoed files or directories then the
directory delete will fail. This is usually what you want.

If this option is set to yes, then Netatalk will attempt to recursively
delete any files and directories within the vetoed directory.

follow symlinks = <BOOLEAN\> (default: *no*) **(V)**

> The default setting is false thus symlinks are not followed on the
server. This is the same behaviour as OS X's AFP server. Setting the
option to true causes afpd to follow symlinks on the server. symlinks
may point outside of the AFP volume, currently afpd doesn't do any
checks for "wide symlinks".

> **NOTE**

> This option will subtly break when the symlinks point across filesystem
boundaries.

invisible dots = <BOOLEAN\> (default: *no*) **(V)**

> make dot files invisible. WARNING: enabling this option will lead to
unwanted side effects where OS X applications, when saving files to a
temporary file starting with a dot first, then renaming the temp file to
its final name, result in the saved file being invisible. The only thing
this option is useful for is making files that start with a dot
invisible on Mac OS 9. It's completely useless on Mac OS X, as both in
Finder and in Terminal files starting with a dot are hidden anyway.

legacy volume size = <BOOLEAN\> (default: *no*) **(V)**

> Limit disk size reporting to 2GB for legacy clients. This can be used
for older Macintoshes running System 7.1 or earlier and using newer
AppleShare clients.

network ids = <BOOLEAN\> (default: *yes*) **(V)**

> Whether the server support network ids. Setting this to *no* will result
in the client not using ACL AFP functions.

preexec close = <BOOLEAN\> (default: *no*) **(V)**

> A non-zero return code from preexec close the volume being immediately,
preventing clients to mount/see the volume in question.

prodos = <BOOLEAN\> (default: *no*) **(V)**

> Enable ProDOS support. This option should only be enabled for volumes
you expect to netboot an Apple II from. In addition to setting the boot
flag on the volume, it restricts the volume free space shown to 32MB.

read only = <BOOLEAN\> (default: *no*) **(V)**

> Specifies the share as being read only for all users. Overwrites
**ea = auto** with **ea = none**

search db = <BOOLEAN\> (default: *no*) **(V)**

> Use fast CNID database namesearch instead of slow recursive filesystem
search. Relies on a consistent CNID database, i.e. Samba or local
filesystem access lead to inaccurate or wrong results. Works only for
"dbd" CNID db volumes.

stat vol = <BOOLEAN\> (default: *yes*) **(V)**

> Whether to stat volume path when enumerating volumes list, useful for
automounting or volumes created by a preexec script.

time machine = <BOOLEAN\> (default: *no*) **(V)**

> Whether to enable Time Machine support for this volume.

unix priv = <BOOLEAN\> (default: *yes*) **(V)**

> Whether to use AFP3 UNIX privileges. This should be set for OS X
clients. See also: **file perm**, **directory perm** and `umask`.

# Examples

## Example: Modern Mac clients

This enables Spotlight and AFP stats if Netatalk was built with
Spotlight and AFP stats support. The **mimic model** option is
used to make the server look like an Xserve.

The home directory is mounted on */home/{user}/afp-data*.

```
[Global]
afpstats = yes
spotlight = yes
mimic model = RackMac

[Home]
basedir regex = /home
path = afp-data
```

## Example: Classic Mac clients

This enables AppleTalk if Netatalk was built with AppleTalk support.
The Random Number and ClearTxt authentication modules are used.
The **legacy icon** option is used to make the server look like a
BSD Daemon.

With **legacy volume size** the volume size is limited to 2 GB
for very old Macs, while **prodos** is used to enable ProDOS
boot flags on the volume while limiting the volume free space
to 32 MB.

```
[Global]
appletalk = yes
uam list = uams_dhx.so uams_dhx2.so uams_randnum.so uams_clrtxt.so
legacy icon = daemon

[mac]
name = Mac Volume
path = /srv/mac
legacy volume size = yes

[apple2]
name = Apple II Volume
path = /srv/apple2
prodos = yes
```

# See Also

afpd(8), afppasswd(5), afp_signature.conf(5), extmap.conf(5),
cnid_metad(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
