# Name

asip-status — Queries an AFP server for its capabilities

# Synopsis

`asip-status [ -4 | -6 ] [-d] [-i] [-x] [HOSTNAME] [PORT]`

`asip-status [ -4 | -6 ] [-d] [-i] [-x] [ HOSTNAME:PORT ]`

`asip-status [ -v | -version | --version ]`

# Description

`asip-status` sends an FPGetSrvrInfo request to an AFP server at
\<HOSTNAME\>:\<PORT\> and displays the results: "Machine type", the
server's name, supported AFP versions, UAMs and AFP flags, the "server
signature" and the network addresses that the server provides AFP
services on.

When you don't supply \<PORT\>, then the default AFP port, 548, will be
used.

Starting with Netatalk 3.1.9, `asip-status` supports both IPv4 and IPv6.
By default, your system automatically decides which protocol version to
use. You can use -4 or -6 option to force IPv4 or IPv6, respectively.

# Options

`-4`

> Use IPv4 only.

`-6`

> Use IPv6 only.

`-d`

> Enable debug output.

`-i`

> Show icon if it exists.

`-x`

> Enable hex dump output.

`-v, -version, --version`

> Show version.

# Examples

    $ asip-status 192.168.1.15
    AFP reply from 192.168.1.15:548 via IPv4
    Flags: 1  Cmd: 3  ID: 57005
    Reply: DSIGetStatus
    Request ID: 57005
    Machine type: Macintosh
    AFP versions: AFPVersion 1.1,AFPVersion 2.0,AFPVersion 2.1,AFP2.2
    UAMs: Cleartxt passwrd,Randnum exchange,2-Way Randnum exchange
    Volume Icon & Mask: Yes
    Flags:
        SupportsCopyFile
        SupportsChgPwd
        SupportsServerMessages
        SupportsServerSignature
        SupportsTCP/IP
        SupportsSuperClient
    Server name: bookchan
    Signature:
    04 1d 65 23 04 1d 65 23 04 1d 65 23 04 1d 65 23  ..e#..e#..e#..e#

    Network address: 192.168.1.15:548 (IPv4 address and port)
    Network address: 65280.128 (ddp address)

    $ asip-status myserver:10548
    AFP reply from myserver:10548 via IPv6
    Flags: 1  Cmd: 3  ID: 57005
    Reply: DSIGetStatus
    Request ID: 57005
    Machine type: Netatalk3.1.9
    AFP versions: AFP2.2,AFPX03,AFP3.1,AFP3.2,AFP3.3,AFP3.4
    UAMs: DHX2,DHCAST128
    Volume Icon & Mask: Yes
    Flags:
        SupportsCopyFile
        SupportsServerMessages
        SupportsServerSignature
        SupportsTCP/IP
        SupportsSrvrNotifications
        SupportsOpenDirectory
        SupportsUTF8Servername
        SupportsUUIDs
        SupportsExtSleep
        SupportsSuperClient
    Server name: myserver
    Signature:
    ea 56 61 0d bf 29 36 31 fa 6a 8a 24 a8 f0 cc 1d  .Va..)61.j.$....

    Network address: [fd00:0000:0000:0000:0000:0000:0001:0160]:10548 (IPv6 address + port)
    UTF8 Servername: myserver

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
