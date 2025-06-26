# Netatalk - Free and Open Source AFP File Server

Continuous Integration:
[![Build Status](https://github.com/Netatalk/netatalk/actions/workflows/build.yml/badge.svg)](https://github.com/Netatalk/netatalk/actions/workflows/build.yml)
[![Build Status](https://github.com/Netatalk/netatalk/actions/workflows/test.yml/badge.svg)](https://github.com/Netatalk/netatalk/actions/workflows/test.yml)
[![Container Status](https://github.com/Netatalk/netatalk/actions/workflows/containers.yml/badge.svg)](https://github.com/Netatalk/netatalk/actions/workflows/containers.yml)

Get Netatalk:
[![Project releases](https://img.shields.io/github/release/Netatalk/netatalk)](https://github.com/Netatalk/netatalk/releases)
[![Downloads](https://img.shields.io/github/downloads/Netatalk/netatalk/total)](https://github.com/Netatalk/netatalk/releases)
[![Packaging status](https://repology.org/badge/tiny-repos/netatalk.svg)](https://repology.org/project/netatalk/versions)

Project Activity:
[![Project contributors](https://img.shields.io/github/contributors/Netatalk/netatalk)](https://github.com/Netatalk/netatalk/graphs/contributors)
[![Monthly commits](https://img.shields.io/github/commit-activity/m/Netatalk/netatalk)](https://github.com/Netatalk/netatalk/graphs/commit-activity)
[![Lines of code](https://sonarcloud.io/api/project_badges/measure?project=Netatalk_netatalk&metric=ncloc)](https://sonarcloud.io/project/overview?id=Netatalk_netatalk)

Code Quality:
[![Security](https://sonarcloud.io/api/project_badges/measure?project=Netatalk_netatalk&metric=security_rating)](https://sonarcloud.io/project/overview?id=Netatalk_netatalk)
[![Reliability](https://sonarcloud.io/api/project_badges/measure?project=Netatalk_netatalk&metric=reliability_rating)](https://sonarcloud.io/project/overview?id=Netatalk_netatalk)
[![Maintainability](https://sonarcloud.io/api/project_badges/measure?project=Netatalk_netatalk&metric=sqale_rating)](https://sonarcloud.io/project/overview?id=Netatalk_netatalk)
[![Maintainability](https://sonarcloud.io/api/project_badges/measure?project=Netatalk_netatalk&metric=duplicated_lines_density)](https://sonarcloud.io/project/overview?id=Netatalk_netatalk)

Supply Chain Security:
[![OpenSSF Scorecard](https://img.shields.io/ossf-scorecard/github.com/Netatalk/netatalk?label=openssf+scorecard&style=flat)](https://scorecard.dev/viewer/?uri=github.com/Netatalk/netatalk)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/10806/badge)](https://www.bestpractices.dev/projects/10806)

## About Netatalk

Netatalk is a Free and Open Source file server that implements
the [Apple Filing Protocol](https://en.wikipedia.org/wiki/Apple_Filing_Protocol)
(AFP) 3.4 over TCP/IP and AppleTalk.
AFP is the native file sharing protocol used on Apple II, Classic Mac OS, and early Mac OS X,
as well as one of several supported protocols built into macOS.
A *NIX/*BSD system running Netatalk provides high-speed AppleShare file sharing for Mac clients.

## Why Should I Use Netatalk?

If you have a local network of macOS, Mac OS X, Classic Mac OS, or even Apple II computers,
running a Netatalk AFP server allows you to share, collaborate on, and back up files remotely.
The latest macOS at the time of writing (macOS 15 Sequoia) comes with a built-in AFP client,
so Netatalk can act as a seamless bridge between new and old Macs.

Compared to cross-platform file sharing protocols like NFS and FTP, Netatalk delivers a Mac-like user experience,
with seamless integration of Mac filesystem metadata - including Extended Attributes on macOSand resource forks
on Classic Mac OS - and compatibility with modern macOS features such as Bonjour, Time Machine, and Spotlight.

Compared to [Samba](https://www.samba.org/), Netatalk has [demonstrably faster transfer speeds](https://netatalk.io/docs/Benchmarks),
as well as stronger backwards compatibility with OS X 10.8 Mountain Lion clients and earlier.

## AppleTalk

Netatalk supports the [AppleTalk](https://en.wikipedia.org/wiki/AppleTalk) family of protocols,
including the DDP transport layer for AFP file sharing with very old Macs or Apple II computers.

In addition, a print server, time server, MacIP gateway bridge, and Apple II netboot server
are also provided.

All versions of Netatalk except the 3.x release series can speak AppleTalk.

## Website

The Netatalk website [netatalk.io](https://netatalk.io) is where project updates and resources are published,
including documentation, release notes, security advisories, links to related projects, and more.

## Community

Netatalk developers and users can be found in online communal spaces like [TinkerDifferent](https://tinkerdifferent.com/)
or [68kmla](https://68kmla.org/).
You can head over there if you want to ask for help or share your Netatalk stories.

There is also an official [Netatalk Discussions](https://github.com/Netatalk/netatalk/discussions) board
on GitHub which is the best place to ask for technical assistance.

Finally, the traditional place where Netatalk developers and users interact with each other
are the [netatalk-admins](https://sourceforge.net/p/netatalk/mailman/netatalk-admins/)
and [netatalk-devel](https://sourceforge.net/p/netatalk/mailman/netatalk-devel/) mailing lists.
While these lists aren't as active as they used to be,
the archives are a veritable treasure trove of decades of Mac networking know-how.

## Installation

Most OS distributions and package repositories ship a version of Netatalk.
If you want a pre-built binary package, try your package manager first.

To get started with building Netatalk from source code, the [install readme](https://netatalk.io/install)
is a good starting point.

## Container deployments

Netatalk runs well in a containerized environment.

Read the [container readme](https://netatalk.io/docker) for more information.

## Webmin module

An administrative GUI frontend built on the Webmin 2.0 platform is distributed with Netatalk.

See the [Webmin README](https://github.com/Netatalk/netatalk/blob/main/contrib/webmin_module/README.md)
for instructions how to install and get started with the module.
or this [overview of the module's feature set](https://netatalk.io/docs/Webmin-Module).

## Contributions

Bug reports and feature requests should be filed as [GitHub issue tickets](https://github.com/Netatalk/netatalk/issues).

Before contributing code to the project, please read the [Developer Notes](https://netatalk.io/docs/Developer-Notes)
to learn how to prepare a PR that is compliant with project guidelines.

PRs are automatically picked up by GitHub CI, which runs the builds, integration tests,
as well as static analysis scan on SonarCloud (the latter only for PRs created by project members.)

## Security

See the [Security Policy](https://netatalk.io/security) for a breakdown of Netatalk version supported with security patches,
our security vulnerability reporting guidelines, as well as the full record of historical security advisories.

We would love to hear from you if you think you found a security vulnerability in Netatalk.
Please read the above policy and then file a
[security issue ticket](https://github.com/Netatalk/netatalk/security/advisories/new) with us.
We promise to follow up with you as soon as possible.

## Documentation

To aid in your installation and configuration of Netatalk, a comprehensive [manual](https://netatalk.io/manual/en/)
is published on the project website.

Additionally, each Netatalk program and configuration file also has a _roff_ man page
which can be accessed on the command line, f.e. `man afpd`.

## Wiki

Collaborative articles can be found on the [Netatalk wiki](https://github.com/Netatalk/netatalk/wiki).
The [Netatalk website](https://netatalk.io/docs) also serves a static mirror of all wiki pages.

Editing is open to all registered GitHub users.
We are looking forward to your additions to existing material,
or brand new articles on topics concerning Netatalk and Mac networking.

## Hosting

This project is currently hosted on GitHub. This is not ideal;
the GitHub platform itself is a proprietary system that is not Free and Open Souce Software (FOSS).
We are deeply concerned about the long term sustainability
of using a proprietary system like GitHub to develop our FOSS project.

We have an [open ticket on our GitLab mirror](https://gitlab.com/netatalk-team/netatalk/-/issues/1)
for project members and volunteers to discuss hosting options.

If you are a contributor who prefer to not use GitHub, please see the [Developer FAQ](https://netatalk.io/docs/Developer-FAQ)
for instructions on how to create and submit patches without using GitHub directly.

## License and Copyright

The netatalk software package is distributed under the GNU General Public License v2.
However, individual source files or modules may be covered by other open source licenses.
Refer to the headers of each source file, or the copyright/license notice in a module subdir.

Any use of this project's code to train commercial large language models
was done without permission by this project's contributors.
We protest the use of our code in a way that goes against the word and spirit of our open source licenses.
