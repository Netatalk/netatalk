Information for Netatalk Developers
===================================

For basic installation instructions, see the Installation chapter
in the html manual published on the [Netatalk homepage](https://netatalk.io)
and the [Installation Quick Start Guide](https://netatalk.io/install).

Netatalk is an implementation of Apple Filing Protocol (AFP) over TCP.
The session layer used to carry AFP over TCP is called DSI.

Netatalk also supports the AppleTalk Protocol Suite for legacy Macs,
Lisas and Apple IIs via the "atalkd" daemon.

The complete stack looks like this on a BSD-derived system:

```txt
    AFP                          AFP
     |                            |
    ASP    PAP                   DSI
      \   /                       |
       ATP RTMP NBP ZIP AEP       |
        |    |   |   |   |        |
   -+---------------------------------------------------+- (kernel boundary)
    |                    Socket                         |
    +-----------------------+------------+--------------+
    |                       |     TCP    |    UDP       |
    |          DDP          +------------+--------------+
    |                       |           IP              |
    +-----------------------+---------------------------+
    |                Network-Interface                  |
    +---------------------------------------------------+
```

When built without AppleTalk support, the network stack looks something like this:

```txt
          AFP
           |
          DSI
           |
           | (port:548)
           |
   -+---------------------------+- (kernel boundary)
    |         Socket            |
    +------------+--------------+
    |     TCP    |    UDP       |
    +------------+--------------+
    |       IP v4 or v6         |
    +---------------------------+
    |     Network Interface     |
    +---------------------------+
```

Error checking and logging
==========================

We want rigid error checking and concise log messages. This often leads
to significant code bloat where the relevant function call is buried in error
checking and logging statements.
In order to alleviate error checking and code readability, we provide a set
of error checking macros in <atalk/errchk.h>. These macros compare the return
value of statements against 0, NULL, -1 (and maybe more, check it out).
Every macro comes in four flavours: EC_CHECK, EC_CHECK_LOG, EC_CHECK_LOG_ERR
and EC_CHECK_CUSTOM:

* EC_CHECK just checks the CHECK
* EC_CHECK_LOG additionally logs the stringified function call.
* EC_CHECK_LOG_ERR allows specifying the return value
* EC_CHECK_CUSTOM allows custom actions

The macros EC_CHECK* unconditionally jump to a cleanup label where the
necessary cleanup can be done alongside controlling the return value.
EC_CHECK_CUSTOM doesn't do that, so an extra "goto EC_CLEANUP" may be
performed as appropriate.

Examples
--------

stat() without EC macro:

```c
  static int func(const char *name) {
    int ret = 0;
    ...
    if ((ret = stat(name, &some_struct_stat)) != 0) {
      LOG(...);
      ret = -1; /* often needed to explicitly set the error indicating return value */
      goto cleanup;
    }

    return ret;

  cleanup:
    ...
    return ret;
  }
```

stat() with EC macro:

```c
  static int func(const char *name) {
    EC_INIT; /* expands to int ret = 0; */

    char *uppername = NULL
    EC_NULL(uppername = strdup(name));
    EC_ZERO(strtoupper(uppername));

    EC_ZERO(stat(uppername, &some_struct_stat)); /* expands to complete if block from above */

    EC_STATUS(0);

EC_CLEANUP:
    if (uppername) free(uppername);
    EC_EXIT;
  }
```

A boilerplate function template is:

```c
int func(void)
{
    EC_INIT;

    ...your code here...

    EC_STATUS(0);

EC_CLEANUP:
    EC_EXIT;
}
```

CNID Database Daemons
=====================

The CNID database daemons cnid_metad and cnid_dbd are an implementation of
the netatalk CNID database support that attempts to put all functionality
into separate daemons.
There is one cnid_dbd daemon per netatalk volume. The underlying database
structure is based on Berkeley DB.

Advantages
----------

* No locking issues or leftover locks due to crashed afpd daemons any
  more. Since there is only one thread of control accessing the
  database, no locking is needed and changes appear atomic.

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

* Performance in an environment of processes sharing the database
  (files) is potentially better for two reasons:

  i)  IPC overhead.
  ii) r/o access to database pages is possible by more than one
      process at once, r/w access is possible for non overlapping regions.

  The current implementation of cnid_dbd uses unix domain sockets as
  the IPC mechanism. While this is not the fastest possible method, it
  is very portable and the cnid_dbd IPC mechanisms can be extended to
  use faster IPC (like mmap) on architectures where it is
  supported. As a ballpark figure, 20000 requests/replies to the cnid_dbd
  daemon take about 0.6 seconds on a Pentium III 733 MHz running Linux
  Kernel 2.4.18 using unix domain sockets. The requests are "empty"
  (no database lookups/changes), so this is just the IPC
  overhead.

  I have not measured the effects of the advantages of simultaneous
  database access.

