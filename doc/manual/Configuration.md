# Setting up Netatalk

## Setting up the AFP file server

AFP (the Apple Filing Protocol) is a protocol originally created for Apple Macintosh file services.
The final revision of the protocol, AFP 3.4, was introduced with OS X Lion (10.7).

Netatalk's **afpd** daemon provides AFP file services to clients, including Macs, Apple IIs, and other AFP clients.
Configuration is managed through the *afp.conf* file, which uses an ini-style syntax.

Netatalk provides compatibility with Time Machine for remote backups
and Spotlight for indexed searching.

To make a volume a Time Machine target, set the
volume option **time machine = yes**.

To enable Spotlight indexing globally or for a specific volume,
set the option **spotlight = yes** where appropriate.

Starting with Netatalk 2.1, UNIX symlinks are supported on the server.
The semantics are the same as for NFS: symlinks are not resolved
on the server side but are instead passed to the client
for resolution, resulting in links that point somewhere inside
the client's filesystem view.

### afp.conf

*afp.conf* is the configuration file used by **afpd** to determine the
behavior and configuration of the AFP file server and the AFP volumes
that it provides.

*afp.conf* is divided into several sections:

[Global]

> The global section defines general server options.

[Homes]

> The homes section defines user home volumes.

Any section not named **Global** or **Homes** is interpreted as an AFP
volume.

To share user homes, define a **Homes** section and specify
the **basedir regex** option. This can be a simple path to
the parent directory of all user homes, or a regular expression.

Example:

    [Homes]
    basedir regex = /home

Any user logging into the AFP server will now have a user volume
available at the path `/home/NAME`.

A more complex setup involves a server with many user homes
split across two different filesystems:

- /RAID1/homes

- /RAID2/morehomes

The following configuration handles this:

    [Homes]
    basedir regex = /RAID./.*homes

If **basedir regex** contains a symlink, set the canonicalized absolute
path. For example, when */home* links to */usr/home*:

    [Homes]
    basedir regex = /usr/home

For a detailed explanation of all available options, refer
to the [afp.conf](afp.conf.5.html) man page.

## CNID backends

Unlike other protocols such as SMB or NFS, the AFP protocol mostly refers
to files and directories by ID rather than by path. These IDs are
called CNIDs (Catalog Node IDs).
A typical AFP request uses a directory ID and a filename,
for example: "server, please open the file named 'Test' in the
directory with ID 167". Mac Aliases work
by ID, with a fallback to the absolute path in more recent AFP clients.
(This fallback applies only to the Finder, not to applications.)

Every file in an AFP volume must have a unique file ID.
According to the AFP specification, CNIDs must never be reused.
IDs are represented as 32-bit numbers, and directories share the same
ID pool. After approximately 4 billion files and directories have been written to an AFP
volume, the ID pool is depleted and no new files can be written to the
volume. Some of Netatalk's CNID backends may attempt to reuse available
IDs after depletion, which technically violates the spec
but may allow continuous use on long-lived volumes.

Netatalk maps IDs to files and directories in the host filesystem.
Several CNID backends are available and can be
selected with the **cnid scheme** option in *afp.conf*.
A CNID backend is essentially a database storing ID ↔ name mappings.

For CNID backends that use a zero-configuration database,
the database files are located by default in a *netatalk/CNID* subdirectory
of your system's state directory path, e.g. */var/lib*.
You can change the state directory path with *-Dwith-statedir-path=PATH*
at compile time.

The **dbd** command-line utility can verify, repair, and rebuild the CNID database.

> **NOTE:** Keep the following CNID-related considerations in mind:

- Don't nest volumes unless **vol dbnest = yes** is set.
- CNID backends are databases, so they make **afpd** a combined file
  server and database system.
- If the filesystem runs out of space, the database will
  become corrupted. You can work around this by using the **vol dbpath**
  option to store database files in a different location.
- Be careful with CNID databases on NFS-mounted volumes.
  Serving AFP volumes over NFS is already risky, and placing a
  database there compounds the risk of corruption. Use the **vol dbpath**
  option to store databases on a local disk if you must use NFS-mounted volumes.

The following sections describe the CNID backends included with Netatalk.
You can choose to build one or several of them at compile time.
Run `afpd -v` to see which backends are available
and which one is the default.

### dbd

The "Database Daemon" backend is built on Berkeley DB.
Rather than accessing the database directly,
**afpd** processes communicate with one or more **cnid_dbd** daemon processes over a wire protocol.
The **cnid_dbd** daemon is responsible for database reads and updates.

