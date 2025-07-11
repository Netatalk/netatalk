# Setting up Netatalk

## Setting up the AFP file server

AFP (the Apple Filing Protocol) is a protocol used on the Apple Macintosh
for file services. The protocol has evolved over the years.
The final revision of the protocol, AFP 3.4, was introduced
with OS X Lion (10.7).

Netatalk's **afpd** daemon offers AFP fileservices to Apple clients. The
configuration is managed through the *afp.conf* file which uses an ini
style configuration syntax.

Netatalk provides compatibility with Time Machine for remote backups,
and Spotlight for indexed searching.

To make a volume a Time Machine target, use the
volume option **time machine = yes**.

To enable Spotlight indexing globally or for a volume,
set the option **spotlight = yes** where appropriate.

Starting with Netatalk 2.1, UNIX symlinks can be used on the server.
Semantics are the same as for e.g. NFS, i.e. they are not resolved
on the server side but instead it's completely up to the client
to resolve them, resulting in links that point somewhere inside
the client's filesystem view.

### afp.conf

*afp.conf* is the configuration file used by afpd to determine the
behaviour and configuration of the AFP file server and the AFP volume
that it provides.

The *afp.conf* is divided into several sections:

[Global]

> The global section defines general server options

[Homes]

> The homes section defines user home volumes

Any section not called **Global** or **Homes** is interpreted as an AFP
volume.

For sharing user homes by defining a **Homes** section you must specify
the option **basedir regex** which can be a simple string with the path to
the parent directory of all user homes or a regular expression.

Example:

    [Homes]
    basedir regex = /home

Now any user logging into the AFP server will have a user volume
available whose path is `/home/NAME`.

A more complex setup would be a server with a large amount of user homes
which are split across e.g. two different filesystems:

- /RAID1/homes

- /RAID2/morehomes

The following configuration is required:

    [Homes]
    basedir regex = /RAID./.*homes

If **basedir regex** contains a symlink, set the canonicalized absolute
path. When */home* links to */usr/home*:

    [Homes]
    basedir regex = /usr/home

For a more detailed explanation of the available options, please refer
to the [afp.conf](afp.conf.5.html) man page.

## CNID backends

Unlike other protocols like SMB or NFS, the AFP protocol mostly refers
to files and directories by ID and not by a path. These IDs are
called CNID, which stand for Catalog Node ID.
A typical AFP request uses a directory ID and a filename,
something like "server, please open the file named 'Test' in the
directory with id 167". For example "Aliases" on the Mac basically work
by ID (with a fallback to the absolute path in more recent AFP clients.
But this applies only to Finder, not to applications).

Every file in an AFP volume has to have a unique file ID.
CNIDs must, according to the AFP specification, never be reused.
The IDs are represented as 32 bit numbers, and directory IDs use the same
ID pool. So, after ~4 billion files/folders have been written to an AFP
volume, the ID pool is depleted and no new file can be written to the
volume. Some of Netatalk's CNID backends may attempt to reuse available
IDs after depletion, which is technically in violation of the spec,
but may enable continuous use on long-lived volumes.

Netatalk needs to map IDs to files and folders in the host filesystem.
To achieve this, several different CNID backends are available and can be
selected with the **cnid scheme** option in the *afp.conf* configuration file.
A CNID backend is basically a database storing ID <-\> name mappings.

For the CNID backends which use a zero-configuration database,
the database files are by default located in a *netatalk/CNID* subdirectory
of your system's state directory path, e.g. */var/lib*.
You can change the state directory path with *-Dwith-statedir-path=PATH*
at compile time.

There is a command line utility called **dbd** available which can be used
to verify, repair and rebuild the CNID database.

***NOTE:*** There are some CNID related things you should keep in mind when
working with netatalk:

- Don't nest volumes unless "**vol dbnest = yes**" is set.
- CNID backends are databases, so they turn afpd into a file
  server/database mix.
- If there's no more space on the filesystem left, the database will
  get corrupted. You can work around this by using the **vol dbpath**
  option and put the database files into another location.
