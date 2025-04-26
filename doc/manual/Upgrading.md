# Upgrading from prior Netatalk versions

## Upgrading from Netatalk 3

Upgrading to Netatalk 4 from Netatalk 3 is trivial. Just install the new
version on top of the old one. The primary difference is that Netatalk 4
brings back essential AppleTalk services, configuration files, and tools
that were removed between Netatalk 2 and Netatalk 3.

Notably, the **atalkd** daemon with its **atalkd.conf** configuration file,
and the **papd** daemon with its **papd.conf** configuration file are once
more available.

## Upgrading from Netatalk 2

There are three major changes between Netatalk 2 and Netatalk 4:

1.  New configuration files that replaces most of the previous ones:
    *afp.conf* and *extmap.conf*

2.  New backend for file metadata which stores metadata in extended attributes
    of the filesystem.

3.  The AppleTalk transport layer is disabled by default. If you want to
    use Netatalk with very old Macs, turn it on with the
    **appletalk = yes** option in *afp.conf*. Then start the **atalkd**
    daemon before **netatalk** in order to activate the AppleTalk
    transport layer.

### New configuration

#### afp.conf

- ini style syntax (akin to Samba's smb.conf)

- one to rule them all: configure AFP settings and volumes in one file

- obsoletes *afpd.conf*, *netatalk.conf*, *AppleVolumes.default* and
*afp_ldap.conf*

> **WARNING**

> most option names have changed, read the full manpage
[afp.conf](afp.conf.5.html) for details

#### extmap.conf

- maps file extensions to Classic Mac OS type/creator

- unlike 2.x, the mappings are disabled by default; uncomment the lines
in the file to enable them

- obsoletes *AppleVolumes.system*

### New metadata backend

New file metadata backend **ea = sys** which stores macOS extended attributes
and Classic Mac OS resource forks in extended attributes of the filesystem.

- default backend (!)

- requires a filesystem with Extended Attributes, fallback is AppleDouble v2
which is enabled with **ea = ad**

- converts filesystems from AppleDouble v2 to Extended Attributes on
the fly when accessed by clients (can be disabled with **convert appledouble**)

- **dbd** can be used to do conversion in one shot

Implementation details:

- stores Mac Metadata (e.g. FinderInfo, AFP Flags, Comment, CNID) in an
Extended Attributed named *org.netatalk.Metadata*

    - Additionally, on macOS hosts running Netatalk 4.1.0 or later,
    FinderInfo is natively stored in the file system and appears as an
    Extended Attribute named *com.apple.FinderInfo*

- stores Mac ResourceFork either in

    - an Extended Attribute named *org.netatalk.ResourceFork* on
    Solaris w. ZFS, or in

    - an extra AppleDouble file named *._file* for a file named *file*,
    or

    - natively stored in the resource fork on macOS hosts as of Netatalk
    4.1.0.

- the format of the .\_ file is exactly as the Mac’s CIFS client expects
it when accessing the same filesystem via a CIFS server (Samba), thus
you can have parallel access from Macs to the same dataset via AFP and
CIFS without the risk of losing data (resources or metadata).
Accessing the same dataset with CIFS from Windows clients will still
break the coupling of *file* and *._file* on non ZFS filesystems
(see above), so for this we still need an enhanced Samba VFS module.

### Other major changes

- New service controller daemon [netatalk](netatalk.html) which is
responsible for starting and restarting the AFP and CNID daemons. All
bundled start scripts have been updated, make sure to update yours!

- All CNID databases are now stored under *$prefix/var/netatalk/CNID/*
by default, rather than in the individual shared volume directories

- Netatalk 2.x volume options **usedots** and **upriv** now enabled by
default

- Removed SLP and AFP proxy support

### Upgrading procedure

1.  Stop Netatalk 2.x

2.  Install Netatalk 4

3.  Manually recreate configurations in *afp.conf* and *extmap.conf*

4.  Update your Netatalk init script to start **netatalk** instead of
    **afpd** and **cnid_metad**, or replace it with the appropriate stock
    init script for your system.

5.  Move *afp_voluuid.conf* and *afp_signature.conf* to the localstate
    directory (default *$prefix/var/netatalk/*), you can use **afpd -v**
    in order to find the correct path

6.  Start Netatalk 4

### Old and new configuration file names

| Old File Name        | New File Name      | Remarks                       |
|----------------------|--------------------|-------------------------------|
| -                    | afp.conf           | new ini-style format          |
| afp_signature.conf   | afp_signature.conf | moved to $localstatedir       |
| afp_voluuid.conf     | afp_voluuid.conf   | moved to $localstatedir       |
| netatalk.conf        | -                  | obsolete                      |
| afpd.conf            | -                  | merged into afp.conf          |
| afp_ldap.conf        | -                  | merged into afp.conf          |
| AppleVolumes.default | -                  | merged into afp.conf          |
| AppleVolumes.system  | extmap.conf        | introduced in netatalk 3.0.2  |
| ~/.AppleVolumes      | -                  | obsolete                      |

### Old and new option names

Mappings from netatalk.conf (Debian: */etc/default/netatalk*) to afp.conf or
to the means of invocation