This is the most reliable and proven backend, recommended for most deployments.

### mysql

CNID backend using a MySQL or MariaDB server.
This is the most performant and scalable backend option,
and is recommended for large deployments with many concurrent users.

The SQL server must be provisioned by the system administrator,
and Netatalk configured to connect to it over a Unix socket or a network interface.

### sqlite

A fast and lightweight CNID database backend that uses the SQLite v3 embedded database engine.
It is as easy to set up as dbd but does not require separate daemon processes,
as the database is accessed directly by **afpd**.

The tradeoff is that SQLite's embedded database is not the most scalable
for concurrent write operations.
It is therefore recommended for deployments with up to 20 concurrent
users, or for admins who do not want to set up MySQL or MariaDB.

## Charsets/Unicode

### Why Unicode?

Internally, computers only work with numbers, not characters.
Each letter is assigned a number, and a
character set — often called a *charset* or
*codepage* — defines the mappings between
numbers and letters.

When two or more computer systems communicate, they
must use the same character set. In the 1960s, the
ASCII (American Standard Code for
Information Interchange) character set was defined by the American
Standards Association. The original form of ASCII represented 128
characters, more than enough to cover the English alphabet and numerals.
To this day, ASCII has been the foundational character encoding used by
computers.

Later extensions defined 256 characters to accommodate international
characters and some graphical symbols.
In this mode of encoding, each character takes exactly one byte.
Obviously, 256 characters was still not enough to represent all the characters
used across the world's languages in a single character set.

As a result, localized character sets were defined, such as the
ISO-8859 family. Most operating system vendors introduced their
own character sets: IBM defined
*codepage 437 (DOSLatinUS)*, Apple introduced
*MacRoman*, and so on. Characters
assigned numbers larger than 127 were referred to as
*extended* characters. These character sets conflict with one another, as
they use the same number for different characters, or vice versa.

Almost all of these character sets defined 256 characters, with the
first 128 (0–127) mappings identical to ASCII. As a
result, communication between systems using different codepages was
effectively limited to the ASCII subset.

To solve this problem, new, larger character sets were defined. To make
room for more mappings, these character sets use at least 2
bytes per character. They are therefore referred to as
*multibyte* character sets.

