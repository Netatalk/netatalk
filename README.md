# About Netatalk
[![Build Status](https://github.com/Netatalk/netatalk/actions/workflows/build.yml/badge.svg)](https://github.com/Netatalk/netatalk/actions/workflows/build.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[<img src="https://sonarcloud.io/images/project_badges/sonarcloud-orange.svg" height="20" />](https://sonarcloud.io/summary/overall?id=Netatalk_netatalk)

Netatalk is a freely-available Open Source fileserver that implements the Apple Filing Protocol (AFP) 3.4 over TCP/IP.
AFP was the primary file sharing protocol for Apple Macintosh and Apple II computers from 1987 to 2013.
A *NIX/*BSD system running Netatalk is capable of serving many Macintosh clients simultaneously as an AppleShare file server.

# Why Should I Use Netatalk?
If you're running a network of older Macs, in particular those running OS X 10.8 Mountain Lion or earlier, all the way back to Classic Mac OS,
are well served by running a Netatalk AppleShare server. The latest macOS at the time of writing (macOS 13 Ventura) still comes with an AFP client,
so Netatalk can act as a seamless bridge between new and old Macs.

Compared to other Open Source file sharing solutions such as NFS or Samba, Netatalk delivers high transfer speeds and full integration
of Classic Mac OS metadata (resource forks) as well as user authentication methods (UAMs) compatible with Classic Mac OS clients that don't support modern cryptography.

Modern AFP features such as Bonjour, Time Machine, and Spotlight are also supported.

# AppleTalk
Support for the AppleTalk network stack was removed in Netatalk v3. If you need AppleTalk to network very old Macs or Apple II computers, please use Netatalk v2.

Find the latest Netatalk v2 releases in the [Releases section](https://github.com/Netatalk/netatalk/releases?q=%22Netatalk+2%22&expanded=false).

# Docker

Netatalk comes with a Dockerfile and entry point script for running a containerized AFP server.

For simplicity, exactly one user, one shared volume, and one Time Machine volume is supported. It is hard coded to output afpd logs to the container's stdout, default info log level.

Make sure you have Docker Engine installed, then build the netatalk container:

```
docker build -t netatalk3 .
```

Once the container is ready, run it with docker run or compose.
When running, expose either port 548 for AFP, or use host networking.
The former option is more secure, but you will have to manually specify the IP address when connecting to the file server.

It is recommended to set up either a bind mount, or a Docker managed volume for persistent storage.
Without this, the shared volume be stored in volatile storage that is lost upon container shutdown.

## Constraints

In order to use Zeroconf service discovery, the container requires the "host" network driver and NET_ADMIN capabilities.

Additionally, we rely on the host's DBUS for Zeroconf, achieved with a bind mount for `/var/run/dbus:/var/run/dbus`.

Note that the Dockerfile currently only supports Avahi for Zeroconf; no mDNS support at present.

## Environment Variables

### Mandatory

These are required to set the credentials used to authenticate with the file server.

- `AFP_USER`
- `AFP_PASS`

### Optional

- `AFP_GROUP` <- group that owns the shared volume, and that AFP_USER gets assigned to
- `AFP_UID` <- specify user id of AFP_USER
- `AFP_GID` <- specify group id of AFP_USER
- `SERVER_NAME` <- the name of the server reported to Zeroconf
- `SHARE_NAME` <- the name of the file sharing volume
- `AFP_LOGLEVEL` <- the verbosity of logs; default is "info"
- `MANUAL_CONFIG` <- when non-zero, skip netatalk config file modification, allowing you to manually manage them

Sample docker-compose.yml with docker managed volume and Zeroconf
```
version: "3"

services:
  netatalk3:
    build: .
    image: netatalk3:latest
    network_mode: "host"
    cap_add:
      - NET_ADMIN
    volumes:
      - afpshare:/mnt/afpshare
      - afpbackup:/mnt/afpbackup
      - /var/run/dbus:/var/run/dbus
    environment:
      - "AFP_USER=atalk"
      - "AFP_PASS=atalk"
volumes:
  afpshare:
  afpbackup:
```

# Contributions
Bug reports and feature requests can be filed as GitHub issue ticket:

https://github.com/Netatalk/netatalk/issues

Before contributing code to the project, please read the guidelines in the wiki on how to prepare a PR that is likely to be accepted by the maintainers:

https://github.com/Netatalk/netatalk/wiki/Developer-Notes

PRs are automatically picked up by GitHub CI, which runs the builds, integration tests, as well as static analysis scan on SonarCloud.

# Documentation
The latest version of the Netatalk manual can be found at:

https://netatalk.io/3.1/htmldocs/

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
