# Name

netatalk-diag — collect diagnostics of a slow AFP server to send to its
developers

# Synopsis

**netatalk-diag** [-c *client*] [-u *user*] [-p *pid*] [-i *interface*]
[-o *folder*] [-d *seconds*] [-M *MiB*] [-t [-s]] [-m] [-b *entries*] [-l]
[-F]

# Description

**netatalk-diag** records what an AFP server does, typically to collect
diagnostics for a slow server, and writes anonymized reports that a
developer can read without access to the server. It captures the AFP
traffic, takes the kernel's counters before and after, and with **-t**
traces one **afpd** session. When the run ends, it writes the reports,
takes every file, folder, volume, user and host name and every address out
of them, checks the result against the names and addresses it saw, and
packs it into *netatalk-diagnostics-YYYYMMDD-HHMMSS.tar.gz* in its output
folder, named for the time the run started.

Run it as root on the server while the client is connected. It finds the
client's **afpd** session process and records until Enter is pressed, the
time given with **-d** passes, or the capture reaches its size limit. Ctrl-C
during the capture ends it early, and the reports are still written. At any
other step, Ctrl-C stops the run without reports; a session that **-m**
switched to maxdebug goes back to its log level, or the message says how to
send it back.

It needs **tcpdump** and **perl** 5.14 or later. Tracing needs **dtrace**
on FreeBSD, and **bpftrace** with a mounted tracefs on Linux. The request
trace needs an **afpd** built with the DTrace probes, which **afpd -V**
reports. FreeBSD and Linux get every report. On other systems, the tool
records the capture and the configuration only.

# Options

**-c** *client*
: Capture and trace only the client at this address.

**-u** *user*
: Trace the session of this user.

**-p** *pid*
: Trace the **afpd** session process with this pid.

**-i** *interface*
: Capture on this interface. The default is the interface of the route to
the client, or of the default route.

**-o** *folder*
: Write to this folder, which must not exist. The default is
*/var/tmp/netatalk-diag-YYYYMMDD-HHMMSS*. A run writes only in its own
folder, so it never replaces the files of an earlier run. Every folder
above it must be root's, and writable by root alone or sticky, as
*/var/tmp* is, so that no other user can rename it.

**-d** *seconds*
: Stop after this many seconds. Without it, the run stops when Enter is
pressed, which needs a terminal.

**-M** *MiB*
: Stop the capture at this size. The default is 1024 MiB, or a quarter of
the free space in the output folder if that is less.

**-t**
: Trace the session: per request, the time **afpd** spent on the CPU,
asleep and waiting to run, what it slept on, and its CNID calls. DTrace
traces on FreeBSD, and bpftrace on Linux. An **afpd** without the DTrace
probes gets its system calls counted instead, as with **-s**, which adds
to the time of every request. On Linux, every run also samples the
session's thread states.

**-s**
: With **-t**, count the session's system calls instead of timing its
requests. These probes add to the time of every request, so this is a run
of its own.

**-m**
: Log the session at maxdebug level and dump its directory cache before
and after the run. Logging at this level roughly doubles request latency,
so compare only runs taken the same way. It needs a terminal, as it asks
for steps on the client. If the session ends during the run, the reports
hold what it wrote until then.

**-b** *entries*
: After the capture, time a read-only walk of at most this many entries on
each volume: listing folders, reading each entry's attributes and, on
Linux, the names of its extended attributes. It shows whether the file
system itself is slow.

**-l**
: Start before the client connects, to time its connection and login. No
session is traced.

**-F**
: Package the reports even when the leak check finds a match.

# Reports

The archive holds one folder of the same name, which also stays in the
output folder. Its *README.txt* lists every file. The main ones:

*summary.txt*
: Where the time went in each phase: **afpd** working on requests, replies
on their way to the client, written data arriving, or **afpd** waiting for
the client's next request, with a verdict. Then TCP health, reads and
writes, logins, the main counter changes, notes on settings with a known
cost, and what was not collected.

*report-pcap.txt*, *requests.csv*
: The AFP requests per phase and command, and every request with its times,
sizes and fork.

*report-counters.txt*, *report-trace.txt*
: The kernel and **afpd** counter changes over the run, and the trace's
totals.

# Phases

A run can mark phases of what the client does. On the Mac, in Terminal,
with *VOL* the volume's name as */Volumes* shows it:

    stat "/Volumes/VOL/navmark-down"

starts a phase named *down*. The **stat** fails with "No such file or
directory", which is expected. The reports then break each phase out.

The reports keep each label as typed, and the leak check does not look at
labels. Use a label that names nothing private, such as *cold* or *warm*.

# Privacy

The raw capture, the **afpd** log and the directory cache dumps hold names.
They stay in the output folder, readable by root alone, and only the
folder of reports goes into the archive. In it, a path keeps its depth as
/+/+, the request reports name files and folders by tokens that change with
every run, *afp.conf* keeps its option names with every identifying value
removed, users and clients are numbered, and ZFS datasets and pools read
DATASETn and POOLn. Before packaging, **netatalk-diag** searches the
shared files for the names, paths, users, hosts and addresses it saw: all
but *README.txt* and the counter files, *report-counters.txt* and those
whose names begin with *counters-* or *vmstat-z-*, whose lines hold kernel
counter and disk device names and numbers. A match stops the run with the
file and line, and **-F** overrides it. Read the reports before you send
them.

Once packed, the folder of reports and the archive can be read by any
user on the server who knows their path. With **-F**, or a phase label
that names something private, they can hold private names.

# Exit Status

0 when the run wrote its archive and nothing failed, 1 when the
collection, the capture, a trace or the packing failed, 2 for a usage
error, 3 when the leak check stopped the packaging, and 130 when Ctrl-C or
a signal stopped the run before its reports.

# Examples

Record a slow folder on the client at 192.0.2.10, with a trace, until Enter
is pressed:

    netatalk-diag -c 192.0.2.10 -t

Record for two minutes, without a terminal:

    netatalk-diag -c 192.0.2.10 -d 120

Time a client's login:

    netatalk-diag -l -c 192.0.2.10 -d 60

# See Also

**afpd**(8), **afp.conf**(5), **tcpdump**(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
