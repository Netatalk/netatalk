# Filesystem Change Events

Netatalk includes a filesystem change event (FCE) mechanism that allows **afpd** processes
to notify interested listeners about certain filesystem events via UDP network datagrams.

This feature is disabled by default.
To enable it,
set the **fce listener** option in *afp.conf* to the hostname
or IP address of the host that should listen for FCE events.

Netatalk distributes a simple FCE listener application called **fce_listen**.
It prints any UDP datagrams received from the AFP server.
Its source code also serves as documentation for the format of the UDP packets.

## FCE v1

The supported FCE v1 events are:

- file modification (fmod)

- file deletion (fdel)

- directory deletion (ddel)

- file creation (fcre)

- directory creation (dcre)

## FCE v2

FCE v2 events provide additional context such as the user who performed the action.
FCE v2 also adds the following events:

- file moving (fmov)

- directory moving (dmov)

- user login (login)

- user logout (logout)
