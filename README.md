# About Netatalk

[![Build Status](https://github.com/Netatalk/netatalk/actions/workflows/build.yml/badge.svg)](https://github.com/Netatalk/netatalk/actions/workflows/build.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[<img src="https://sonarcloud.io/images/project_badges/sonarcloud-orange.svg" height="20" />](https://sonarcloud.io/summary/overall?id=Netatalk_netatalk)

Netatalk is a Free and Open Source file server that implements the Apple Filing Protocol (AFP) 3.4 over TCP/IP.
AFP is the primary file sharing protocol used on Apple II, Classic Mac OS, and Mac OS X, as well as one of several supported protocols on macOS.
A *NIX/*BSD system running Netatalk provides high-speed AppleShare file sharing for macOS and Classic Mac OS clients.

Modern AFP features such as Bonjour, Time Machine, and Spotlight are supported.

# Why Should I Use Netatalk?

If you have a local network of Macs - macOS, Mac OS X, and all the way back to Classic Mac OS -
running a Netatalk AppleShare server allows you to share, collaborate on, and back up files comfortably.
The latest macOS at the time of writing (macOS 14 Sonoma) ships with an AFP client,
so Netatalk can act as a seamless bridge between new and old Macs.

Compared to other Open Source file sharing solutions such as NFS or FTP, Netatalk delivers high transfer speeds and full integration
of Classic Mac OS metadata (resource forks).

And while Samba is a great option for file sharing now when SMB is the primary file sharing protocol in macOS,
Netatalk has reportedly better performance, as well as stronger backwards compatibility with OS X 10.8 Mountain Lion and earlier.

# AppleTalk

Support for the AppleTalk network stack was removed in Netatalk v3. If you need AppleTalk to network very old Macs or Apple II computers, please use *old stable* Netatalk v2.

Find the latest Netatalk v2 releases in the [Releases section](https://github.com/Netatalk/netatalk/releases?q=%22Netatalk+2%22&expanded=false).

# Docker

Netatalk comes with a Dockerfile and entry point script for running a containerized AFP server.

For simplicity, exactly one user, one shared volume, and one Time Machine volume is supported. It is hard coded to output afpd logs to the container's stdout, default info log level.

Make sure you have Docker Engine installed, then build the netatalk container:

```
docker build -t netatalk3 .
```

Alternatively, fetch a pre-built docker container from [Docker Hub](https://hub.docker.com/u/netatalk).

## How to Run

Once the container is ready, run it with `docker run` or `docker compose`.
When running, expose either port 548 for AFP, or use the `host` network driver.
The former option is more secure, but you will have to manually specify the IP address when connecting to the file server.

It is recommended to set up either a bind mount, or a Docker managed volume for persistent storage.
Without this, the shared volume be stored in volatile storage that is lost upon container shutdown.

Sample `docker-compose.yml` with docker managed volume and Zeroconf

```
version: "3"

services:
  netatalk:
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

Sample `docker run` command. Substitute `/path/to/share` with an actual path on your file system with appropriate permissions.

```
docker run --rm --network host --cap-add=NET_ADMIN --volume "/path/to/share:/mnt/afpshare" --volume "/path/to/backup:/mnt/afpbackup" --volume "/var/run/dbus:/var/run/dbus" --env AFP_USER=atalk --env AFP_PASS=atalk --name netatalk netatalk3:latest
```

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

# Webmin module

An administrative GUI webapp in the form of a first-party module for Webmin can be found in a sister repository:

https://github.com/Netatalk/netatalk-webmin

See the README in that repo for instructions how to install and get started with the module.

This wiki page provides an overview of the module's feature set:

https://github.com/Netatalk/netatalk/wiki/Webmin-Module

# Contributions

Bug reports and feature requests can be filed as GitHub issue ticket:

https://github.com/Netatalk/netatalk/issues

Before contributing code to the project, please read the guidelines in the wiki on how to prepare a PR that is likely to be accepted by the maintainers:

https://github.com/Netatalk/netatalk/wiki/Developer-Notes

PRs are automatically picked up by GitHub CI, which runs the builds, integration tests, as well as static analysis scan on SonarCloud.

# Documentation

The latest version of the Netatalk manual can be found at:

https://netatalk.io/stable/htmldocs/

Each Netatalk program and configuration file also has a *NIX man page which can be accessed on the command line, f.e. `man afpd`.

# Wiki

Collaborative articles can be found on the Netatalk wiki hosted on GitHub.

Editing is open to all registered GitHub users. Please feel free to edit existing and create new articles.

https://github.com/Netatalk/netatalk/wiki

# Mailing Lists

Netatalk contributors and maintainers interact with each other primarily on the netatalk-admins and netatalk-devel mailing list.

Subscribe and participate, or read archived discussion threads at:

https://sourceforge.net/p/netatalk/mailman/

# Website

The Netatalk website is where project updates and resources are published.

https://netatalk.io
