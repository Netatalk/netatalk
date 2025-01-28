# Name

aecho - send AppleTalk Echo Protocol packets to network hosts

# Synopsis

`aecho [-c count] [ address | nbpname ]`

# Description

**aecho** repeatedly sends an Apple Echo Protocol (AEP) packet to the host
specified by the given AppleTalk **address** or **nbpname** and reports
whether a reply was received. Requests are sent at the rate of one per
second.

**address** is parsed by **atalk_aton**(3). **nbpname** is parsed by
**nbp_name**(3). The nbp type defaults to \`*Workstation*'.

When **aecho** is terminated, it reports the number of packets sent, the
number of responses received, and the percentage of packets lost. If any
responses were received, the minimum, average, and maximum round trip
times are reported.

# Examples

Check to see if a particular host is up and responding to AEP packets:

    example% aecho bloodsport
    11 bytes from 8195.13: aep_seq=0. time=10. ms
    11 bytes from 8195.13: aep_seq=1. time=10. ms
    11 bytes from 8195.13: aep_seq=2. time=10. ms
    11 bytes from 8195.13: aep_seq=3. time=10. ms
    11 bytes from 8195.13: aep_seq=4. time=10. ms
    11 bytes from 8195.13: aep_seq=5. time=9. ms
    ^C
    ----8195.13 AEP Statistics----
    6 packets sent, 6 packets received, 0% packet loss
    round-trip (ms)  min/avg/max = 9/9/10

# Options

**-c** <count\>

> Stop after *count* packets.

# See Also

ping(1), atalk_aton(3), nbp_name(3), aep(4), atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
