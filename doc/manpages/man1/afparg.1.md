# Name

afparg — Send commands to an AFP server

# Synopsis

**afparg** [-1234567Vv] [-A *uam*] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*] [-f *command*]

**afparg** -l

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

**-A** *uam*
: Select authentication with the specified UAM name or alias.
  Use *clrtxt* for the legacy ClearTxt login path, *dhx* for DHCAST128, and
  *dhx2* for DHX2.

**-f** *command* *arguments*
: Command to execute

**-h** *host*
: Server hostname or IP address (default: localhost)

**-l**
: List available commands then exit

**-p** *port*
: Server port number (default: 548)

**-u** *user*
: Username for authentication with AFP server (default: current uid)

**-v**
: Verbose output

**-V**
: Very verbose output

**-w** *password*
: Password for authentication with AFP server

# Configuration

By default, afparg uses ClearTxt; **-A clrtxt** selects that same legacy login
path explicitly. Pass **-A dhx** or **-A dhx2** to use the corresponding native
encrypted UAM. Configure the selected UAM in netatalk's
afp.conf:

    [Global]
    uam list = uams_dhx.so uams_dhx2.so uams_clrtxt.so

# Examples

List available commands and their arguments

    $ afparg -l
    FPResolveID CNID
    FPEnumerate dir
    FPCopyFile source dest
    FPLockrw d | r file [seconds]
    FPLockw d | r file [seconds]
    FPWrite file content
    FPRead file
    FPSetEA file attribute value
    FPGetEA file attribute
    FPSetInhibit file on | off
    FPByteLockHold d | r file seconds

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

Open a file's data fork with read/write lock. Without the optional
seconds argument the fork is held until interrupted (Ctrl-C); with it,
the fork is held for that many seconds, then closed cleanly with a
proper AFP logout

    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPLockrw d AFP_Reference2.pdf 10
    ======================
    FPOpen with read/write lock
    source: "AFP_Reference2.pdf"

Write a string to a file, read it back, and exchange Extended
Attributes — scriptable one-shots for cross-protocol checks

    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPWrite note.txt "hello"
    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPRead note.txt
    DATA:hello
    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPSetEA note.txt color red
    $ afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPGetEA note.txt color
    EA:red

Set or clear the DeleteInhibit attribute on a file

    afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPSetInhibit note.txt on

Hold an AFP byte-range lock (bytes 0-7) on a file's data fork for ten
seconds, then unlock and close cleanly

    afparg -h 10.0.0.8 -u myuser -w mypass -s "test volume" -f FPByteLockHold d note.txt 10

# See Also

**afp_logintest**(1), **afp_spectest**(1), **afpd**(8)
