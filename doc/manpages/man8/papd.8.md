# Name

papd — AppleTalk print server daemon

# Synopsis

`papd [-d] [-f configfile] [-p printcap] [-P pidfile]`

`papd [ -v | -V ]`

# Description

`papd` is the AppleTalk printer daemon. This daemon accepts print jobs
from AppleTalk clients (typically Macintosh computers) using the Printer
Access Protocol (PAP). When used with System V printing systems, `papd`
spools jobs directly into an `lpd(8)` spool directory and wakes up `lpd`
after accepting a job from the network to have it re-examine the
appropriate spool directory. The actual printing and spooling is handled
entirely by `lpd`.

`papd` can also pipe the print job to an external program for
processing, and this is the preferred method on systems not using CUPS
to avoid compatibility problems with all the flavours of `lpd` in use.

As of Netatalk 2.0, CUPS is also supported. Simply using *cupsautoadd*
as the first papd.conf entry will share all CUPS printers automagically.
It is still possible to overwrite these defaults by individually
defining printer shares. See `papd.conf(5)` for details on the
configuration file format.

`papd` is typically started at boot time from system init scripts or
services. It first reads from its configuration file, `papd.conf`.

If no configuration file is given, the hostname of the machine is used
as the NBP name, and all options take their default value.

# Options

-d

> Do not fork or disassociate from the terminal.

-f <configfile\>

> Consult <configfile\> instead of `papd.conf` for the configuration
information.

-p <printcap\>

> Consult <printcap\> instead of `/etc/printcap` for LPD configuration
information.

-P <pidfile\>

> Specifies the file in which `papd` stores its process id.

-v | -V

> Print version information and exit.

# Authentication

PSSP (Print Server Security Protocol) is
an authentication protocol carried out
through postscript printer queries to the print server. Using PSSP
requires LaserWriter 8.6.1 or greater on the client Mac. The user will
be prompted to enter their username and password before they print. It
may be necessary to re-setup the printer on each client the first time
PSSP is enabled, so that the client can figure out that authentication
is required to print. You can enable PSSP on a per-printer basis. PSSP
is the recommended method of authenticating printers as it is more
robust than CAP-style authentication, described below.

CAP-style authentication gets its name
from the method CAP (Columbia AppleTalk Package) used to authenticate
its Mac clients' printing. This method requires that a user login to a
file share before they print. `afpd` records the username in a temporary
file named after the client's AppleTalk address, and it deletes the
temporary file when the user disconnects. Therefore CAP style
authentication will *not* work for clients connected to `afpd` via
TCP/IP. `papd` gets the username from the file with the same AppleTalk
address as the machine connecting to it. CAP-style authentication will
work with any Mac client. If both CAP and PSSP are enabled for a
particular printer, CAP will be tried first, then `papd` will fall back
to PSSP.

The list of UAMs to use for authentication (specified with the 'am'
option) applies to all printers. It is not possible to define different
authentication methods on each printer. You can specify the list of UAMS
multiple times, but only the last setting will be used. Currently,
*uams_guest.so* and *uams_clrtxt.so* are supported as printer
authentication methods. The guest method requires a valid username, but
not a password. The Cleartext UAM requires both a valid username and the
correct password.

> **NOTE**

> Print authentication is only supported on Mac OS 9 and earlier.

# Files

`papd.conf`

> Default configuration file.

`/etc/printcap`

> Printer capabilities database.

`.ppd`

> PostScript Printer Description file. papd answers configuration and font
queries from printing clients by consulting the configured PPD file.
Such files are available from Adobe, Inc., or from the printer's
manufacturer. If no PPD file is configured, papd will return the default
answer, possibly causing the client to send excessively large jobs.

# Caveats

`papd` accepts characters with the high bit set (a full 8-bits) from the
clients, but some PostScript printers (including Apple's LaserWriter
family) only accept 7-bit characters on their serial interface by
default. The same applies for some printers when they're accessed via
TCP/IP methods (remote LPR or socket). You will need to configure your
printer to accept a full 8 bits or take special precautions and convert
the printjob's encoding (e.g. by using *co="protocol=BCP"* when using
CUPS 1.1.19 or above).

When printing clients run Mac OS X 10.2 or later, take care that PPDs do
not make use of *\*cupsFilter:* comments unless the appropriate filters
are installed at the client's side, too.

# See also

`lp(1)`, `lpr(1)`, `lprm(1)`, `printcap(5)`, `lpc(8)`, `lpd(8)`, `papd.conf(8)`.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
