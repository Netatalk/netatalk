# Name

pap — client interface to remote printers using Printer Access Protocol

# Synopsis

`pap [-A address] [-c] [-d] [-e] [-E] [-p nbpname] [-s statusfile] [-w] [-W] [FILES]`

# Description

`pap` is used to connect and send files to an AppleTalk connected
printer using the Apple Printer Access Protocol (PAP). When `pap` starts
execution, it tries to open a session with the printer using PAP, and
then downloads the *files* to the printer.

If no *files* are given on the command line, `pap` begins reading from
standard input.

If no printer is specified on the command line, `pap` looks for a file
called `.paprc` in the current working directory and reads it to obtain
the *nbpname* of a printer. Blank lines and lines that begin with a
\`*\#*' are ignored. *type* and *zone* default to *LaserWriter* and the
zone of the local host, respectively.

Note that `pap` is designed to be useful as a communication filter for
sending `lpd(8)` spooled print jobs to AppleTalk connected printers. See
`psf(8)` for hints on how to use it this way.

# Options

`-A` <address\>

> Connect to the printer with Appletalk address *address* and do not
consult the `.paprc` file to find a printer name. See `atalk_aton(3)`
for the syntax of *address*.

`-c`

> Take cuts. The PAP protocol specified a simple queuing procedure, such
that the clients tell the printer how long they have been waiting to
print. This option causes `pap` to lie about how long it has been
waiting.

`-d`

> Enable debug output.

`-e`

> Send any message from the printer to stderr instead of stdout. `psf(8)`
invokes `pap` with this option.

`-E`

> Don't wait for EOF from the printer. This option is useful for printers
which don't implement PAP correctly. In a correct implementation, the
client side should wait for the printer to return EOF before closing the
connection. Some clients don't wait, and hence some printers have
related bugs in their implementation.

`-p` <nbpname\>

> Connect to the printer named *nbpname* and do not consult the `.paprc`
file to find a printer name. See `nbp_name(3)` for the syntax of
*nbpname*.

`-s` <statusfile\>

> Update the file called *statusfile* to contain the most recent status
message from the printer. `pap` gets the status from the printer when it
is waiting for the printer to process input. The *statusfile* will
contain a single line terminated with a newline. This is useful when
`pap` is invoked by `psf(8)` within *lpd*'s spool directory.

`-w`

> Wait for the printer status to contain the word "waiting" before sending
the job. This is to defeat printer-side spool available on HP IV and V
printers.

`-W`

> Wait for the printer status to contain the word "idle" before sending
the job. This is to defeat printer-side spool available on HP IV and V
printers.

# Files

`.paprc`

> file read to obtain printer name if not specified on command line

# See Also

`nbp_name(3)`, `atalk_aton(3)`, `lpd(8)`, `psf(8)`.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
