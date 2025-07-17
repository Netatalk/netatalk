# Introduction

## Overview of Netatalk

Netatalk is an Open Source software package that can be used to turn a
\*NIX machine into a performant and light-weight file server for
Macintosh computers. It provides a TCP/IP as well as a traditional
AppleTalk transport layer.

Using Netatalk's AFP 3.4 compliant file-server leads to noticeably
higher transmission speeds for macOS clients compared to Samba/NFS,
while providing users with the best possible Mac-like user experience.
It can read and write Mac metadata - macOS extended file attributes as
well as Classic Mac OS resource forks - facilitating mixed environments
of new and old Macs.

Netatalk ships with range of capabilities to accommodate most deployment
environments, including Kerberos, ACLs and LDAP. Modern macOS features
such as Bonjour service discovery, Time Machine backups, and Spotlight
indexed search are provided.

For AppleTalk networks with legacy Macs and Apple IIs, Netatalk provides
a print server, time server, and Apple II NetBoot server. The print
server is fully integrated with CUPS, allowing old Macs to discover and
print to a modern CUPS/AirPrint compatible printer on the network.

Additionally, Netatalk can be configured as an AppleTalk router through
its AppleTalk Network Management daemon, providing both segmentation and
zone names in traditional Macintosh networks.

## Legal Notices

The Netatalk software package as well as this documentation are
distributed under the [GNU General Public License version 2](License.html).
A copy of the license is included with this documentation,
as well as within the Netatalk source distribution.

Legal notices for individual modules bundled with the Netatalk distribution follow below.

### netatalk

Copyright (c) 1990,1996 Regents of The University of Michigan.
All Rights Reserved.

    Permission to use, copy, modify, and distribute this software and
    its documentation for any purpose and without fee is hereby granted,
    provided that the above copyright notice appears in all copies and
    that both that copyright notice and this permission notice appear
    in supporting documentation, and that the name of The University
    of Michigan not be used in advertising or publicity pertaining to
    distribution of the software without specific, written prior
    permission. This software is supplied as is without expressed or
    implied warranties of any kind.

    This product includes software developed by the University of
    California, Berkeley and its contributors.

    Research Systems Unix Group
    The University of Michigan
    c/o Wesley Craig
    535 W. William Street
    Ann Arbor, Michigan
    +1-313-764-2278
    netatalk@umich.edu

Modifications for Appleshare IP and other files copyrighted by Adrian
Sun are under the following copyright:

    Copyright (c) 1997,1998,1999,2000 Adrian Sun (asun@cobalt.com)
    All Rights Reserved.

    Permission to use, copy, modify, and distribute this software and
    its documentation for any purpose and without fee is hereby granted,
    provided that the above copyright notice appears in all copies and
    that both that copyright notice and this permission notice appear
    in supporting documentation. This software is supplied as is
    without expressed or implied warranties of any kind.

### a2boot

    Recoding circa Dec 2002-early 2003          by Marsha Jackson
    support booting of Apple 2 computers,   with aid of Steven N. Hirsch
    Code is copyrighted as listed below...
    M. Jackson is establishing no personal copyright
    or personal restrictions on this software.  Only the below rights exist

Copyright (c) 1990,1994 Regents of The University of Michigan.  
All Rights Reserved.

    Permission to use, copy, modify, and distribute this software and
    its documentation for any purpose and without fee is hereby granted,
    provided that the above copyright notice appears in all copies and
    that both that copyright notice and this permission notice appear
    in supporting documentation, and that the name of The University
    of Michigan not be used in advertising or publicity pertaining to
    distribution of the software without specific, written prior
    permission. This software is supplied as is without expressed or
    implied warranties of any kind.

    Research Systems Unix Group
    The University of Michigan
    c/o Wesley Craig
    535 W. William Street
    Ann Arbor, Michigan
    +1 313 764 2278
    netatalk@umich.edu

The "timelord protocol" was reverse engineered from Timelord,
distributed with CAP, Copyright (c) 1990, The University of
Melbourne.  The following copyright, supplied by The University
of Melbourne, may apply to this code:

    This version of timelord.c is based on code distributed
    by the University of Melbourne as part of the CAP package.

    The tardis/Timelord package for Macintosh/CAP is
    Copyright (c) 1990, The University of Melbourne.

### macipgw

AppleTalk MacIP Gateway

(c) 2013, 1997 Stefan Bethke. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

Copyright (c) 1990,1996 Regents of The University of Michigan.  
All Rights Reserved.

    Permission to use, copy, modify, and distribute this software and
    its documentation for any purpose and without fee is hereby granted,
    provided that the above copyright notice appears in all copies and
    that both that copyright notice and this permission notice appear
    in supporting documentation, and that the name of The University
    of Michigan not be used in advertising or publicity pertaining to
    distribution of the software without specific, written prior
    permission. This software is supplied as is without expressed or
    implied warranties of any kind.

Copyright (c) 1988, 1992, 1993  
The Regents of the University of California.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    4. Neither the name of the University nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.

### timelord

Copyright (c) 1990,1994 Regents of The University of Michigan.  
All Rights Reserved.

    Permission to use, copy, modify, and distribute this software and
    its documentation for any purpose and without fee is hereby granted,
    provided that the above copyright notice appears in all copies and
    that both that copyright notice and this permission notice appear
    in supporting documentation, and that the name of The University
    of Michigan not be used in advertising or publicity pertaining to
    distribution of the software without specific, written prior
    permission. This software is supplied as is without expressed or
    implied warranties of any kind.

    Research Systems Unix Group
    The University of Michigan
    c/o Wesley Craig
    535 W. William Street
    Ann Arbor, Michigan
    +1 313 764 2278
    netatalk@umich.edu

The "timelord protocol" was reverse engineered from Timelord,
distributed with CAP, Copyright (c) 1990, The University of
Melbourne.  The following copyright, supplied by The University
of Melbourne, may apply to this code:

    This version of timelord.c is based on code distributed
    by the University of Melbourne as part of the CAP package.

    The tardis/Timelord package for Macintosh/CAP is
    Copyright (c) 1990, The University of Melbourne.

### testsuite

The Netatalk Test-Suite  
Copyright (C) 2003 Rafal Lewczuk <rlewczuk@pronet.pl>  
Copyright (C) 2003-2010 Didier Gautheron <dgautheron@magic.fr>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

### webmin module

images (authentic-theme)

MIT License

Copyright (c) Ilia Rostovtsev

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
