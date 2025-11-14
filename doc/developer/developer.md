Netatalk Architecture
=====================

Netatalk is an implementation of Apple Filing Protocol (AFP) over TCP.
The session layer used to carry AFP over TCP is called DSI.
Netatalk also supports the AppleTalk Protocol Suite for legacy Macs,
Lisas and Apple IIs via the **atalkd** daemon.

The complete stack looks like this on a BSD-derived system:

```txt
    AFP                          AFP
     |                            |
    ASP    PAP                   DSI
      \   /                       |
       ATP RTMP NBP ZIP AEP       | (port:548)
        |    |   |   |   |        |
   -+---------------------------------------------------+- (kernel boundary)
    |                    Socket                         |
    +-----------------------+------------+--------------+
    |                       |     TCP    |    UDP       |
    |          DDP          +------------+--------------+
    |                       |       IP v4 or v6         |
    +-----------------------+---------------------------+
    |                Network Interface                  |
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
