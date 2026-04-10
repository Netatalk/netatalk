# Authentication

## AFP authentication basics

Apple chose a flexible model called "User Authentication Modules" (UAMs)
for authentication between AFP clients and servers.
An AFP client connecting to an AFP server first requests the list of UAMs that the server supports,
then chooses the one with the strongest encryption that the client also supports.

Several UAMs have been developed by Apple over the years,
and some by third-party developers.

## UAMs supported by Netatalk

Netatalk supports the following UAMs by default:

- "No User Authent" UAM (guest access without authentication)

- "Cleartxt Passwrd" UAM (no password encryption)

- "Randnum exchange"/"2-Way Randnum exchange" UAMs
(weak password encryption, separate password storage)

- "DHCAST128" UAM (a.k.a. DHX; stronger password encryption)

- "DHX2" UAM (successor of DHCAST128 with even stronger password encryption)

- "SRP" UAM ("Secure Remote Password", strongest password encryption, separate verifier storage)

With Kerberos support enabled at compile time, Netatalk also supports:

- "Client Krb v2" UAM
(Kerberos V, suitable for "Single Sign On" scenarios with macOS clients — see below)

You can configure which UAMs to activate by setting **uam list** in the **Global** section.
**afpd** logs which UAMs it activates and reports any problems in either **netatalk.log**
or syslog at startup.
**asip-status** can also query the available UAMs of AFP servers.

Having a specific UAM available on the server does not automatically mean that a client can use it.
Client-side support is also required.
For older Macintosh clients running Classic Mac OS,
DHCAST128 support has been available since AppleShare client 3.8.x.

