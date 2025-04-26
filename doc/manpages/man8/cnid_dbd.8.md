# Name

cnid_dbd — CNID database access daemon

# Synopsis

**cnid_dbd**

**cnid_dbd** [-v | -V]

# Description

**cnid_dbd** provides an interface for storage and retrieval of catalog
node IDs (CNIDs) and related information to the *afpd* daemon. CNIDs are
a component of Macintosh based file systems with semantics that map not
easily onto Unix file systems. This makes separate storage in a database
necessary. **cnid_dbd** is part of the *CNID backend* framework of *afpd*
and implements the *dbd* backend.

**cnid_dbd** is never started via the command line or system startup
scripts but only by the *cnid_metad* daemon. There is one instance of
**cnid_dbd** per netatalk volume.

**cnid_dbd** uses the *Berkeley DB* database library and uses
transactionally protected updates. The *dbd* backend with transactions
will avoid corruption of the CNID database even if the system crashes
unexpectedly.

**cnid_dbd** inherits the effective userid and groupid from *cnid_metad*
on startup, which is normally caused by *afpd* serving a netatalk volume
to a client. It changes to the *Berkeley DB* database home directory
*dbdir* that is associated with the volume. If the userid inherited from
*cnid_metad* is 0 (root), **cnid_dbd** will change userid and groupid to
the owner and group of the database home directory. Otherwise, it will
continue to use the inherited values. **cnid_dbd** will then attempt to
open the database and start serving requests using filedescriptor
*clntfd*. Subsequent instances of *afpd* that want to access the same
volume are redirected to the running **cnid_dbd** process by *cnid_metad*
via the filedescriptor *ctrlfd*.

**cnid_dbd** can be configured to run forever or to exit after a period of
inactivity. If **cnid_dbd** receives a TERM or an INT signal it will exit
cleanly after flushing dirty database buffers to disk and closing
*Berkeley DB* database environments. It is safe to terminate **cnid_dbd**
this way, it will be restarted when necessary. Other signals are not
handled and will cause an immediate exit, possibly leaving the CNID
database in an inconsistent state (no transactions) or losing recent
updates during recovery (transactions).

The *Berkeley DB* database subsystem will create files named
log.xxxxxxxxxx in the database home directory *dbdir*, where xxxxxxxxxx
is a monotonically increasing integer. These files contain the
transactional database changes. They will be removed regularly, unless
the **logfile_autoremove** option is specified in the *db_param*
configuration file (see below) with a value of 0 (default 1).

# Options

**-v** | **-V**

> Show version and exit.

# Configuration

**cnid_dbd** reads configuration information from the file *db_param* in
the database directory *dbdir* on startup. If the file does not exist or
a parameter is not listed, suitable default values are used. The format
for a single parameter is the parameter name, followed by one or more
spaces, followed by the parameter value, followed by a newline. The
following parameters are currently recognized:

**logfile_autoremove**

> If set to 0, unused Berkeley DB transactional logfiles (log.xxxxxxxxxx
in the database home directory) are not removed on startup of **cnid_dbd**
and on a regular basis. Default: 1.

**cachesize**

> Determines the size of the Berkeley DB cache in kilobytes.
Default: 8192. Each **cnid_dbd** process grabs that much memory on top of
its normal memory footprint. It can be used to tune database
performance. The *db_stat* utility with the **-m** option that comes with
Berkeley DB can help you determine whether you need to change this
value. The default is pretty conservative so that a large percentage of
requests should be satisfied from the cache directly. If memory is not a
bottleneck on your system you might want to leave it at that value. The
*Berkeley DB Tutorial and Reference Guide* has a section *Selecting a
cache size* that gives more detailed information.

**flush_frequency**; **flush_interval**

> **flush_frequency** (Default: 1000) and **flush_interval** (Default: 1800)
control how often changes to the database are checkpointed. Both of
these operations are performed if either i) more than **flush_frequency**
requests have been received or ii) more than **flush_interval** seconds
have elapsed since the last save/checkpoint. Be careful to check your
hard disk configuration for on disk cache settings. Many IDE disks just
cache writes as the default behaviour, so even flushing database files
to disk will not have the desired effect.

**fd_table_size**

> is the maximum number of connections (filedescriptors) that can be open
for *afpd* client processes in *cnid_dbd.* Default: 512. If this number
is exceeded, one of the existing connections is closed and reused. The
affected *afpd* process will transparently reconnect later, which causes
slight overhead. On the other hand, setting this parameter too high
could affect performance in **cnid_dbd** since all descriptors have to be
checked in a *select()* system call, or worse, you might exceed the per
process limit of open file descriptors on your system. It is safe to set
the value to 1 on volumes where only one *afpd* client process is
expected to run, e.g. home directories.

**idle_timeout**

> is the number of seconds of inactivity before an idle **cnid_dbd** exits.
Default: 600. Set this to 0 to disable the timeout.

# Updating

Starting with Netatalk 2.1.1, automatic Berkeley DB updates are supported,
as long as Netatalk 2.1.0 or later was used to create the database.

In order to update between older Netatalk releases using different
Berkeley DB library versions, follow these steps:

- Stop the to be upgraded old version of Netatalk

- Using the old Berkeley DB utilities run
*db_recover -h path/to/.AppleDB*

- Using the new Berkeley DB utilities run
*db_upgrade -v -h path/to/.AppleDB -f cnid2.db*

- Again using the new Berkeley DB utilities run
*db_checkpoint -1 -h path/to/.AppleDB*

- Start the the new version of Netatalk

# See Also

cnid_metad(8), afpd(8), dbd(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
