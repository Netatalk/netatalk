# About Netatalk

[![Build Status](https://github.com/Netatalk/netatalk/actions/workflows/build.yml/badge.svg)](https://github.com/Netatalk/netatalk/actions/workflows/build.yml)
[![Project releases](https://img.shields.io/github/release/Netatalk/netatalk)](https://github.com/Netatalk/netatalk/releases)
[![Project contributors](https://img.shields.io/github/contributors/Netatalk/netatalk)](https://github.com/Netatalk/netatalk/graphs/contributors)
[![License: GPL v2](https://img.shields.io/github/license/Netatalk/netatalk)](https://github.com/Netatalk/netatalk/blob/main/COPYING)
[<img src="https://sonarcloud.io/images/project_badges/sonarcloud-orange.svg" height="20" />](https://sonarcloud.io/summary/overall?id=Netatalk_netatalk)

Netatalk is a Free and Open Source file server that implements the [Apple Filing Protocol](https://en.wikipedia.org/wiki/Apple_Filing_Protocol) (AFP) 3.4 over TCP/IP and AppleTalk.
AFP is the primary file sharing protocol used on Apple II, Classic Mac OS, and Mac OS X, as well as one of several supported protocols on macOS.
A *NIX/*BSD system running Netatalk provides high-speed AppleShare file sharing for macOS and Classic Mac OS clients.

# AppleTalk

Netatalk provides AFP over the [AppleTalk](https://en.wikipedia.org/wiki/AppleTalk) family of protocols in order to provide file sharing and other services with very old Macs or Apple II computers.

All versions of Netatalk except the 3.x release series support AppleTalk.

# Community

Netatalk developers and users hang out in online communal spaces like [TinkerDifferent](https://tinkerdifferent.com/) or [68kmla](https://68kmla.org/).
You can head over there if you want to ask for help or share your experiences.

There is also an official [Netatalk Discussions](https://github.com/Netatalk/netatalk/discussions) board on GitHub which is the best place to ask for technical assistance.

Finally, the traditional place where Netatalk developers and users interact with each other are the [netatalk-admins](https://sourceforge.net/p/netatalk/mailman/netatalk-admins/) and [netatalk-devel](https://sourceforge.net/p/netatalk/mailman/netatalk-devel/) mailing lists.
While these lists aren't as active as they used to be, the archives are a veritable treasure trove of decades of Mac networking know-how.

# Why Should I Use Netatalk?

If you have a local network macOS, Mac OS X, Classic Mac OS, or even Apple II computers,
running a Netatalk AppleShare server allows you to share, collaborate on, and back up files comfortably.
The latest macOS at the time of writing (macOS 14 Sonoma) ships with an AFP client,
so Netatalk can act as a seamless bridge between new and old Macs.

Compared to cross-platform file sharing protocols NFS and FTP, Netatalk delivers full integration
of Classic Mac OS metadata (resource forks) and macOS services such as Bonjour, Time Machine, and Spotlight.

Compared to [Samba](https://www.samba.org/), Netatalk has [demonstrably faster transfer speeds](https://github.com/Netatalk/netatalk/wiki/Benchmarks), as well as stronger backwards compatibility with OS X 10.8 Mountain Lion and earlier.

# Installation

See [INSTALL.md](https://github.com/Netatalk/netatalk/blob/main/INSTALL.md)

# Docker

See [DOCKER.md](https://github.com/Netatalk/netatalk/blob/main/DOCKER.md)

# Webmin module

An administrative GUI webapp in the form of a first-party module for Webmin can be found in a [sister repository](https://github.com/Netatalk/netatalk-webmin)

See the [README](https://github.com/Netatalk/netatalk-webmin/blob/main/README.md) in that repo for instructions how to install and get started with the module.

An [overview of the module's feature set](https://github.com/Netatalk/netatalk/wiki/Webmin-Module) can be found in the wiki.

# Contributions

Bug reports and feature requests should be filed as [GitHub issue tickets](https://github.com/Netatalk/netatalk/issues).

Before contributing code to the project, please read the [coding guidelines](https://github.com/Netatalk/netatalk/wiki/Developer-Notes) in the wiki on how to prepare a PR that is likely to be accepted by the maintainers.

PRs are automatically picked up by GitHub CI, which runs the builds, integration tests, as well as static analysis scan on SonarCloud.

# Documentation

To aid in your installation and configuration of Netatalk, a comprehensive [html manual](https://netatalk.io/stable/htmldocs/) is published online.

Additionally, each Netatalk program and configuration file also has a _troff_ man page which can be accessed on the command line, f.e. `man afpd`.

# Wiki

Collaborative articles can be found on the [Netatalk wiki](https://github.com/Netatalk/netatalk/wiki) hosted on GitHub.

Editing is open to all registered GitHub users.
We welcome improvements to existing material, as well as brand new articles on topics concerning Netatalk or Mac networking in general.

# Website

The Netatalk website [netatalk.io](https://netatalk.io) is where project updates and resources are published, including documentation, release notes, security advisories, links to related projects, and more.
