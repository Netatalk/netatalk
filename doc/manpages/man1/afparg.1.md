# Name

afparg — Send commands to an AFP server

# Synopsis

**afparg** [-1234567lVv] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*] [-f *command*]

# Description

**afparg** is a simple tool for sending commands to an AFP server with an authenticated user.
Use it to inspect files on the server, or perform trivial file operations.

Run *afparg -l* to list available commands and their arguments.

# Options

**-1**
: Use AFP 2.1 protocol version

**-2**
: Use AFP 2.2 protocol version

**-3**
: Use AFP 3.0 protocol version

**-4**
: Use AFP 3.1 protocol version

**-5**
: Use AFP 3.2 protocol version

**-6**
: Use AFP 3.3 protocol version

**-7**
: Use AFP 3.4 protocol version (default)

**-f** *command* *arguments*
: Command to execute

**-h** *host*
: Server hostname or IP address (default: localhost)

**-l**
: List available commands then exit

**-p** *port*
: Server port number (default: 548)

**-u** *user*
: Username for authentication

**-v**
: Verbose output

**-V**
: Very verbose output

**-w** *password*
: Password for authentication

# Configuration

The test runner AFP client only supports the ClearTxt UAM currently.
Configure the UAM in netatalk's afp.conf:

    [Global]
    uam list = uams_clrtxt.so

# Examples

List available commands and their arguments

    $ afparg -l
    FPResolveID CNID
    FPEnumerate dir
    FPCopyFile source dest
    FPLockrw d | r file
    FPLockw d | r file

Resolve a CNID to file name

    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPResolveID 18
    ======================
    FPResolveID with args:
    Trying to resolve id 18
    Resolved ID 18 to: 'AFP_Reference.pdf'

List files inside of a directory on the shared volume

    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPEnumerate "my dir"
    file1
    file2

Make a copy of a file on the shared volume

    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPCopyFile AFP_Reference.pdf AFP_Reference2.pdf
    ======================
    FPCopyFile with args:
    source: "AFP_Reference.pdf" -> dest: "AFP_Reference2.pdf"

Open a file's data fork with read/write lock

    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPLockrw d AFP_Reference2.pdf
    ======================
    FPOpen with read/write lock
    source: "AFP_Reference2.pdf"

# See Also

**afp_logintest**(1), **afp_spectest**(1), **afpd**(8)
