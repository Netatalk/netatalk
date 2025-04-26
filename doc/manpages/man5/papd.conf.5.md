# Name

papd.conf — Configuration file used by papd to determine the configuration of printers used by the Netatalk printing daemon

# Description

**papd.conf** is the configuration file used by papd to configure the
printing services offered by netatalk. *papd* shares the same defaults
as lpd on many systems. One notable exception is Solaris.

The format of papd.conf is derived from **printcap**(5) and can contain
configurations for one or more printers. Any line not prefixed with *\#*
is interpreted. The configuration lines are composed like this:

*printername:[options]*

On systems running a System V printing system, the simplest case is to
have either no papd.conf, or to have one that has no active lines. In
this case, **atalkd** should auto-discover the local printers on the
machine. Please note that you can split lines with a *\\* (backslash).

printername may be just a name (*Printer 1*), or it may be a full name
in nbp_name format (*Printer 1:LaserWriter@My Zone*).

Systems using a BSD printing system should make use of a pipe to the
printing command in question within the *pr* option (e.g.
*pr=|/usr/bin/lpr*).

When CUPS support is compiled in, then *cupsautoadd* as the first entry
in papd.conf will automagically configure and make all CUPS printers
available to papd (customizable -- see below). This can be overwritten
for individual printers by subsequently adding individual entries using
the CUPS queue name as *pr* entry. Note: CUPS support is mutually
exclusive with System V support described above.

The possible options are colon delimited (*:*), and lines must be
terminated with colons. The available options and flags are:

**am=(uams list)**

> The **am** option allows specific UAMs to be specified for a particular
printer. It has no effect if the **au** flag is not present. Note:
possible values are *uams_guest.so* and *uams_clrtxt.so* only. The first
method requires a valid username, but no password. The second requires
both a valid username and the correct password.

**au**

> If present, this flag enables authentication for the printer.

**co=(CUPS options)**

> The *co* option allows options to be passed through to CUPS (e.g.
*co="protocol=TBCP"* or *co="raw"*).

**cupsautoadd[:type][@zone]**

> If used as the first entry in papd.conf this will share all CUPS
printers via papd. type/zone settings as well as other parameters
assigned to this special shared printer will apply to all CUPS printers.
Unless the *pd* option is set, the CUPS PPDs will be used. To overwrite
these global settings for individual printers simply add them
subsequently to papd.conf and assign different settings.

**fo**

> If present, this flag enables a hack to translate line endings
originating from pre-Mac OS X LaserWriter drivers to let *foomatic-rip*
recognize foomatic PPD options set in the printer dialog. Attention: Use
with caution since this might corrupt binary print jobs!

**op=(operator)**

> This specifies the operator name, for lpd spooling. Default value is
"operator".

**pa=(appletalk address)**

> Allows specification of AppleTalk addresses. Usually not needed.

**pd=(path to ppd file)**

> Specifies a particular PPD (printer description file) to associate with
the selected printer.

**pr=(lpd/CUPS printer name or pipe command)**

> Sets the *lpd* or *CUPS* printer that this is spooled to. Default value
is "lp".

# Examples

Unless CUPS support has been compiled in (which is default from Netatalk
2.0 on) one simply defines the lpd queue in question by setting the **pr**
parameter to the queue name, in the following example "ps". If no **pr**
parameter is set, the default printer will be used.

## Example: papd.conf System V printing system examples

The first spooler is known by the AppleTalk name Mac Printer Spooler,
and uses a PPD file located in */usr/share/lib/ppd*. In addition, the
user mcs will be the owner of all jobs that are spooled. The second
spooler is known as HP Printer and all options are the default.

    Mac Printer Spooler:\
       :pr=ps:\
       :pd=/usr/share/lib/ppd/HPLJ_4M.PPD:\
       :op=mcs:

    HP Printer:\
       :

An alternative to the technique outlined above is to direct papd's
output via a pipe into another program. Almost any printing system can
be driven using this mechanism.

## Example: papd.conf examples using pipes

The first spooler is known as HP 8100. It pipes the print job to
*/usr/bin/lpr* for printing. PSSP authenticated printing is enabled, as
is CAP-style authenticated printing. Both methods support guest and
cleartext authentication as specified by the '**am**' option. The PPD used
is */etc/atalk/ppds/hp8100.ppd*.

    HP 8100:\
       :pr=|/usr/bin/lpr -Plp:\
       :sp:\
       :ca=/tmp/print:\
       :am=uams_guest.so,uams_clrtxt.so:\
       :pd=/etc/atalk/ppds/hp8100.ppd:

Starting with Netatalk 2.0, direct CUPS integration is available. In
this case, defining only a queue name as **pr** parameter won't invoke the
SysV lpd daemon but uses CUPS instead. Unless a specific PPD has been
assigned using the **pd** switch, the PPD configured in CUPS will be used
by **papd**, too.

There exists one special share named "cupsautoadd". If this is present
as the first entry then all available CUPS queues will be served
automagically using the parameters assigned to this global share. But
subsequent printer definitions can be used to override these global
settings for individual spoolers.

## Example: papd.conf CUPS examples

The first entry sets up automatic sharing of all CUPS printers. All
those shares appear in the zone "1st floor" and since no additional
settings have been made, they use the CUPS printer name as NBP name and
use the PPD configured in CUPS. The second entry defines different
settings for one single CUPS printer. Its NBP name is differing from the
printer's name and the registration happens in another zone.

    cupsautoadd@1st floor:op=root:

    Boss' LaserWriter@2nd floor:\
       :pr=laserwriter-chief:

# Caveats

If you are using more than 15 printers in your network, you must specify
AppleTalk zones for the papd printer configurations. Otherwise, only
some of the printers may appear in the Chooser on Mac clients.

# See Also

papd(8), atalkd.conf(5), lpd(8), lpoptions(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