- Be careful with CNID databases for volumes that are mounted via NFS.
  That is a pretty audacious decision to make anyway, but putting a
  database there as well is really asking for trouble, i.e. database
  corruption. Use the **vol dbpath** directive to put the databases onto
  a local disk if you must use NFS mounted volumes.

Below follows descriptions of the various CNID backends that are
included with netatalk.
You can choose to build one or several of them at compile time.
Run the command *afpd -v* to see which backends are available to you,
as well as which one is the default.

### dbd

The "Database Daemon" backend is built on Berkeley DB. Access to the
CNID database is restricted to the **cnid_dbd** daemon process.
**afpd** processes communicate with the **cnid_dbd** daemon
for database reads and updates, which is in turn launched and
controlled by the **cnid_metad** daemon.

This is the most reliable and proven backend for daily use.

### last

The last backend is an in-memory Trivial Database (tdb). It is not persistent,
with IDs valid only for the current session. Starting with netatalk 3.0,
it operates in *read only mode*. This backend is useful e.g. for
mounting CD-ROMs, or for automated testing.

This is basically equivalent to how **afpd** stored CNID data in netatalk
1.5 and earlier.

### mysql

CNID backend using a MySQL server. The MySQL server has to be
provisioned by the system administrator, and Netatalk configured
to connect to it over the network.

See **afp.conf**(5) for documentation of the configuration options.

### sqlite

A zero-configuration database backend that uses the SQLite v3
embedded database engine.

This backend is considered experimental and should only be used
for testing.

## Charsets/Unicode

### Why Unicode?

Internally, computers don't know anything about characters and texts,
they only know numbers. Therefore, each letter is assigned a number. A
character set, often referred to as *charset* or
*codepage*, defines the mappings between
numbers and letters.

If two or more computer systems need to communicate with each other, the
have to use the same character set. In the 1960s the
ASCII (American Standard Code for
Information Interchange) character set was defined by the American
Standards Association. The original form of ASCII represented 128
characters, more than enough to cover the English alphabet and numerals.
Up to date, ASCII has been the normative character scheme used by
computers.

Later versions defined 256 characters to produce a more international
fluency and to include some slightly esoteric graphical characters.
Using this mode of encoding each character takes exactly one byte.
Obviously, 256 characters still wasn't enough to map all the characters
used in the various languages into one character set.

As a result localized character sets were defined later, e.g the
ISO-8859 character sets. Most operating system vendors introduced their
own characters sets to satisfy their needs, e.g. IBM defined the
*codepage 437 (DOSLatinUS)*, Apple introduced the
*MacRoman* codepage and so on. The
characters that were assigned number larger than 127 were referred to as
*extended* characters. These character sets conflict with another, as
they use the same number for different characters, or vice versa.

Almost all of those characters sets defined 256 characters, where the
first 128 (0-127) character mappings are identical to ASCII. As a
result, communication between systems using different codepages was
effectively limited to the ASCII charset.

To solve this problem new, larger character sets were defined. To make
room for more character mappings, these character sets use at least 2
bytes to store a character. They are therefore referred to as
*multibyte* character sets.