One standardized multibyte encoding scheme is
[Unicode](http://www.unicode.org/). A key advantage of using a multibyte
charset is that you only need one. There is no need to ensure that two
computers use the same charset when communicating.

### Character sets used by Apple

In the past, Apple clients used single-byte charsets for network
communication. Over the years Apple defined a number of codepages; western
users most commonly used the *MacRoman* codepage.

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

Starting with Mac OS X and AFP 3.x, [UTF-8](http://www.utf-8.com/) is used.
UTF-8 encodes Unicode characters in an ASCII-compatible way; each
Unicode character is encoded into 1–6 bytes. UTF-8 is
therefore not a charset itself, but rather an encoding of the Unicode
charset.

To complicate matters, Unicode defines several
*[normalization](http://www.unicode.org/reports/tr15/index.html)* forms.
While [Samba](http://www.samba.org) uses *precomposed* Unicode
(which most UNIX tools prefer as well), Apple chose
*decomposed* normalization.

For example, take the character 'ä' (lowercase a with diaeresis).
Using precomposed normalization, Unicode maps this character to 0xE4.
In decomposed normalization, 'ä' is mapped to two characters:
0x61 for 'a' and 0x308 for *COMBINING DIAERESIS*.

Netatalk refers to precomposed UTF-8 as *UTF8* and to decomposed UTF-8 as
*UTF8-MAC*.

### afpd and character sets

To support both AFP 3.x and older AFP 2.x clients simultaneously, **afpd**
must convert between the various charsets in use. AFP 3.x
clients always use UTF8-MAC, while AFP 2.x clients use one of the Apple
codepages.

At the time of writing, Netatalk supports the following Apple codepages:

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

**afpd** handles three different character set options:

unix charset

> The codepage used internally by your operating system. If not
specified, it defaults to **UTF8**. If **LOCALE** is specified and your
system supports UNIX locales, **afpd** tries to detect the codepage. **afpd**
uses this codepage to read its configuration files, so you can use
extended characters for volume names, login messages, etc.

mac charset

> Older Mac OS clients (up to AFP 2.2) use codepages
to communicate with **afpd**. However, the AFP protocol has no support for negotiating
the codepage in use. If not specified
otherwise, **afpd** assumes *MacRoman*. If your
clients use a different codepage, such as *MacCyrillic*, you **must**
configure this explicitly.

vol charset

> The charset that **afpd** uses for filenames on disk. By
default, this is the same as **unix charset**. If you have
[iconv](http://www.gnu.org/software/libiconv/)
installed, you can use any iconv-provided charset as well.

**afpd** needs a way to preserve extended Macintosh characters, or
characters illegal in UNIX filenames, when saving files on a UNIX
filesystem. Earlier versions used the so-called CAP
encoding. An extended character (>0x7F)
was converted to a `:xx` hex sequence — for example, the Apple Logo (MacRoman:
0xF0) was saved as `:f0`. Some special characters are also converted to
`:xx` notation: `/` is encoded as `:2f`, and if **usedots** is not
specified, a leading dot `.` is encoded as `:2e`.

Even though this version now uses **UTF8** as the default encoding for
filenames, `/` is still converted to `:`. For western users, another
useful setting is **vol charset = ISO-8859-15**.

If a character cannot be converted from the **mac charset** to the
selected **vol charset**, you will receive a -50 error on the Mac. *Note*:
Whenever possible, use the default UTF8 volume format.

## Authentication

### AFP authentication basics

Apple chose a flexible model called "User Authentication
Modules" (UAMs) for authentication
between AFP clients and servers. An AFP client connecting
to an AFP server first requests the list of UAMs that the
server supports, then chooses the one with the strongest encryption that
the client also supports.

Several UAMs have been developed by Apple over the years, and some by
third-party developers.

### UAMs supported by Netatalk

Netatalk supports the following UAMs by default:

- "No User Authent" UAM (guest access without authentication)

- "Cleartxt Passwrd" UAM (no password encryption)

- "Randnum exchange"/"2-Way Randnum exchange" UAMs (weak password
encryption, separate password storage)

- "DHCAST128" UAM (a.k.a. DHX; stronger password encryption)

- "DHX2" UAM (successor of DHCAST128)

With Kerberos support enabled at compile time, Netatalk also supports:

- "Client Krb v2" UAM (Kerberos V, suitable for "Single Sign On" scenarios
with macOS clients — see below)

You can configure which UAMs to activate by setting
**uam list** in the **Global** section. **afpd** logs which UAMs it activates
and reports any problems in either **netatalk.log** or
syslog at startup. **asip-status** can also query the
available UAMs of AFP servers.

Having a specific UAM available on the server does not automatically
mean that a client can use it. Client-side support is also required.
For older Macintoshes running Classic Mac OS, DHCAST128 support has been available
since AppleShare client 3.8.x.

On macOS, there are client-side techniques to make the AFP client
more verbose, allowing you to observe the UAM negotiation process. See this
[hint](https://web.archive.org/web/20080312054723/http://article.gmane.org/gmane.network.netatalk.devel/7383/).

### Which UAMs to activate?

The choice depends primarily on your needs and the macOS clients you
need to support. If your network consists exclusively of macOS
clients, DHX2 is sufficient and provides the strongest encryption.

- Unless you specifically need guest access,
  disable "No User Authent" to prevent
  accidental unauthorized access. If you must enable guest
  access, enforce it on a per-volume basis using
  access controls.

    Note: "No User Authent" is required to use Apple II network boot services
    (**a2boot**) to boot an Apple //e over AFP.

- The "ClearTxt Passwrd" UAM is as bad as it sounds: passwords are sent
  unencrypted over the wire. Avoid it on both the server and client side.

    Note: If you want to provide Mac OS 8/9 clients with network boot services,
    you need uams_cleartxt.so since the AFP client integrated into
    the Mac's firmware can only handle this basic form of
    authentication.

- "Randnum exchange"/"2-Way Randnum exchange" uses only 56-bit DES
  for encryption, so it should also be avoided. An additional disadvantage is
  that passwords must be stored in cleartext on the server, and these UAMs
  do not integrate with PAM or classic /etc/shadow. You must
  administer passwords separately using the **afppasswd** utility.

    However, this is the strongest form of authentication available for
    Macintosh System Software 7.1 or earlier.

- "DHCAST128" ("DHX") or "DHX2" is the best choice for most users,
  combining stronger encryption with PAM integration.

- The Kerberos V ("Client Krb v2")
  UAM enables true single sign-on scenarios using
  Kerberos tickets. The password is not sent over the network. Instead,
  the user's password decrypts a service ticket for the
  AppleShare server. The service ticket contains an encryption key for
  the client and some encrypted data (which only the AppleShare server
  can decrypt). The encrypted portion of the service ticket is sent to
  the server and used to authenticate the user. Because of how
  **afpd** implements service principal detection, this
  authentication method is vulnerable to man-in-the-middle attacks.

For a more detailed overview of the technical implications of the
different UAMs, see Apple's [File Server
Security](http://developer.apple.com/library/mac/#documentation/Networking/Conceptual/AFP/AFPSecurity/AFPSecurity.html#//apple_ref/doc/uid/TP40000854-CH232-SW1)
documentation.

### Using different authentication sources with specific UAMs

Some UAMs support different authentication
backends, namely **uams_clrtxt.so**, **uams_dhx.so**, and
**uams_dhx2.so**. They can use either classic UNIX passwords from
*/etc/passwd* (*/etc/shadow*) or PAM if the system supports it.
**uams_clrtxt.so** can be symlinked to either **uams_passwd.so** or
**uams_pam.so**, **uams_dhx.so** to **uams_dhx_passwd.so** or
**uams_dhx_pam.so**, and **uams_dhx2.so** to **uams_dhx2_passwd.so** or
**uams_dhx2_pam.so**.

If Netatalk's UAM folder (by default
*/etc/netatalk/uams/*) looks like this, then you are using PAM; otherwise you are using classic UNIX
passwords:

    uams_clrtxt.so -> uams_pam.so
    uams_dhx.so -> uams_dhx_pam.so
    uams_dhx2.so -> uams_dhx2_pam.so

The main advantage of PAM is that it integrates Netatalk into
centralized authentication scenarios, such as LDAP, NIS, and similar systems.
Keep in mind that the security of your users' login
credentials in such scenarios also depends on the encryption strength
of the UAM in use. Consider eliminating weak UAMs like
"ClearTxt Passwrd" and "Randnum exchange" entirely from your network.

### Netatalk UAM overview table

An overview of the officially supported UAMs on Macs.

| UAM              | No User Auth  | Cleartxt Passwrd | RandNum Exchange | DHCAST128    | DHX2          | Kerberos V       |
|------------------|---------------|------------------|------------------|--------------|---------------|------------------|
| Password length  | guest access  | max 8 chars      | max 8 chars      | max 64 chars | max 256 chars | Kerberos tickets |
| Client support   | built-in into all Mac OS versions | built-in in all Mac OS versions except 10.0. Has to be activated explicitly in later Mac OS X versions | built-in into almost all Mac OS versions | built-in since AppleShare client 3.8.4, available as a plug-in for 3.8.3, integrated in macOS's AFP client | built-in since Mac OS X 10.2 | built-in since Mac OS X 10.2 |
| Encryption       | Enables guest access without authentication between client and server. | Password will be sent in cleartext over the wire. Just as bad as it sounds, therefore avoid at all if possible (note: providing NetBoot services requires the ClearTxt UAM) | 8-byte random numbers are sent over the wire, comparable with DES, 56 bits. Vulnerable to offline dictionary attack. Requires passwords in clear on the server. | Password will be encrypted with 128 bit CAST, user will be authenticated against the server but not vice versa. Therefore weak against man-in-the-middle attacks. | Password will be encrypted with 128 bit CAST in CBC mode. User will be authenticated against the server but not vice versa. Therefore weak against man-in-the-middle attacks. | Password is not sent over the network. Due to the service principal detection method, this authentication method is vulnerable to man-in-the-middle attacks. |
| Server support   | uams_guest.so | uams_cleartxt.so | uams_randnum.so  | uams_dhx.so  | uams_dhx2.so  | uams_gss.so      |
| Password storage | None          | Either system auth or PAM | Passwords stored in clear text in a separate text file | Either system auth or PAM | Either system auth or PAM | At the Kerberos Key Distribution Center |

Note that a number of open-source and other third-party AFP clients exist.
Refer to their documentation for a list of supported UAMs.

### Save and change passwords

Netatalk can be configured to allow clients to save or change their passwords on the server.
The **save password** option in the **Global** section of *afp.conf* enables this feature,
but the AFP client must also support and honor this flag.

To allow clients to change their passwords, set the **set password** option.
This depends on the UAM in use and is not supported by all of them.
Notably, the PAM-based UAMs support this feature, while those based on classic UNIX passwords do not.

## ACL Support

ACL support for AFP is implemented for ZFS ACLs on Solaris and derived
platforms, and for POSIX 1e ACLs on Linux.

### Configuration

For basic operation, no additional configuration is needed. Netatalk
reads ACLs on the fly and calculates effective permissions, which are
then sent to the AFP client via the so-called
UARights permission bits. On a Mac, the
Finder uses these bits to adjust permissions in its windows. For example,
a folder whose UNIX mode is read-only but whose ACL grants the user write
access will display the effective read-write permission. Without this
permission mapping, the Finder would display a read-only icon and the
user would not be able to write to the folder.

By default, the authenticated user's effective permissions are only
mapped to the UARights permission structure, not the UNIX mode.
You can adjust this behavior with the configuration option
[map acls](afp.conf#options-for-acl-handling).

However, neither in Finder "Get Info" windows nor in the Terminal will
you be able to see the ACLs, because of how macOS ACLs are designed.
To display ACLs on the client, you must set up both client and server as part of an
authentication domain (directory service, e.g. LDAP, OpenDirectory). The
reason is that macOS ACLs are bound to UUIDs, not just UIDs or
GIDs. Therefore **afpd** must be able to map every filesystem UID and GID
to a UUID so that it can return server-side ACLs — which are bound to
UNIX UID and GID — mapped to macOS UUIDs.

Netatalk can query a directory server using LDAP. Either the
directory server already provides a UUID attribute for users and groups
(Active Directory, Open Directory), or you can repurpose an unused attribute (or
add a new one) in your directory server (e.g. OpenLDAP).

In detail:

1. For Solaris/ZFS: ZFS Volumes

    You should configure the following ZFS ACL properties for any volume you want to use
    with Netatalk:

        aclinherit = passthrough
        aclmode = passthrough

    For an explanation of what these properties do and how to apply them, check
    your host's ZFS documentation (e.g. `man zfs`).

2. Authentication Domain

    Your server and clients must be part of a security association
    where identity data comes from a common source. ACLs in Darwin
    are based on UUIDs, as is the ACL specification in AFP 3.2.
    Your source of identity data must provide an attribute
    for every user and group containing a UUID stored as an ASCII string.
    In other words:

    - You need an Open Directory server or an LDAP server where you
      store UUIDs in an attribute.

    - Your clients must be configured to use this server.

    - Your server should be configured to use this server via nsswitch
      and PAM.

    - Configure Netatalk via the [LDAP options for
      ACLs](afp.conf.html#options-for-acl-handling) in *afp.conf* so that Netatalk can
      retrieve UUIDs for users and groups via LDAP search
      queries.

### macOS ACLs

With Access Control Lists (ACLs), macOS offers a powerful extension of
the traditional UNIX permissions model. An ACL is an ordered list of
Access Control Entries (ACEs) that explicitly grant or deny a set of
permissions to a given user or group.

Unlike UNIX permissions, which are bound to user or group IDs, ACLs are
tied to UUIDs. Accessing an object's ACL therefore requires that server
and client use a common directory service to translate between
UUIDs and user/group IDs.

ACLs and UNIX permissions interact straightforwardly. Because ACLs are
optional, UNIX permissions act as a default mechanism for access control.
Changing an object's UNIX permissions leaves its ACL intact, and
modifying an ACL never changes the object's UNIX permissions. During
access checks, macOS first examines an object's ACL, evaluating
ACEs in order until all requested rights have been granted, a requested
right has been explicitly denied by an ACE, or the end of the list has
been reached. If there is no ACL or the permissions granted by the
ACL are insufficient, macOS next evaluates the
object's UNIX permissions. ACLs therefore always take precedence over
UNIX permissions.

### ZFS ACLs

ZFS ACLs closely match macOS ACLs. Both offer mostly identical fine-grained
permissions and inheritance settings.

### POSIX ACLs

#### Overview

Compared to macOS or NFSv4 ACLs, POSIX ACLs represent a different, less
versatile approach to overcoming the limitations of traditional UNIX
permissions. Implementations are based on the withdrawn POSIX 1003.1e
standard.

The standard defines two types of ACLs. Files and directories can have
access ACLs, which are consulted for access checks. Directories can also
have default ACLs, which are not used in access checks. When a new object is
created inside a directory with a default ACL, the default ACL is
applied to the new object as its access ACL. Subdirectories inherit
default ACLs from their parent. There are no further mechanisms of
inheritance control.

Architectural differences between POSIX ACLs and macOS ACLs include:

- No fine-grained permissions model. Like UNIX permissions, POSIX ACLs
only differentiate between read, write, and execute permissions.

- Entries within an ACL are unordered.

- POSIX ACLs can only grant rights. There is no way to explicitly deny
rights.

- UNIX permissions are integrated into an ACL as special entries.

POSIX 1003.1e defines six different types of ACL entries. The first three
types integrate standard UNIX permissions. They form a
minimal ACL; their presence is mandatory and only one entry of each type
is allowed per ACL.

- ACL_USER_OBJ: the owner's access rights.

- ACL_GROUP_OBJ: the owning group's access rights.

- ACL_OTHER: everybody's access rights.

The remaining entry types extend the traditional permissions model:

- ACL_USER: grants access rights to a specific user.

- ACL_GROUP: grants access rights to a specific group.

- ACL_MASK: limits the maximum access rights that can be granted by
entries of type ACL_GROUP_OBJ, ACL_USER, and ACL_GROUP. As the name
suggests, this entry acts as a mask. Only one ACL_MASK entry is
allowed per ACL. If an ACL contains ACL_USER or ACL_GROUP entries, an
ACL_MASK entry must be present; otherwise it is optional.

In order to maintain compatibility with applications not aware of ACLs,
POSIX 1003.1e changes the semantics of system calls and utilities that
retrieve or manipulate an object's UNIX permissions. If an object
only has a minimal ACL, the group permission bits of the UNIX
permissions correspond to the value of the ACL_GROUP_OBJ entry.

However, if the ACL also contains an ACL_MASK entry, the behavior
differs. The group permission
bits of the UNIX permissions correspond to the value of the ACL_MASK
entry — i.e. calling `chmod g-w` will not only revoke write access for
the group, but for all entities granted write access by
ACL_USER or ACL_GROUP entries.

#### Mapping POSIX ACLs to macOS ACLs

When a client reads an object's ACL, **afpd** maps its POSIX ACL
onto an equivalent macOS ACL. Writing an object's ACL requires **afpd** to
map a macOS ACL onto a POSIX ACL. Due to architectural limitations of
POSIX ACLs, an exact mapping is usually impossible, so
the result is an approximation of the
original ACL's semantics.

- **afpd** silently discards entries that deny permissions, because
they cannot be represented in the POSIX architecture.

- Because entries within POSIX ACLs are unordered, it is impossible to
preserve order.

- Inheritance control is subject to severe limitations:

  - Entries with the only_inherit flag set become part of the
    directory's default ACL only.

  - Entries with at least one of the flags file_inherit,
    directory_inherit, or limit_inherit set become part of the
    directory's access and default ACL, but the restrictions they impose
    on inheritance are ignored.

- The lack of a fine-grained permission model on the POSIX side
normally results in an increase of granted permissions.

Because macOS clients are unaware of the POSIX 1003.1e-specific relationship
between UNIX permissions and ACL_MASK, **afpd** does not expose this feature
to the client. This avoids compatibility issues and handles UNIX
permissions and ACLs the same way as Apple's reference implementation of
AFP. When an object's UNIX permissions are requested, **afpd**
calculates proper group rights and returns the result together with the
owner's and everybody's access rights to the caller via the "permissions"
and "ua_permissions" members of the FPUnixPrivs structure (see Apple
Filing Protocol Reference, page 181). When changing an object's permissions,
**afpd** always updates ACL_USER_OBJ, ACL_GROUP_OBJ, and ACL_OTHERS. If an
ACL_MASK entry is present, **afpd** recalculates its value so that the
new group rights become effective while existing ACL_USER
and ACL_GROUP entries remain intact.

## Directory Cache Tuning

The dircache (directory cache) in Netatalk is a sophisticated
multi-layer cache for files and directories, including filesystem stat
information, AppleDouble metadata, and resource forks.

Once populated during an initial directory enumeration, subsequent
operations return cached data directly from memory. This enables fast
operations even with very large working sets.
When any AFP user renames or deletes files and directories, dircache
entries are updated or removed for all users. While the dircache is
always kept in sync with AFP operations, it does not detect changes
made outside of Netatalk (e.g. local filesystem changes, or
modifications by other file sharing services such as Samba).

By default (with `dircache validation freq = 1`), Netatalk performs a
`stat()` operation on every access to validate that the cache entry
still matches the storage layer. While the cache still avoids most
I/O operations, this ctime check adds some overhead. When Netatalk is
the only process accessing a storage volume, you can set a high
`dircache validation freq` value (e.g. 100) to skip most of these
extra `stat()` calls, reducing load on the storage and page-cache
layers.

If a dircache entry is found to be stale when accessed, Netatalk
gracefully detects the issue, rebuilds the entry with the latest
state from storage, and increments the `invalid_on_use` counter.
You can use the `invalid_on_use` counter to tune
`dircache validation freq` in environments where external changes are
infrequent but maximum performance is still desired.

dircache size = *number* **(G)**

Maximum number of entries in the cache.
The given value is rounded up to the nearest power of 2. Each entry takes about
192 bytes (on 64-bit systems), and each **afpd** child process
for every connected user has its own cache. Consider the number of users when configuring `dircache size`.

Default: 65536, Maximum: 2097152.

dircache validation freq = *number* **(G)**

Directory cache validation frequency for external change detection.
Controls how often cached entries are validated against the filesystem
to detect changes made outside of Netatalk (e.g. direct filesystem
modifications by other processes).
A value of 1 means validate on every access (the default, for backward
compatibility); higher values validate less
frequently. For example, 5 means validate cache entries every 5th access.
Higher values improve performance and reduce storage I/O
and page-cache stress, but may delay detection of external changes.

Default: 1, Range: 1–100.

Internal Netatalk operations (file/directory create, delete, rename)
always update dircache entries immediately regardless of this setting.
If Netatalk is the only process accessing the volume, you can safely
set a value of 100 for maximum performance.

dircache mode = *lru* | *arc* (default: *lru*) **(G)**

Cache replacement algorithm. Netatalk supports two cache eviction policies:

- **LRU (Least Recently Used)** — the legacy algorithm and the default. Maintains a single list
  ordered by access time (FIFO); the least recently accessed entry is evicted when
  the cache is full. Simple and memory-efficient.

- **ARC (Adaptive Replacement Cache)** — a modern self-tuning algorithm that
  dynamically balances *recency* and *frequency* to achieve better cache hit rates. Based on
  ["Outperforming LRU with an Adaptive Replacement Cache Algorithm"](https://theory.stanford.edu/~megiddo/pdf/IEEE_COMPUTER_0404.pdf)
  (Megiddo & Modha, IBM, 2004).

ARC adapts to the observed access pattern, making it effective for workloads
that mix temporal locality with frequency-based reuse — for example, when
users alternate between recently and frequently accessed files or directories.
It maintains four lists: two for cached entries (T1 recent, T2 frequent) and
two *ghost* lists (B1, B2) that track recently evicted entries. When a ghost
entry is re-accessed, ARC learns from the miss and adjusts its policy. This ghost
learning and segmentation is what distinguishes ARC from LRU, and also makes ARC resistant to
sequential scans and backups that can flush an LRU cache.

**Netatalk's ARC uses complete ghost entries** (retaining all content) rather
than the metadata-only ghosts in the IBM paper, because cache entries are small. A ghost hit is promoted back
to the cache instantly without any filesystem lookup, so ARC should always
perform at least as well as LRU in edge cases and better in all others.

**Memory:** ARC uses approximately **twice the memory** of LRU because it
tracks up to *c* ghost entries alongside *c* cached entries (where *c* is
`dircache size`). Each entry is ~192 bytes on 64-bit systems. Even so, ARC still outperforms an LRU
cache of the same total memory.

| Mode | Entries tracked | Memory (1000-entry cache) |
|------|-----------------|--------------------------|
| LRU  | 1000 cached     | ~192 KB                  |
| ARC  | 1000 cached + up to 1000 ghosts | ~384 KB    |

For the default `dircache size` of 65536: LRU ≈ 12 MB, ARC ≈ 24 MB per
connected user.

**Recommendation**: Use **arc** for servers with 4 GB or more RAM.
Use **lru** for memory-constrained systems.

Default: lru.

### Resource Fork Caching

Netatalk can optionally cache resource fork data
(AppleDouble `._` data in classic Mac OS, or extended attributes in
modern macOS/Linux/UNIX systems) in a tier-2 dircache layer. This
avoids repeated storage I/O when AFP clients enumerate directories
with FPGetFileDirParams or FPEnumerate and request resource fork
data. Resource forks store many macOS data structures such as folder
cover art (icns data).
Users who set custom directory icons, for example, will observe
significant performance gains from the resource fork cache layer.

Resource fork caching is disabled by default and is controlled by two
settings:

dircache rfork budget = *number* (default: *0*) **(G)**

Total memory budget in KB for caching resource fork data per connected
user. When set to 0 (the default), resource fork caching is disabled.

Maximum: 10485760 (10 GB in KB).

dircache rfork maxsize = *number* (default: *1024*) **(G)**

Maximum size in KB of a single resource fork entry that will be cached.
Resource forks larger than this value are not cached even if the total
budget has space remaining.

Maximum: 10240 (10 MB in KB).

Cached entries are automatically invalidated when the file's ctime or
inode changes, or when an AFP client modifies the resource fork.
Entries are evicted in LRU/ARC order when the budget is exceeded.

**Example** (100 MB rfork budget, max 5 MB per entry):

    dircache rfork budget = 102400
    dircache rfork maxsize = 5120

**Example** (Netatalk-only access to volume):

    dircache validation freq = 100

**Example** (file-heavy workload with large cache):

    dircache size = 262144
    dircache validation freq = 100
    dircache mode = arc

**Note**: Monitor dircache effectiveness by checking Netatalk log files
for "dircache statistics:" lines when **afpd** shuts down gracefully (on user
disconnect).

## Interoperability with Other Network File Systems

Netatalk and Samba can share the same volume and store Extended Attribute
metadata in a mutually compatible format, enabling access
via AFP or SMB.

> ***WARNING:*** The **ea = samba** and **ea = sys** (Netatalk default) metadata
> formats are **not** compatible, and there is currently no automated way to
> convert between them. If you have an existing Netatalk volume using
> the default settings and want to add Samba sharing, the recommended
> migration path is to copy files from one volume to another using an AFP
> client.

When sharing a volume with other processes (Samba, NFS, local applications),
keep `dircache validation freq` at **1** (the default) so Netatalk
detects external changes on every access. If Netatalk is the **only**
process on the volume, you can set `dircache validation freq = 100` for
maximum performance.

### Netatalk configuration

Use `ea = samba` to store Extended Attributes in the Samba-compatible format.

    [Global]
        vol preset = my default values
        dircache validation freq = 1
        ea = samba

    [Homes]
        basedir regex = /home

    [Test Volume]
        path = /export/test1

    [My Time Machine Volume]
        path = /export/timemachine
        time machine = yes

### Samba configuration

The Samba **catia**, **fruit**, and **streams_xattr** VFS modules provide
compatibility for Apple SMB clients and interoperability with
Netatalk. Use **hide files** (not **veto files**) to hide Netatalk's
invisible files from Windows clients.

    [global]
        ea support = yes
        vfs objects = catia fruit streams_xattr

        fruit:encoding = native
        streams_xattr:prefix = user.
        streams_xattr:store_stream_type = no

        hide files = /.DS_Store/Network Trash Folder/TheFindByContentFolder/TheVolumeSettingsFolder/Temporary Items/.TemporaryItems/.VolumeIcon.icns/Icon?/.FBCIndex/.FBCLockFolder/

        read only = no

    [homes]

    [Test Volume]
        path = /export/test1

    [My Time Machine Volume]
        path = /export/timemachine

## Filesystem Change Events

Netatalk includes a filesystem change event (FCE) mechanism that allows
**afpd** processes to notify interested listeners about certain filesystem
events via UDP network datagrams.

This feature is disabled by default. To enable it, set the **fce listener**
option in *afp.conf* to the hostname or IP address of the host that should
listen for FCE events.

Netatalk distributes a simple FCE listener application called **fce_listen**.
It prints any UDP datagrams received from the AFP server.
Its source code also serves as documentation for the format of the UDP packets.

### FCE v1

The supported FCE v1 events are:

- file modification (fmod)

- file deletion (fdel)

- directory deletion (ddel)

- file creation (fcre)

- directory creation (dcre)

### FCE v2

FCE v2 events provide additional context such as the user who performed the
action. FCE v2 also adds the following events:

- file moving (fmov)

- directory moving (dmov)

- user login (login)

- user logout (logout)

## Spotlight Compatible Search

You can enable Netatalk's Spotlight-compatible search and indexing
either globally or on a per-volume basis with the **spotlight** option.

> ***WARNING:*** Once Spotlight is enabled for a single volume,
all other volumes for which Spotlight is disabled will not be searchable at all.

The **dbus-daemon** binary must be installed for the Spotlight feature. The
path to **dbus-daemon** is determined at compile time by the dbus-daemon build
system option.

If **dbus-daemon** is installed at a different path, use
the global option **dbus daemon** to specify it. For example, for
Solaris with Tracker from OpenCSW:

    dbus daemon = /opt/csw/bin/dbus-daemon
