CNID Database Daemons
=====================

The CNID database daemons cnid_metad and cnid_dbd are an implementation of
the netatalk CNID database support that attempts to put all functionality
into separate daemons.
There is one cnid_dbd daemon per netatalk volume. The underlying database
structure is based on Berkeley DB.

Advantages
----------

* No locking issues or leftover locks due to crashed afpd daemons.
  Since there is only one thread of control accessing the database,
  no locking is needed and changes appear atomic.

* Berkeley DB transactions are difficult to get right with several
  processes attempting to access the CNID database simultaneously. This
  is much easier with a single process and the database can be made nearly
  crash-proof this way (at a performance cost).

* No problems with user permissions and access to underlying database
  files, the cnid_dbd process runs under a configurable user
  ID that normally also owns the underlying database
  and can be contacted by whatever afpd daemon accesses a volume.

* If an afpd process crashes, the CNID database is unaffected. If the
  process was making changes to the database at the time of the crash,
  those changes will be rolled back entirely (transactions).
  If the process was not using the database at the time of the crash,
  no corrective action is necessary. In any case, database consistency
  is assured.

Disadvantages
-------------

Performance in an environment of processes sharing the database
(files) is potentially better for two reasons:

1. IPC overhead.
2. r/o access to database pages is possible by more than one
   process at once, r/w access is possible for non overlapping regions.

The current implementation of cnid_dbd uses Unix domain sockets as
the IPC mechanism. While this is not the fastest possible method, it
is very portable and the cnid_dbd IPC mechanisms can be extended to
use faster IPC (like mmap) on architectures where it is
supported. As a ballpark figure, 20000 requests/replies to the cnid_dbd
daemon take about 0.6 seconds on a Pentium III 733 MHz running Linux
Kernel 2.4.18 using Unix domain sockets. The requests are "empty"
(no database lookups/changes), so this is just the IPC
overhead.

The effects of the advantages of simultaneous database access
have not been measured.

Installation and configuration
------------------------------

There are two executables that will be built in etc/cnid_dbd and
installed into the systems binaries directories of netatalk:
cnid_metad and cnid_dbd. cnid_metad should run all the
time with root permissions. It will be notified when an instance of
afpd starts up and will in turn make sure that a cnid_dbd daemon is
started for the volume that afpd wishes to access. The cnid_dbd daemon runs as
long as necessary and services any
other instances of afpd that access the volume. You can safely kill it
with SIGTERM, it will be restarted automatically by cnid_metad as soon
as the volume is accessed again.

cnid_dbd changes to the Berkeley DB directory on startup and sets
effective UID and GID to owner and group of that directory. Database and
supporting files should therefore be writeable by that user/group.

Current shortcomings
--------------------

* The parameter file parsing of db_param is very simpleminded.
  For example, there is no support for blanks (or weird characters) in
  filenames for the usock_file parameter.

* There is no protection against a malicious user connecting to the
  cnid_dbd socket and changing the database.
