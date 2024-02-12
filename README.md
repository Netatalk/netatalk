# About Netatalk
[![Build Status](https://github.com/Netatalk/netatalk/actions/workflows/build.yml/badge.svg)](https://github.com/Netatalk/netatalk/actions/workflows/build.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[<img src="https://sonarcloud.io/images/project_badges/sonarcloud-orange.svg" height="20" />](https://sonarcloud.io/summary/overall?id=Netatalk_netatalk&branch=branch-netatalk-2-3)

Netatalk is a freely-available Open Source fileserver that implements the Apple Filing Protocol (AFP) v3.3 over TCP/IP and AppleTalk. A *NIX/*BSD system running Netatalk is capable of serving many Macintosh and Apple II clients simultaneously as an AppleShare file server.

AFP was the primary file sharing protocol for Apple Macintosh and Apple II computers from 1987 to 2013. Mac OS X 10.9 Mavericks removed the AFP *server*, but macOS retains the AFP *client* to this day, and can therefore connect to an AFP fileserver served by Netatalk. This makes Netatalk an effective bridge between the very earliest networked Apple machines, and the very latest. 

# Why Should I Use Netatalk?

If you're running a network of older Macs, in particular those running OS X 10.8 Mountain Lion or earlier, all the way back to Classic Mac OS,
are well served by running a Netatalk AppleShare server. The latest macOS at the time of writing (macOS 14 Sonoma) still comes with an AFP client,
so Netatalk can act as a seamless bridge between new and old Macs.

Compared to other Open Source file sharing solutions such as NFS or Samba, Netatalk delivers high transfer speeds and full integration
of Classic Mac OS metadata (resource forks) as well as user authentication methods (UAMs) compatible with Classic Mac OS clients that don't support modern cryptography.

Modern AFP features such as Bonjour, and Time Machine are also supported.

# AppleTalk

Netatalk v2 supports the legacy AppleTalk protocol, which means it can act as a bridge between older Macs, networked Apple IIs, and the very latest macOS systems. It is AFP 3.3 compliant with AFP 1.1 and 2.2 backwards compatibility.

The included print server daemon can provide Apple II and Macintosh clients with the ability to print to AppleTalk-only printers. In addition, Netatalk is fully integrated with CUPS, allowing any networked Mac to discover and print to a CUPS/AirPrint compatible printer on the network.

Additionally, Netatalk can be used to act as an AppleTalk router, providing both segmentation and zone names in traditional Macintosh networks. 

# Docker

Netatalk comes with a Dockerfile and entry point script for running a containerized AFP server and AppleTalk router.
It is configured with the DHX2 UAM for authentication with macOS or Mac OS X, and RandNum UAM for authentication with Classic Mac OS, Macintosh System Software 6 and 7, and GS/OS.

For simplicity, exactly one user and one shared volume is supported. It is hard coded to output afpd logs to the container's stdout, default info log level.

Make sure you have Docker Engine installed, then build the netatalk container:

```
docker build -t netatalk2 .
```

Once the container is ready, run it with docker run or compose.
It is recommended to set up either a bind mount, or a Docker managed volume for persistent storage.
Without this, the shared volume be stored in volatile storage that is lost upon container shutdown.

## Constraints

The container requires the "host" network driver, and NET_ADMIN capabilities, to allow AppleTalk routing and Zeroconf.

We currently rely on the host's DBUS for Zeroconf service discovery via Avahi.
You need a bind mount for `/var/run/dbus:/var/run/dbus` in order for Bonjour service discovery to work.

Note that the Dockerfile currently only supports Avahi for Zeroconf; no mDNS support at present.

## Printing

The CUPS administrative web app should be running on port 631, which is exposed to the host machine by default since we are using the `host` network driver. This is used for configuring CUPS compatible printers for use with the papd print server daemon.

You may have to restart papd (or the entire container) after adding a CUPS printer for it to be picked up as an AppleTalk printer.

## Environment Variables

### Mandatory

- `AFP_USER` <- the authorized username
- `AFP_PASS` <- password with max length of 8 characters (Classic Mac OS limitation)
- `ATALKD_INTERFACE` <- the host network interface to broadcast AppleTalk from

### Optional

- `SERVER_NAME` <- the name of the server as displayed in the Chooser or Network drawer (default is hostname)
- `SHARE_NAME` <- the name of the shared volume (default is the final segment of the path)
- `AFP_GROUP` <- group that owns the shared volume, and that AFP_USER gets assigned to
- `AFP_UID` <- specify user id of AFP_USER
- `AFP_GID` <- specify group id of AFP_USER
- `AFPD_OPTIONS` <- options to append to afpd.conf
- `AVOLUMES_OPTIONS` <- options to append to AppleVolumes.default
- `ATALKD_OPTIONS` <- options to append to atalkd.conf
- `TZ` <- time zone for the timelord time server (e.g. `America/Chicago`)

Refer to the Netatalk manual for a list of available options to append each of the config files.

Sample docker-compose.yml with docker managed volume
```
version: "3"

services:
  netatalk2:
    build: .
    image: netatalk2:latest
    network_mode: "host"
    cap_add:
      - NET_ADMIN
    volumes:
      - afpshare:/mnt/afpshare
      - /var/run/dbus:/var/run/dbus
    environment:
      - "SERVER_NAME=Netatalk Server"
      - "SHARE_NAME=Shared Volume"
      - "AFP_USER=atalk"
      - "AFP_PASS=atalk"
      - "AFPD_OPTIONS=-icon -mimicmodel RackMac"
      - "AVOLUMES_OPTIONS=options:limitsize"
      - "ATALKD_INTERFACE=eth0"
      - "ATALKD_OPTIONS=-router -phase 2 -net 0-65534 -zone NETATALK"
volumes:
  afpshare:
```

# Contributions

Bug reports and feature requests can be filed as GitHub issue ticket:

https://github.com/Netatalk/netatalk/issues

Before contributing code to the project, please read the guidelines in the wiki on how to prepare a PR that is likely to be accepted by the maintainers:

https://github.com/Netatalk/netatalk/wiki/Developer-Notes

PRs are automatically picked up by GitHub CI, which runs the builds, integration tests, as well as static analysis scan on SonarCloud.

# Documentation

The latest version of the Netatalk manual can be found at:

https://netatalk.io/2.3/htmldocs/

Each Netatalk component also has a *NIX man page which can be accessed on the command line, f.e. `man afpd`.

# Wiki

Collaborative articles can be found on the Netatalk wiki hosted on GitHub.

Please feel free to edit existing and create new articles.

https://github.com/Netatalk/netatalk/wiki

# Mailing Lists

Netatalk contributors and maintainers interact with each other primarily on the netatalk-admins and netatalk-devel mailing list.

Subscribe and participate, or read archived discussion threads at:

https://sourceforge.net/p/netatalk/mailman/

# Website

The Netatalk website is where new releases and other news about the project are posted.

https://netatalk.io