One standardized multibyte charset encoding scheme is known as
[Unicode](http://www.unicode.org/). A big advantage of using a multibyte
charset is that you only need one. There is no need to make sure two
computers use the same charset when they are communicating.

### Character sets used by Apple

In the past, Apple clients used single-byte charsets to communicate over
the network. Over the years Apple defined a number of codepages, western
users will most likely be using the *MacRoman* codepage.

Codepages defined by Apple include:

- MacArabic, MacFarsi

- MacCentralEurope

- MacChineseSimple

- MacChineseTraditional

- MacCroatian

- MacCyrillic

- MacDevanagari

- MacGreek

- MacHebrew

- MacIcelandic

- MacJapanese

- MacKorean

- MacRoman

- MacRomanian

- MacThai

- MacTurkish

Starting with Mac OS X and AFP3, [UTF-8](http://www.utf-8.com/) is used.
UTF-8 encodes Unicode characters in an ASCII compatible way, each
Unicode character is encoded into 1-6 ASCII characters. UTF-8 is
therefore not really a charset itself, it's an encoding of the Unicode
charset.

To complicate things, Unicode defines several
*[normalization](http://www.unicode.org/reports/tr15/index.html)* forms.
While [samba](http://www.samba.org) uses *precomposed* Unicode,
which most UNIX tools prefer as well, Apple decided to use the
*decomposed* normalization.

For example lets take the character 'ä' (lowercase a with diaeresis).
Using the precomposed normalization, Unicode maps this character to 0xE4.
In decomposed normalization, 'ä' is actually mapped to two characters,
0x61 is the mapping for an 'a', 0x308 is the mapping for a *COMBINING
DIAERESIS*.

Netatalk refers to precomposed UTF-8 as *UTF8* and to decomposed UTF-8 as
*UTF8-MAC*.

### afpd and character sets

To support new AFP 3.x and older AFP 2.x clients at the same time, afpd
needs to be able to convert between the various charsets used. AFP 3.x
clients always use UTF8-MAC, while AFP 2.x clients use one of the Apple
codepages.

At the time of writing, netatalk supports the following Apple codepages:

- MAC_CENTRALEUROPE

- MAC_CHINESE_SIMP

- MAC_CHINESE_TRAD

- MAC_CYRILLIC

- MAC_GREEK

- MAC_HEBREW

- MAC_JAPANESE

- MAC_KOREAN

- MAC_ROMAN

- MAC_TURKISH

afpd handles three different character set options:

unix charset

> This is the codepage used internally by your operating system. If not
specified, it defaults to **UTF8**. If **LOCALE** is specified and your
system support UNIX locales, afpd tries to detect the codepage. afpd
uses this codepage to read its configuration files, so you can use
extended characters for volume names, login messages, etc.

mac charset

> As already mentioned, older Mac OS clients (up to AFP 2.2) use codepages
to communicate with afpd. However, there is no support for negotiating
the codepage used by the client in the AFP protocol. If not specified
otherwise, afpd assumes the *MacRoman* codepage is used. In case your
clients use another codepage, e.g. *MacCyrillic*, you'll **have** to
explicitly configure this.

vol charset

> This defines the charset afpd should use for filenames on disk. By
default, it is the same as **unix charset**. If you have
[iconv](http://www.gnu.org/software/libiconv/)
installed, you can use any iconv provided charset as well.

afpd needs a way to preserve extended Macintosh characters, or
characters illegal in UNIX filenames, when saving files on a UNIX
filesystem. Earlier versions used the the so called CAP
encoding. An extended character (\>0x7F)
would be converted to a :xx hex sequence, e.g. the Apple Logo (MacRoman:
0xF0) was saved as :f0. Some special characters will be converted as to
:xx notation as well. '/' will be encoded to :2f, if **usedots** was not
specified, a leading dot '.' will be encoded as :2e.

Even though this version now uses **UTF8** as the default encoding for
filenames, '/' will be converted to ':'. For western users another
useful setting could be **vol charset = ISO-8859-15**.

If a character cannot be converted from the **mac charset** to the
selected **vol charset**, you'll receive a -50 error on the Mac. *Note*:
Whenever you can, please stick with the default UTF8 volume format.

## Authentication

### AFP authentication basics

Apple chose a flexible model called "User Authentication
Modules" (UAMs) for authentication
purposes between AFP client and server. An AFP client initially
connecting to an AFP server will ask for the list of UAMs which the
server provides, and will choose the one with strongest encryption that
the client supports.

Several UAMs have been developed by Apple over the time, some by
3rd-party developers.

### UAMs supported by Netatalk

Netatalk supports the following ones by default:

- "No User Authent" UAM (guest access without authentication)

- "Cleartxt Passwrd" UAM (no password encryption)

- "Randnum exchange"/"2-Way Randnum exchange" UAMs (weak password
encryption, separate password storage)

- "DHCAST128" UAM (a.k.a. DHX; stronger password encryption)

- "DHX2" UAM (successor of DHCAST128)

There exist other optional UAMs as well:

- "Client Krb v2" UAM (Kerberos V, suitable for "Single Sign On" Scenarios
with macOS clients – see below)

You can configure which UAMs should be activated by defining
"**uam list**" in **Global** section. **afpd** will log which UAMs it's using
and if problems occur while activating them in either **netatalk.log** or
syslog at startup time. **asip-status** can be used to query the
available UAMs of AFP servers as well.

Having a specific UAM available at the server does not automatically
mean that a client can use it. Client-side support is also necessary.
For older Macintoshes running Classic Mac OS, DHCAST128 support exists
since AppleShare client 3.8.x.

On macOS, there exist some client-side techniques to make the AFP-client
more verbose, so one can have a look at what's happening while
negotiating the UAMs to use. Compare with this
[hint](https://web.archive.org/web/20080312054723/http://article.gmane.org/gmane.network.netatalk.devel/7383/).

### Which UAMs to activate?

It depends primarily on your needs and on the kind of macOS clients you
have to support. If your network consists of exclusively macOS (Mac OS
X) clients, DHX2 is sufficient, and provides the strongest encryption.

- Unless you really have to supply guest access to your server's volumes
ensure that you disable "No User Authent" since it might lead
accidentally to unauthorized access. In case you must enable guest
access take care that you enforce this on a per volume base using the
access controls.

    Note: "No User Authent" is required to use Apple II NetBoot services
    (**a2boot**) to boot an Apple //e over AFP.

- The "ClearTxt Passwrd" UAM is as bad as it sounds since passwords go
unencrypted over the wire. Try to avoid it at both the server's side
as well as on the client's.

    Note: If you want to provide Mac OS 8/9 clients with NetBoot-services
    then you need uams_cleartxt.so since the AFP-client integrated into
    the Mac's firmware can only deal with this basic form of
    authentication.

- Since "Randnum exchange"/"2-Way Randnum exchange" uses only 56 bit DES
for encryption it should be avoided as well. Another disadvantage is
the fact that the passwords have to be stored in cleartext on the
server and that it doesn't integrate into both PAM scenarios or
classic /etc/shadow (you have to administrate passwords separately by
using the **afppasswd** utility, in order for clients to use these
UAMs)

    However, this is the strongest form of authentication that can be used
    with Macintosh System Software 7.1 or earlier.

- "DHCAST128" ("DHX") or "DHX2" should be the sweet spot for most people
since it combines stronger encryption with PAM integration.

- Using the Kerberos V ("Client Krb v2")
UAM, it's possible to implement real single sign on scenarios using
Kerberos tickets. The password is not sent over the network. Instead,
the user password is used to decrypt a service ticket for the
AppleShare server. The service ticket contains an encryption key for
the client and some encrypted data (which only the AppleShare server
can decrypt). The encrypted portion of the service ticket is sent to
the server and used to authenticate the user. Because of the way that
the afpd service principal detection is implemented, this
authentication method is vulnerable to man-in-the-middle attacks.

For a more detailed overview over the technical implications of the
different UAMs, please have a look at Apple's [File Server
Security](http://developer.apple.com/library/mac/#documentation/Networking/Conceptual/AFP/AFPSecurity/AFPSecurity.html#//apple_ref/doc/uid/TP40000854-CH232-SW1)
pages.

### Using different authentication sources with specific UAMs

Some UAMs provide the ability to use different authentication
"backends", namely **uams_clrtxt.so**, **uams_dhx.so** and
**uams_dhx2.so**. They can use either classic UNIX passwords from
*/etc/passwd* (*/etc/shadow*) or PAM if the system supports that.
**uams_clrtxt.so** can be symlinked to either **uams_passwd.so** or
**uams_pam.so**, **uams_dhx.so** to **uams_dhx_passwd.so** or
**uams_dhx_pam.so** and **uams_dhx2.so** to **uams_dhx2_passwd.so** or
**uams_dhx2_pam.so**.

So, if it looks like this in Netatalk's UAMs folder (per default
*/etc/netatalk/uams/*) then you're using PAM, otherwise classic UNIX
passwords.

    uams_clrtxt.so -> uams_pam.so
    uams_dhx.so -> uams_dhx_pam.so
    uams_dhx2.so -> uams_dhx2_pam.so

The main advantage of using PAM is that one can integrate Netatalk in
centralized authentication scenarios, e.g. via LDAP, NIS and the like.
Please always keep in mind that the protection of your user's login
credentials in such scenarios also depends on the strength of encryption
that the UAM in question supplies. So think about eliminating weak UAMs like
"ClearTxt Passwrd" and "Randnum exchange" completely from your network.

### Netatalk UAM overview table

A small overview of the officially supported UAMs.

| UAM              | No User Auth  | Cleartxt Passwrd | RandNum Exchange | DHCAST128    | DHX2          | Kerberos V       |
|------------------|---------------|------------------|------------------|--------------|---------------|------------------|
| Password length  | guest access  | max 8 chars      | max 8 chars      | max 64 chars | max 256 chars | Kerberos tickets |
| Client support   | built-in into all Mac OS versions | built-in in all Mac OS versions except 10.0. Has to be activated explicitly in later Mac OS X versions | built-in into almost all Mac OS versions | built-in since AppleShare client 3.8.4, available as a plug-in for 3.8.3, integrated in macOS's AFP client | built-in since Mac OS X 10.2 | built-in since Mac OS X 10.2 |
| Encryption       | Enables guest access without authentication between client and server. | Password will be sent in cleartext over the wire. Just as bad as it sounds, therefore avoid at all if possible (note: providing NetBoot services requires the ClearTxt UAM) | 8-byte random numbers are sent over the wire, comparable with DES, 56 bits. Vulnerable to offline dictionary attack. Requires passwords in clear on the server. | Password will be encrypted with 128 bit SSL, user will be authenticated against the server but not vice versa. Therefore weak against man-in-the-middle attacks. | Password will be encrypted using libgcrypt with CAST 128 in CBC mode. User will be authenticated against the server but not vice versa. Therefore weak against man-in-the-middle attacks. | Password is not sent over the network. Due to the service principal detection method, this authentication method is vulnerable to man-in-the-middle attacks. |
| Server support   | uams_guest.so | uams_cleartxt.so | uams_randnum.so  | uams_dhx.so  | uams_dhx2.so  | uams_gss.so      |
| Password storage | None          | Either system auth or PAM | Passwords stored in clear text in a separate text file | Either system auth or PAM | Either system auth or PAM | At the Kerberos Key Distribution Center |

### SSH tunneling

Tunneling and VPNs usually have nothing to do with AFP authentication
and UAMs. But since Apple introduced an option called "Allow Secure
Connections Using SSH" and many people tend to confuse the two, we'll
cover this here.

#### Manually tunneling an AFP session

This works since the first AFP servers that spoke "AFP over TCP"
appeared in networks. One simply tunnels the remote server's AFP port to
a local port different than 548 and connects locally to this port
afterwards. On macOS this can be done by

    ssh -l $USER $SERVER -L 10548:127.0.0.1:548 sleep 3000

After establishing the tunnel one will use `"afp://127.0.0.1:10548"` in
the "Connect to server" dialog. All AFP traffic including the initial
connection attempts will be sent encrypted over the wire since the local
AFP client will connect to the Mac's local port 10548 which will be
forwarded to the remote server's AFP port (we used the default 548) over
SSH.

This sort of tunnel is an ideal solution if you must access an AFP
server through the Internet without having the ability or desire to use
a "real" VPN. Note that you can let **ssh** compress the data by using its
"-C" switch and that the tunnel endpoints can be different from both AFP
client and server (compare with the SSH documentation for details).

#### Automatically establishing a tunneled AFP connection

From Mac OS X 10.2 to 10.4, Apple added an "Allow Secure Connections
Using SSH" checkbox to the "Connect to Server" dialog. The idea behind
this was: When the server signals that it can be contacted by SSH then
macOS's AFP client tries to establish the tunnel and automagically send
all AFP traffic through it.

But it took until the release of Mac OS X 10.3 that this feature worked
the first time... partly. In case, the SSH tunnel could not be
established and the AFP client **silently** fell back to an unencrypted
AFP connection attempt.

Netatalk's afpd will report that it is capable of handling SSH tunneled
AFP requests, when both "**advertise ssh**" and "**fqdn**" options are set
in the **Global** section (double check with **asip-status** after you
restarted afpd when you made changes to the settings). But there are a
couple of reasons why you don't want to use this option at all:

- Most users who need such a feature are probably already familiar with
using a VPN; it might be easier for the user to employ the same VPN
software in order to connect to the network on which the AFP server is
running, and then to access the AFP server as normal.

    That being said, for the simple case of connecting to one specific AFP
    server, a direct SSH connection is likely to perform better than a
    general-purpose VPN; contrary to popular belief, tunneling via SSH
    does **not** result in what's called "TCP-over-TCP meltdown", because
    the AFP data that are being tunneled do not encapsulate TCP data.

- Since this SSH kludge isn't a normal UAM that integrates directly into
the AFP authentication mechanisms but instead uses a single flag
signalling clients whether they can **try** to establish a tunnel or
not, it makes life harder to see what's happening when things go
wrong.

- You cannot control which machines are logged on by Netatalk tools like
a **macusers** since all connection attempts seem to be made from
localhost.

- Indeed, to ensure that all AFP sessions are encrypted via SSH, you
need to limit afpd to connections that originate only from localhost
(e.g., by using Wietse Venema's TCP Wrappers, or by using suitable
firewall or packet-filtering facilities, etc.).

    Otherwise, when you're using Mac OS X 10.2 through 10.3.3, you get the
    opposite of what you'd expect: potentially unencrypted AFP
    communications (including login credentials) being sent across the
    network without a single notification that establishing the tunnel
    failed. Apple fixed that with Mac OS X 10.3.4.

- Encrypting all AFP sessions via SSH can lead to a significantly higher
load on the computer that is running the AFP server, because that
computer must also handle encryption; if the user is connecting
through a trusted network, then such encryption might be an
unnecessary overhead.

## ACL Support

ACL support for AFP is implemented for ZFS ACLs on Solaris and derived
platforms and for POSIX 1e ACLs on Linux.

### Configuration

For a basic mode of operation there's nothing to configure. Netatalk
reads ACLs on the fly and calculates effective permissions which are
then send to the AFP client via the so called
UARights permission bits. On a Mac, the
Finder uses these bits to adjust permission in Finder windows. Example:
a folder whose UNIX mode is read-only and an ACL giving the user write
access, will display the effective read-write permission. Without the
permission mapping the Finder would display a read-only icon and the
user wouldn't be able to write to the folder.

By default, the effective permission of the authenticated user are only
mapped to the mentioned UARightspermission structure, not the UNIX mode.
You can adjust this behaviour with the configuration option
[map acls](afp.conf#options-for-acl-handling).

However, neither in Finder "Get Info" windows nor in the Terminal will
you be able to see the ACLs, because of how ACLs in macOS are designed.
If you want to be able to display ACLs on the client, things get more
involved as you must then setup both client and server to be part on a
authentication domain (directory service, e.g. LDAP, OpenDirectory). The
reason is, that in macOS ACLs are bound to UUIDs, not just uid's or
gid's. Therefore afpd must be able to map every filesystem uid and gid
to a UUID so that it can return the server side ACLs which are bound to
UNIX uid and gid mapped to macOS UUIDs.

Netatalk can query a directory server using LDAP queries. Either the
directory server already provides an UUID attribute for user and groups
(Active Directory, Open Directory) or you reuse an unused attribute (or
add a new one) to you directory server (e.g. OpenLDAP).

In detail:

1. For Solaris/ZFS: ZFS Volumes

    You should configure a ZFS ACL know for any volume you want to use
    with Netatalk:

        aclinherit = passthrough
        aclmode = passthrough

    For an explanation of what this knob does and how to apply it, check
    your hosts ZFS documentation (e.g. man zfs).

2. Authentication Domain

    Your server and the clients must be part of a security association
    where identity data is coming from a common source. ACLs in Darwin
    are based on UUIDs and so is the ACL specification in AFP 3.2.
    Therefore your source of identity data has to provide an attribute
    for every user and group where a UUID is stored as a ASCII string.
    In other words:

    - you need an Open Directory Server or an LDAP server where you
      store UUIDs in some attribute

    - your clients must be configured to use this server

    - your server should be configured to use this server via nsswitch
      and PAM

    - configure Netatalk via the special [LDAP options for
      ACLs](afp.conf.html#options-for-acl-handling) in *afp.conf* so that Netatalk is
      able to retrieve the UUID for users and groups via LDAP search
      queries

### macOS ACLs

With Access Control Lists (ACLs), macOS offers a powerful extension of
the traditional UNIX permissions model. An ACL is an ordered list of
Access Control Entries (ACEs) explicitly granting or denying a set of
permissions to a given user or group.

Unlike UNIX permissions, which are bound to user or group IDs, ACLs are
tied to UUIDs. For this reason accessing an object's ACL requires server
and client to use a common directory service which translates between
UUIDs and user/group IDs.

ACLs and UNIX permissions interact in a rather simple way. As ACLs are
optional UNIX permissions act as a default mechanism for access control.
Changing an objects's UNIX permissions will leave its ACL intact and
modifying an ACL will never change the object's UNIX permissions. While
doing access checks, macOS first examines an object's ACL evaluating
ACEs in order until all requested rights have been granted, a requested
right has been explicitly denied by an ACE or the end of the list has
been reached. In case there is no ACL or the permissions granted by the
ACL are not sufficient to fulfill the request, macOS next evaluates the
object's UNIX permissions. Therefore ACLs always have precedence over
UNIX permissions.

### ZFS ACLs

ZFS ACLs closely match macOS ACLs. Both offer mostly identical fine
grained permissions and inheritance settings.

### POSIX ACLs

#### Overview

Compared to macOS or NFSv4 ACLs, POSIX ACLs represent a different, less
versatile approach to overcome the limitations of the traditional UNIX
permissions. Implementations are based on the withdrawn POSIX 1003.1e
standard.

The standard defines two types of ACLs. Files and directories can have
access ACLs which are consulted for access checks. Directories can also
have default ACLs irrelevant to access checks. When a new object is
created inside a directory with a default ACL, the default ACL is
applied to the new object as its access ACL. Subdirectories inherit
default ACLs from their parent. There are no further mechanisms of
inheritance control.

Architectural differences between POSIX ACLs and macOS ACLs especially
involve:

- No fine-granular permissions model. Like UNIX permissions POSIX ACLs
only differentiate between read, write and execute permissions.

- Entries within an ACL are unordered.

- POSIX ACLs can only grant rights. There is no way to explicitly deny
rights by an entry.

- UNIX permissions are integrated into an ACL as special entries.

POSIX 1003.1e defines 6 different types of ACL entries. The first three
types are used to integrate standard UNIX permissions. They form a
minimal ACL, their presence is mandatory and only one entry of each type
is allowed within an ACL.

- ACL_USER_OBJ: the owner's access rights.

- ACL_GROUP_OBJ: the owning group's access rights.

- ACL_OTHER: everybody's access rights.

The remaining entry types expand the traditional permissions model:

- ACL_USER: grants access rights to a certain user.

- ACL_GROUP: grants access rights to a certain group.

- ACL_MASK: limits the maximum access rights which can be granted by
entries of type ACL_GROUP_OBJ, ACL_USER and ACL_GROUP. As the name
suggests, this entry acts as a mask. Only one ACL_MASK entry is
allowed per ACL. If an ACL contains ACL_USER or ACL_GROUP entries, an
ACL_MASK entry must be present too, otherwise it is optional.

In order to maintain compatibility with applications not aware of ACLs,
POSIX 1003.1e changes the semantics of system calls and utilities which
retrieve or manipulate an object's UNIX permissions. In case an object
only has a minimal ACL, the group permissions bits of the UNIX
permissions correspond to the value of the ACL_GROUP_OBJ entry.

However, if the ACL also contains an ACL_MASK entry, the behavior of
those system calls and utilities is different. The group permissions
bits of the UNIX permissions correspond to the value of the ACL_MASK
entry, i. e. calling "chmod g-w" will not only revoke write access for
the group, but for all entities which have been granted write access by
ACL_USER or ACL_GROUP entries.

#### Mapping POSIX ACLs to macOS ACLs

When a client wants to read an object's ACL, afpd maps its POSIX ACL
onto an equivalent macOS ACL. Writing an object's ACL requires afpd to
map an macOS ACL onto a POSIX ACL. Due to architectural restrictions of
POSIX ACLs, it is usually impossible to find an exact mapping so that
the result of the mapping process will be an approximation of the
original ACL's semantic.

- afpd silently discard entries which deny a set of permissions because
they they can't be represented within the POSIX architecture.

- As entries within POSIX ACLs are unordered, it is impossible to
preserve order.

- Inheritance control is subject to severe limitations as well:

  - Entries with the only_inherit flag set will only become part of the
    directory's default ACL.

  - Entries with at least one of the flags file_inherit,
    directory_inherit or limit_inherit set, will become part of the
    directory's access and default ACL, but the restrictions they impose
    on inheritance will be ignored.

- The lack of a fine-granular permission model on the POSIX side will
normally result in an increase of granted permissions.

As macOS clients aren't aware of the POSIX 1003.1e specific relationship
between UNIX permissions and ACL_MASK, afpd does not expose this feature
to the client to avoid compatibility issues and handles \*unix
permissions and ACLs the same way as Apple's reference implementation of
AFP does. When an object's UNIX permissions are requested, afpd
calculates proper group rights and returns the result together with the
owner's and everybody's access rights to the caller via "permissions"
and "ua_permissions" members of the FPUnixPrivs structure (see Apple
Filing Protocol Reference, page 181). Changing an object's permissions,
afpd always updates ACL_USER_OBJ, ACL_GROUP_OBJ and ACL_OTHERS. If an
ACL_MASK entry is present too, afpd recalculates its value so that the
new group rights become effective and existing entries of type ACL_USER
or ACL_GROUP stay intact.

## Filesystem Change Events

Netatalk includes a nifty filesystem change event (FCE) mechanism where
afpd processes notify interested listeners about certain filesystem
events by UDP network datagrams.

This feature is disabled by default. To enable it, set the **fce listener**
option in afp.conf to the hostname or IP address of the host that should
listen for FCE events.

Netatalk distributes a simple FCE listener application called **fce_listen**.
It will print out any UDP datagrams received from the AFP server.
Its source code also serves as documentation for the format of the UDP packets.

### FCE v1

The currently supported FCE v1 events are:

- file modification (fmod)

- file deletion (fdel)

- directory deletion (ddel)

- file creation (fcre)

- directory creation (dcre)

### FCE v2

FCE v2 events provide additional context such as the user who performed the
action. When using FCE v2 you also get the following events:

- file moving (fmov)

- directory moving (dmov)

- user login (login)

- user logout (logout)

## Spotlight Compatible Search

You can enable Netatalk's Spotlight compatible search and indexing
either globally or on a per volume basis with the **spotlight** option.

> ***WARNING:*** Once Spotlight is enabled for a single volume,
all other volumes for which spotlight is disabled won't be searchable at all.

The **dbus-daemon** binary has to be installed for Spotlight feature. The
path to dbus-daemon is determined at compile time the dbus-daemon build
system option.

In case the **dbus-daemon** binary is installed at the other path, you
must use the global option **dbus daemon** to point to the path, e.g. for
Solaris with Tracker from OpenCSW:

    dbus daemon = /opt/csw/bin/dbus-daemon
