# Name

atalk — AppleTalk protocol family

# Synopsis

    #include <sys/types.h>
    #include <netatalk/at.h>

# Description

The AppleTalk protocol family is a collection of protocols layered above
the Datagram Delivery Protocol (DDP), and using AppleTalk address
format. The AppleTalk family may provide SOCK_STREAM (ADSP), SOCK_DGRAM
(DDP), SOCK_RDM (ATP), and SOCK_SEQPACKET (ASP). Currently, only DDP is
implemented in the kernel; ATP and ASP are implemented in user level
libraries; and ADSP is planned.

# Addressing

AppleTalk addresses are three byte quantities, stored in network byte
order. The include file <*netatalk/at.h*\> defines the AppleTalk
address format.

Sockets in the AppleTalk protocol family use the following address
structure:

    struct sockaddr_at {
        short sat_family;
        unsigned char sat_port;
        struct at_addr sat_addr;
        char sat_zero[ 8 ];
    };

The port of a socket may be set with `bind(2)`. The node for *bind* must
always be *ATADDR_ANYNODE*: \`\`this node.'' The net may be
*ATADDR_ANYNET* or *ATADDR_LATENET*. *ATADDR_ANYNET* corresponds to the
machine's \`\`primary'' address (the first configured). *ATADDR_LATENET*
causes the address in outgoing packets to be determined when a packet is
sent, i.e. determined late. *ATADDR_LATENET* is equivalent to opening
one socket for each network interface. The port of a socket and either
the primary address or *ATADDR_LATENET* are returned with
`getsockname(2)`.

# See Also

`bind(2)`, `getsockname(2)`, `atalkd(8)`.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