| Old netatalk.conf  | New afp.conf    | Old Default Value | New Default Value | Section | Remarks                       |
|--------------------|-----------------|-------------------|-------------------|---------|-------------------------------|
| ATALK_NAME         | hostname        | -                 | -                 | (G)     | use gethostname() by default  |
| ATALK_UNIX_CHARSET | unix charset    | LOCALE            | UTF8              | (G)     | -                             |
| ATALK_MAC_CHARSET  | mac charset     | MAC_ROMAN         | MAC_ROMAN         | (G)/(V) | -                             |
| CNID_METAD_RUN     | -               | yes               | -                 | -       | controlled by netatalk(8)     |
| AFPD_RUN           | -               | yes               | -                 | -       | controlled by netatalk(8)     |
| AFPD_MAX_CLIENTS   | max connections | 20                | 200               | (G)     | -                             |
| AFPD_UAMLIST       | uam list        | -U uams_dhx.so,uams_dhx2.so | uams_dhx.so uams_dhx2.so | (G) | -                |
| AFPD_GUEST         | guest account   | nobody            | nobody            | (G)     | -                             |
| CNID_CONFIG        | log level       | -l log_note       | cnid:note         | (G)     | -                             |
| CNID_CONFIG        | log file        | -                 | -                 | (G)     | -                             |
| ATALKD_RUN         | -               | no                | -                 | -       | controlled by the init system |
| PAPD_RUN           | -               | no                | -                 | -       | controlled by the init system |
| TIMELORD_RUN       | -               | no                | -                 | -       | controlled by the init system |
| A2BOOT_RUN         | -               | no                | -                 | -       | controlled by the init system |
| ATALK_BGROUND      | -               | no                | -                 | -       | obsolete                      |
| ATALK_ZONE         | ddp zone        | -                 | -                 | (G)     | introduced in 4.0.0           |

Mappings from afpd.conf to afp.conf

