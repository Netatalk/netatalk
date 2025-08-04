# Name

afp_speedtest — Simple AFP file transfer benchmarking tool

# Synopsis

**afp_speedtest** [-1234567aeiLnVvy] [-h *host*] [-p *port*] [-s *volume*] [-S *volume2*] [-u *user*]
[-w *password*] [-n *iterations*] [-d *size*] [-q *quantum*] [-F *file*] [-f *test*]

# Description

**afp_speedtest** is an AFP benchmark testsuite for read, write and copy operations.
It can be run using either AFP commands or POSIX syscalls,
handy for comparing netatalk speeds against other file transfer protocols.

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

**-a**
: Don't flush to disk after write

**-d** *size*
: File size (Mbytes, default: 64)

**-D**
: Use O_DIRECT in open flags (when in POSIX mode)

**-e**
: Use sparse file

**-f** *tests*
: Specific tests to run (Read, Write, Copy, ServerCopy, default: Write)

**-F** *file*
: Read from file in volume root folder (default: create a temporary file)

**-h** *host*
: Server hostname or IP address (default: localhost)

**-i**
: Interactive mode – prompt user before each test (used for debugging)

**-L**
: Use POSIX calls instead of AFP

**-n** *iterations*
: Number of test iterations to run (default: 1)

**-p** *port*
: Server port number (default: 548)

**-q** *size*
: Packet size (Kbytes, default: server quantum)

**-r** *number*
: Number of outstanding requests (default: 1)

**-R** *number*
: Number of not interleaved outstanding requests (default: 1)

**-s** *volume*
: Volume name to mount for testing

**-S** *volume*
: Volume name for second volume to mount for testing

**-u** *user*
: Username for authentication (default: current uid)

**-v**
: Verbose output

**-V**
: Very verbose output

**-w** *password*
: Password for authentication

**-y**
: Use a new file for each run (default: same file)

# Configuration

The test runner AFP client only supports the ClearTxt UAM currently.
Configure the UAM in netatalk's afp.conf:

    [Global]
    uam list = uams_clrtxt.so

# Examples

Run 5 iterations each of the four benchmark tests: Read,Write,Copy,ServerCopy

    $ afp_speedtest -h 10.0.0.10 -s speed1 -u myuser -w mypass -n 5 -f Read,Write,Copy,ServerCopy
    Read quantum 1048576, size 67108864 
    run	 microsec	  KB/s
    1	   251577	273154.84
    2	   222635	308664.31
    3	   226490	303410.66
    4	   228992	300095.53
    5	   220840	311173.16

    Write quantum 1048576, size 67108864
    run	 microsec	  KB/s
    1	   210057	327146.81
    2	   195791	350983.84
    3	   193921	354368.41
    4	   208458	329656.22
    5	   206792	332312.06

    Copy quantum 1048576, size 67108864 
    run	 microsec	  KB/s
    1	   418005	164398.70
    2	   392906	174900.55
    3	   400815	171449.36
    4	   392021	175295.39
    5	   390320	176059.33

    ServerCopy quantum 1048576, size 67108864 
    run	 microsec	  KB/s
    1	    20585	3338327.75
    2	    21986	3125601.50
    3	    21762	3157774.00
    4	    21913	3136014.00
    5	    20832	3298746.00

# See Also

**afp_lantest**(1), **afpd**(8)