Installation and configuration
------------------------------

There are two executables that will be built in etc/cnid_dbd and
installed into the systems binaries directories of netatalk
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

* The parameter file parsing of db_param is very simpleminded. It is
easy to cause buffer overruns and the like.
Also, there is no support for blanks (or weird characters) in
filenames for the usock_file parameter.

* There is no protection against a malicious user connecting to the
cnid_dbd socket and changing the database.

Spotlight Compatible Indexing
=============================

Starting with version 3.1 Netatalk supports Spotlight searching.
Netatalk uses GNOME TinySPARQL/[LocalSearch](https://gnome.pages.gitlab.gnome.org/localsearch/)
(previously known as Tracker) as metadata store, indexer and search engine.

Limitations and notes
---------------------

* Large filesystems

    Tracker on Linux uses the inotify Kernel filesystem change event API
    for tracking filesystem changes. On large filesystems this may be
    problematic since the inotify API doesn't offer recursive directory
    watches but instead requires that for every subdirectory watches must
    be added individually.

    On Solaris the FEN file event notification system is used. It is
    unknown which limitations and resource consumption this Solaris
    subsystem may have.

    We therefore recommend to disable live filesystem monitoring and let
    Tracker periodically scan filesystems for changes instead, see the
    Tracker configuration options enable-monitors and crawling-interval below.

* Indexing home directories

    A known limitation with the current implementation means that shared
    volumes in a user's home directory does not get indexed by Spotlight.

    As a workaround, keep the shared volumes you want to have indexed
    elsewhere on the host filesystem.

Supported metadata attributes
-----------------------------

The following table lists the supported Spotlight metadata attributes,
based on Apple's [MDItem object](https://developer.apple.com/documentation/coreservices/mditemref/)
Core Services file metadata standard.

| Description | Spotlight Key |
|-------------|---------------|
| Name | kMDItemDisplayName, kMDItemFSName |
| Document content (full text search) | kMDItemTextContent |
| File type | \_kMDItemGroupId, kMDItemContentTypeTree |
| File modification date | kMDItemFSContentChangeDate, kMDItemContentModificationDate, kMDItemAttributeChangeDate |
| Content Creation date | kMDItemContentCreationDate |
| The author, or authors, of the contents of the file | kMDItemAuthors, kMDItemCreator |
| The name of the country where the item was created | kMDItemCountry |
| Duration | kMDItemDurationSeconds |
| Number of pages | kMDItemNumberOfPages |
| Document title | kMDItemTitle |
| The width, in pixels, of the contents. For example, the image width or the video frame width | kMDItemPixelWidth |
| The height, in pixels, of the contents. For example, the image height or the video frame height | kMDItemPixelHeight |
| The color space model used by the document contents | kMDItemColorSpace |
| The number of bits per sample | kMDItemBitsPerSample |
| Focal length of the lens, in millimeters | kMDItemFocalLength |
| ISO speed | kMDItemISOSpeed |
| Orientation of the document. Possible values are 0 (landscape) and 1 (portrait) | kMDItemOrientation |
| Resolution width, in DPI | kMDItemResolutionWidthDPI |
| Resolution height, in DPI | kMDItemResolutionHeightDPI |
| Exposure time, in seconds | kMDItemExposureTimeSeconds |
| The composer of the music contained in the audio file | kMDItemComposer |
| The musical genre of the song or composition | kMDItemMusicalGenre |