| Old afpd.conf      | New afp.conf    | Old Default Value    | New Default Value  | Section | Remarks |
|--------------------|-----------------|----------------------|--------------------|---------|---------|
| - or "server name" | server name     | -                    | -                  | (G)     | new in 4.2.0; default is *hostname* |
| -uamlist           | uam list        | uams_dhx.so,uams_dhx2.so | uams_dhx.so uams_dhx2.so | (G) | - |
| -nozeroconf        | zeroconf        | -                    | yes (if supported) | (G)     | -       |
| -advertise_ssh     | advertise ssh   | -                    | no                 | (G)     | -       |
| -[no]savepassword  | save password   | -savepassword        | yes                | (G)     | -       |
| -[no]setpassword   | set password    | -nosetpassword       | no                 | (G)     | -       |
| -client_polling    | client polling  | -                    | no                 | (G)     | -       |
| -hostname          | hostname        | -                    | -                  | (G)     | use gethostname() by default |
| -loginmesg         | login message   | -                    | -                  | (G)/(V) | -       |
| -guestname         | guest account   | nobody               | nobody             | (G)     | -       |
| -passwdfile        | passwd file     | afppasswd            | afppasswd          | (G)     | -       |
| -passwdminlen      | passwd minlen   | -                    | -                  | (G)     | -       |
| -tickleval         | tickleval       | 30                   | 30                 | (G)     | -       |
| -timeout           | timeout         | 4                    | 4                  | (G)     | -       |
| -sleep             | sleep time      | 10                   | 10                 | (G)     | -       |
| -dsireadbuf        | dsireadbuf      | 12                   | 12                 | (G)     | -       |
| -server_quantum    | server quantum  | 303840               | 1048576            | (G)     | -       |
| -volnamelen        | volnamelen      | 80                   | 80                 | (G)     | -       |
| -setuplog          | log level       | default log_note     | default:note       | (G)     | -       |
| -setuplog          | log file        | -                    | -                  | (G)     | -       |
| -admingroup        | admingroup      | -                    | -                  | (G)     | -       |
| -k5service         | k5 service      | -                    | -                  | (G)     | -       |
| -k5realm           | k5 realm        | -                    | -                  | (G)     | -       |
| -k5keytab          | k5 keytab       | -                    | -                  | (G)     | -       |
| -uampath           | uam path        | etc/netatalk/uams    | lib/netatalk       | (G)     | moved to $libdir |
| -ipaddr            | afp listen      | -                    | -                  | (G)     | -       |
| -cnidserver        | cnid server     | localhost:4700       | localhost:4700     | (G)/(V) | -       |
| -port              | port            | 548                  | 548                | (G)     | -       |
| -signature         | signature       | auto                 | -                  | (G)     | -       |
| -fqdn              | fqdn            | -                    | -                  | (G)     | -       |
| -unixcodepage      | unix charset    | LOCALE               | UTF8               | (G)     | -       |
| -maccodepage       | mac charset     | MAC_ROMAN            | MAC_ROMAN          | (G)/(V) | -       |
| -closevol          | close vol       | -                    | no                 | (G)     | -       |
| -ntdomain          | nt domain       | -                    | -                  | (G)     | -       |
| -ntseparator       | nt separator    | -                    | -                  | (G)     | -       |
| -dircachesize      | dircachesize    | 8192                 | 8192               | (G)     | -       |
| -tcpsndbuf         | tcpsndbuf       | -                    | -                  | (G)     | OS default |
| -tcprcvbuf         | tcprcvbuf       | -                    | -                  | (G)     | OS default |
| -fcelistener       | fce listener    | -                    | -                  | (G)     | -       |
| -fcecoalesce       | fce coalesce    | -                    | -                  | (G)     | -       |
| -fceevents         | fce events      | -                    | -                  | (G)     | -       |
| -fceholdfmod       | fce holdfmod    | 60                   | 60                 | (G)     | -       |
| -mimicmodel        | mimic model     | -                    | -                  | (G)     | -       |
| -adminauthuser     | admin auth user | -                    | -                  | (G)     | -       |
| -noacl2maccess     | map acls        | -                    | rights             | (G)     | -       |
| -[no]tcp           | -               | -tcp                 | -                  | -       | TCP transport layer is always active |
| -[no]ddp           | appletalk       | -ddp                 | no                 | (G)     | introduced in 4.0.0 |
| -[no]transall      | -               | -transall            | -                  | -       | TCP transport layer is always active |
| -nodebug           | -               | -                    | -                  | -       | obsolete |
| -[no]slp           | -               | -noslp               | -                  | -       | SLP support is obsoleted |
| -[no]uservolfirst  | -               | -nouservolfirst      | -                  | -       | uservol is obsoleted |
| -[no]uservol       | -               | -uservol             | -                  | -       | uservol is obsoleted |
| -proxy             | -               | -                    | -                  | -       | obsolete |
| -defaultvol        | -               | AppleVolumes.default | -                  | -       | afp.conf only |
| -systemvol         | -               | AppleVolumes.system  | -                  | -       | extmap.conf only |
| -loginmaxfail      | -               | -                    | -                  | -       | obsolete |
| -unsetuplog        | -               | -                    | -                  | -       | obsolete |
| -authprintdir      | -               | -                    | -                  | -       | CAP style auth is obsoleted |
| -ddpaddr           | ddp address     | 0.0                  | 0.0                | (G)     | introduced in 4.0.0 |
| -[no]icon          | legacy icon     | -noicon              | -                  | (G)     | introduced in 4.0.2 |
| -keepsessions      | -               | -                    | -                  | -       | obsolete; Use kill -HUP |

Mappings from afp_ldap.conf to afp.conf

| Old afp_ldap.conf | New afp.conf     | Old Default Value | New Default Value | Section | Remarks |
|-------------------|------------------|-------------------|-------------------|---------|---------|
| ldap_server       | ldap server      | -                 | -                 | (G)     | -       |
| ldap_auth_method  | ldap auth method | -                 | -                 | (G)     | -       |
| ldap_auth_dn      | ldap auth dn     | -                 | -                 | (G)     | -       |
| ldap_auth_pw      | ldap auth pw     | -                 | -                 | (G)     | -       |
| ldap_userbase     | ldap userbase    | -                 | -                 | (G)     | -       |
| ldap_userscope    | ldap userscope   | -                 | -                 | (G)     | -       |
| ldap_groupbase    | ldap groupbase   | -                 | -                 | (G)     | -       |
| ldap_groupscope   | ldap groupscope  | -                 | -                 | (G)     | -       |
| ldap_uuid_attr    | ldap uuid attr   | -                 | -                 | (G)     | -       |
| ldap_uuid_string  | ldap uuid string | -                 | -                 | (G)     | -       |
| ldap_name_attr    | ldap name attr   | -                 | -                 | (G)     | -       |
| ldap_group_attr   | ldap group attr  | -                 | -                 | (G)     | -       |

Mappings from AppleVolumes.\* to afp.conf

