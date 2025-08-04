# Name

fce_listen — Netatalk Filesystem Change Events listener application

# Synopsis

**fce_listen** [-h *host*] [-p *port*]

# Description

**fce_listen** is a simple listener for Netatalk's Filesystem Change Event (FCE) protocol.
It will print out any UDP datagrams received from the AFP server to stdout.

# Options

**-h** *host*
: Server hostname or IP address

**-p** *port*
: Server port number

# Examples

Configure netatalk to emit FCE packets to a listener on localhost port 58585, in afp.conf:

    [Global]
    fce listener = localhost:58585
    fce version = 2
    fce events = fmod,fdel,ddel,fcre,dcre,fmov,dmov,login,logout

Listen to FCE events on localhost port 58585

    $ fce_listen -h localhost -p 58585
    Listening for Netatalk FCE datagrams on localhost:58585...
    FCE Start
    ID: 1, Event: FCE_LOGIN, pid: 429918, user: nobody, Path: 
    ID: 2, Event: FCE_LOGOUT, pid: 429918, user: nobody, Path: 
    FCE Start
    ID: 1, Event: FCE_LOGIN, pid: 429924, user: myuser, Path: 
    ID: 2, Event: FCE_FILE_DELETE, pid: 429924, user: myuser, Path: /srv/afpme/Read Me First copy
    ID: 3, Event: FCE_DIR_CREATE, pid: 429924, user: myuser, Path: /srv/afpme/untitled folder
    ID: 4, Event: FCE_DIR_MOVE, pid: 429924, user: myuser, source: /srv/afpme/untitled folder, Path: /srv/afpme/My Folder
    ID: 5, Event: FCE_FILE_MOVE, pid: 429924, user: myuser, source: /srv/afpme/LICENSE, Path: /srv/afpme/My Folder/LICENSE

# See Also

**afpd**(8)
