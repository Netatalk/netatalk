Netatalk for FreeBSD 2.1.5-RELEASE
==================================

These files provide support for netatalk 1.4 on FreeBSD 2.1.5-RELEASE.

netatalk support is integrated into FreeBSD 2.2 and subsequent versions;
however, to use it on FreeBSD 2.1.5 or 2.1.6 you need kernel support for
AppleTalk.

netatalk 1.4 afpd makes use of quota information; I have decided to remove
it for the lack of time to make it work.

For further information on netatalk see http://www.umich.edu/~rsug/netatalk.
For further information on FreeBSD see http://www.freebsd.org/.


What do you need?
-----------------

Besides these files you will need netatalk 1.4b1 or newer to use AppleTalk
services on your system.


Installation
------------
1. Un-tar the netatalk sources into your kernel source tree, i. e.

	tar xf sys-netatalk.tar -C /sys

2. Apply the patches to the kernel:

	cd /sys
	patch -p1 < [this directory]/diff-FreeBSD-215-ssys-netatalk

3. Apply the netatalk patches:

	cd [netatalk distribution]
	patch -p1 < [this directory]/diff-netatalk-14b2-FreeBSD

   This will add the option not to use quota information to afpd.

4. Build a kernel with AppleTalk.  Add a line 'option NETATALK' to your
   favorite kernel configuration file and build the kernel.

5. Build netatalk.  Simply say 'make' at the top of the netatalk
   distribution. You might want to change the directories into which
   netatalk will be installed; see the file README in the distribution 
   for details.

6. 'make install' will install netatalk; consult netatalks documentation
   for further options.


7. Have fun!


12/07/96 Stefan Bethke