| Old AppleVolumes.\*         | New afp.conf       | Old Default Value     | New Default Value | Section | Remarks |
|----------------------------|--------------------|-----------------------|-------------------|---------|---------|
| (leading-dot lines)        | -                  | -                     | -                 | -       | moved to extmap.conf |
| :DEFAULT:                  | -                  | options:upriv,usedots | -                 | -       | use *vol preset* |
| 1st field ("~")            | -                  | -                     | -                 | -       | use [Homes] section |
| 1st field ("/path")        | path               | -                     | -                 | (V)     | -       |
| 2nd field                  | volume name        | -                     | *section name*      | (V)     | introduced in 4.2.0 |
| allow:                     | valid users        | -                     | -                 | (V)     | -       |
| deny:                      | invalid users      | -                     | -                 | (V)     | -       |
| rwlist:                    | rwlist             | -                     | -                 | (V)     | -       |
| rolist:                    | rolist             | -                     | -                 | (V)     | -       |
| volcharset:                | vol charset        | UTF8                  | *unix charset*      | (G)/(V) | -       |
| maccharset:                | mac charset        | MAC_ROMAN             | MAC_ROMAN         | (G)/(V) | -       |
| veto:                      | veto files         | -                     | -                 | (V)     | -       |
| cnidscheme:                | cnid scheme        | dbd                   | dbd               | (V)     | -       |
| casefold:                  | casefold           | -                     | -                 | (V)     | -       |
| adouble:                   | -                  | v2                    | -                 | -       | removed in 4.2.0 |
| cnidserver:                | cnid server        | localhost:4700        | localhost:4700    | (G)/(V) | -       |
| dbpath:                    | vol dbpath         | (volume directory)    | var/netatalk/CNID | (G)     | moved to $localstatedir |
| umask:                     | umask              | 0000                  | 0000              | (V)     | -       |
| dperm:                     | directory perm     | 0000                  | 0000              | (V)     | -       |
| fperm:                     | file perm          | 0000                  | 0000              | (V)     | -       |
| password:                  | password           | -                     | -                 | (V)     | -       |
| root_preexec:              | -                  | -                     | -                 | -       | removed in 4.1.0 |
| preexec:                   | preexec            | -                     | -                 | (V)     | -       |
| postexec:                  | postexec           | -                     | -                 | (V)     | -       |
| allowed_hosts:             | hosts allow        | -                     | -                 | (V)     | -       |
| denied_hosts:              | hosts deny         | -                     | -                 | (V)     | -       |
| ea:                        | ea                 | auto                  | *auto detection*    | (V)     | leave empty for auto detection |
| volsizelimit:              | vol size limit     | -                     | -                 | (V)     | -       |
| perm:                      | -                  | -                     | -                 | -       | Use *directory perm* and *file perm* |
| forceuid:                  | -                  | -                     | -                 | -       | obsolete |
| forcegid:                  | -                  | -                     | -                 | -       | obsolete |
| options:ro                 | read only          | -                     | no                | (V)     | -       |
| options:invisibledots      | invisible dots     | -                     | no                | (V)     | -       |
| options:nostat             | stat vol           | -                     | yes               | (V)     | -       |
| options:preexec_close      | preexec close      | -                     | no                | (V)     | -       |
| options:root_preexec_close | -                  | -                     | -                 | -       | removed in 4.1.0 |
| options:upriv              | unix priv          | -                     | yes               | (V)     | -       |
| options:nodev              | cnid dev           | -                     | yes               | (V)     | -       |
| options:illegalseq         | illegal seq        | -                     | no                | (V)     | -       |
| options:tm                 | time machine       | -                     | no                | (V)     | -       |
| options:searchdb           | search db          | -                     | no                | (V)     | -       |
| options:nonetids           | network ids        | -                     | yes               | (V)     | -       |
| options:noacls             | acls               | -                     | yes               | (V)     | -       |
| options:followsymlinks     | follow symlinks    | -                     | no                | (V)     | -       |
| options:nohex              | -                  | -                     | -                 | -       | auto-convert from ":2f" to ":" |
| options:usedots            | -                  | -                     | -                 | -       | auto-convert from ":2e" to "." |
| options:nofileid           | -                  | -                     | -                 | -       | obsolete |
| options:prodos             | prodos             | -                     | no                | (V)     | introduced in 4.0.0 |
| options:mswindows          | -                  | -                     | -                 | -       | obsolete |
| options:crlf               | -                  | -                     | -                 | -       | obsolete |
| options:noadouble          | -                  | -                     | -                 | -       | obsolete |
| options:limitsize          | legacy volume size | -                     | no                | (V)     | introduced in 4.0.0 |
| options:dropbox            | -                  | -                     | -                 | -       | obsolete |
| options:dropkludge         | -                  | -                     | -                 | -       | obsolete |
| options:nocnidcache        | -                  | -                     | -                 | -       | obsolete |
| options:caseinsensitive    | -                  | -                     | -                 | -       | obsolete |