On macOS, there are client-side techniques to make the AFP client more verbose,
allowing you to observe the UAM negotiation process.
See this [hint](https://web.archive.org/web/20080312054723/http://article.gmane.org/gmane.network.netatalk.devel/7383/).

## Which UAMs to activate?

The choice depends primarily on your needs and the macOS clients you need to support.
If your network consists exclusively of macOS clients,
DHX2 is sufficient and provides the strongest encryption.

- Unless you specifically need guest access,
  do not enable "No User Authent" to prevent accidental unauthorized access.
  If you must enable guest access,
  enforce it on a per-volume basis using access controls.

    Note: "No User Authent" is required to use Apple II network boot services
    (**a2boot**) to boot an Apple //e over AFP.

- The "ClearTxt Passwrd" UAM is as bad as it sounds:
  passwords are sent unencrypted over the wire.
  Avoid it on both the server and client side.

    Note: If you want to provide Mac OS 8/9 clients with network boot services,
    you need uams_clrtxt.so since the AFP client integrated into
    the Mac's firmware can only handle this basic form of authentication.

- "Randnum exchange"/"2-Way Randnum exchange" uses only 56-bit DES for encryption,
  so it should also be avoided.
  An additional disadvantage is that passwords must be stored as raw hex on the server.

    However, this is the strongest form of authentication available for
    Macintosh System Software 7.1 or earlier.

- "DHCAST128" ("DHX") or "DHX2" is the best choice for most users,
  combining stronger encryption with PAM integration.

- "SRP" is the strongest UAM, but it requires a separate file to store per-user salts and verifiers.
  If you don't mind the extra maintenance overhead of this file, SRP is the best choice for security-conscious users.

- The Kerberos V ("Client Krb v2")
  UAM enables true single sign-on scenarios using Kerberos tickets.
  The password is not sent over the network.
  Instead, the user's password decrypts a service ticket for the AFP server.
  The service ticket contains an encryption key for the client and some encrypted data
  (which only the AFP server can decrypt).
  The encrypted portion of the service ticket is sent to the server and used to authenticate the user.
  Because of how **afpd** implements service principal detection,
  this authentication method is vulnerable to man-in-the-middle attacks.

For a more detailed overview of the technical implications of the different UAMs,
see Apple's [File Server Security](http://developer.apple.com/library/mac/#documentation/Networking/Conceptual/AFP/AFPSecurity/AFPSecurity.html#//apple_ref/doc/uid/TP40000854-CH232-SW1)
documentation.

## Password storage

Randnum and SRP do not use system passwords directly. They both rely on
separate files managed with **afppasswd**:

- **Randnum** uses the legacy *afppasswd* file. By default it stores the raw
  password as hex. If an optional key file exists alongside the Randnum
  password file (same path with a `.key` suffix), the stored password is
  DES-encrypted using that key instead. To use a key file, create
  `<passwd file>.key` containing 16 hex characters (8 bytes) and restrict
  permissions (for example, owner-readable only).

- **SRP** uses *afppasswd.srp*, which stores per-user salts and verifiers.

## Using different authentication backends

Some UAMs support different authentication backends,
namely **uams_clrtxt.so**, **uams_dhx.so**, and **uams_dhx2.so**.
They can use either classic UNIX passwords from */etc/passwd* (*/etc/shadow*)
or PAM if the system supports it.
**uams_clrtxt.so** can be symlinked to either **uams_passwd.so** or **uams_pam.so**,
**uams_dhx.so** to **uams_dhx_passwd.so** or **uams_dhx_pam.so**,
and **uams_dhx2.so** to **uams_dhx2_passwd.so** or **uams_dhx2_pam.so**.

If Netatalk's UAM folder (by default */etc/netatalk/uams/*) looks like this,
then you are using PAM;
otherwise you are using classic UNIX passwords:

    uams_clrtxt.so -> uams_pam.so
    uams_dhx.so -> uams_dhx_pam.so
    uams_dhx2.so -> uams_dhx2_pam.so

The main advantage of PAM is that it integrates Netatalk into centralized authentication scenarios,
such as LDAP, NIS, and similar systems.
Keep in mind that the security of your users' login credentials in such scenarios
also depends on the encryption strength of the UAM in use.

## Netatalk UAM overview table

An overview of the officially supported UAMs on Macs.

| UAM              | No User Auth  | Cleartxt Passwrd | RandNum Exchange | DHCAST128    | DHX2          | Kerberos V       | SRP              |
| ---------------- | ------------- | ---------------- | ---------------- | ------------ | ------------- | ---------------- | ---------------- |
| Password length  | guest access  | max 8 chars      | max 8 chars      | max 64 chars | max 255 chars | Kerberos tickets | max 255 chars    |
| Client support   | built-in into all Mac OS versions | built-in in all Mac OS versions except 10.0. Has to be activated explicitly in later Mac OS X versions | built-in into almost all Mac OS versions | built-in since AppleShare client 3.8.4, available as a plug-in for 3.8.3, integrated in macOS's AFP client | built-in since Mac OS X 10.2 | built-in since Mac OS X 10.2 | built-in since Mac OS X 10.7 |
| Encryption       | Enables guest access without authentication between client and server. | Password will be sent in cleartext over the wire. Just as bad as it sounds, therefore avoid at all costs. | 8-byte random numbers are sent over the wire, comparable with DES, 56 bits. Vulnerable to offline dictionary attack. Requires passwords in clear on the server. | Password will be encrypted with 128 bit CAST, user will be authenticated against the server but not vice versa. Therefore weak against man-in-the-middle attacks. | Password will be encrypted with 128 bit CAST in CBC mode. User will be authenticated against the server but not vice versa. Therefore weak against man-in-the-middle attacks. | Password is not sent over the network. Due to the service principal detection method, this authentication method is vulnerable to man-in-the-middle attacks. | Password is never sent; SRP uses a verifier and mutual proofs (M1/M2) to authenticate both client and server, providing protection against man‑in‑the‑middle attacks. |
| Server support   | uams_guest.so | uams_clrtxt.so   | uams_randnum.so  | uams_dhx.so  | uams_dhx2.so  | uams_gss.so      | uams_srp.so      |
| Password storage | None          | Either system auth or PAM | Separate *afppasswd* file; raw hex or DES-encrypted with *.key* | Either system auth or PAM | Either system auth or PAM | At the Kerberos Key Distribution Center | In a separate *afppasswd.srp* verifier file |

Note that a number of open-source and other third-party AFP clients exist.
Refer to their documentation for a list of supported UAMs.

## Save and change passwords

Netatalk can be configured to allow clients to save or change their passwords on the server.
The **save password** option in the **Global** section of *afp.conf* enables this feature,
but the AFP client must also support and honor this flag.

To allow clients to change their passwords, set the **set password** option.
This depends on the UAM in use and is not supported by all of them.
Notably, the PAM-based UAMs support this feature, while those based on classic UNIX passwords do not.
