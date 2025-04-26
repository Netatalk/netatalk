# Name

papstatus — get the status of an AppleTalk-connected printer

# Synopsis

**papstatus** [-d] [-p *printer*] [*retrytime*]

# Description

**papstatus** is used to obtain the current status message from an
AppleTalk connected printer. It uses the Printer Access Protocol (PAP)
to obtain the status information.

If no printer is specified on the command line, **papstatus** looks for a
file called *.paprc* in the current directory and reads it to obtain the
name of a printer. The *.paprc* file should contain a single line of the
form *object:type@zone* where each of *object*, *:type,* and *@zone* are
optional. *type* and *zone* must be proceeded by \`*:*' and \`*@*'
respectively. Blank lines and lines the begin with a \`*\#*' are
ignored. *type* and *zone* default to *LaserWriter* and the zone of the
local host, respectively.

# Options

**-d**

> Turns on a debugging mode that prints some extra information to standard
error.

**-p** *printer*

> Get status from *printer* (do not consult any *.paprc* files to find a
printer name). The syntax for *printer* is the same as discussed above
for the *.paprc* file.

*retrytime*

> Normally, **papstatus** only gets the status from the printer once. If
*retrytime* is specified, the status is obtained repeatedly, with a
sleep of *retrytime* seconds between inquiring the printer.

# Files

*.paprc*

> file that contains printer name

# See Also

nbp(1), pap(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
