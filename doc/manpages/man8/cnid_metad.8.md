# Name

cnid_metad — daemon for starting cnid_dbd daemons on demand

# Synopsis

`cnid_metad [-d] [-F configuration file]`

`cnid_metad [ -v | -V ]`

# Description

**cnid_metad** waits for requests from *afpd* to start up instances of the
*cnid_dbd* daemon. It keeps track of the status of a *cnid_dbd* instance
once started and will restart it if necessary. **cnid_metad** is normally
started at boot time by **netatalk**(8) and runs until shutdown.

# Options

**-d**

> **cnid_metad** will remain in the foreground and will also leave the
standard input, standard output and standard error file descriptors
open. Useful for debugging.

**-F** <configuration file\>

> Use *configuration file* as the configuration file.

**-v**, **-V**

> Show version and exit.

# Caveats

**cnid_metad** does not block or catch any signals apart from SIGPIPE. It
will therefore exit on most signals received. This will also cause all
instances of **cnid_dbd** started by that **cnid_metad** to exit
gracefully. Since state about and IPC access to the subprocesses is only
maintained in memory by **cnid_metad** this is desired behaviour. As soon
as **cnid_metad** is restarted **afpd** processes will transparently
reconnect.

# See Also

netatalk(8), cnid_dbd(8), afpd(8), dbd(1), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
