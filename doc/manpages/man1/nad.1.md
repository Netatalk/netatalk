# Name

nad - Netatalk AppleDouble file utility suite

# Synopsis

**nad** [-F *configfile*] [\-\-force] [ls | cp | mv | rm | mkdir | rmdir | set | find | bin | hex | unbin | unhex] [...]

**nad** [-v | \-\-version]

# Description

**nad** is a file utility suite that can be used on a Netatalk host
to manipulate files and directories, including files in AFP shared volumes.
AppleDouble data, which could be one of extended attributes of files,
.\_ files, or files in **.AppleDouble** directories,
as well as the CNID databases are updated appropriately
when files in the shared Netatalk volume are modified.
By default, **nad** refuses to operate on paths outside configured AFP
volumes. Use **\-\-force** to override this validation.

Using **nad** is preferable over the operating system's native file operation commands
on files and directories in a Netatalk AFP volume, because it preserves the integrity
of Mac OS metadata and accuracy of the CNID database.

Only users with appropriate permissions to access AFP files and directories
can use **nad** to manipulate them.
It is sensitive to afp.conf settings such as *valid users* and *invalid users*.

When used with **\-\-force** outside AFP volumes, **nad** can manipulate files
and directories while making a best effort to preserve Mac OS metadata using
extended attributes or AppleDouble sidecar files, depending on the options
used. CNID database updates are skipped outside AFP volumes.

The **bin**, **hex**, **unbin**, and **unhex** commands transform files
between Netatalk metadata, MacBinary, and BinHex formats. They preserve data
forks, resource forks, and Finder metadata when moving files between classic
Mac archive formats and Netatalk-managed storage.

The file format transformer commands are based on *megatron* by Charles Clark.

# Options

**-F** *configfile*

> Use *configfile* as the full path to the Netatalk configuration file.

**\-\-force**

> Allow operations on paths outside configured AFP volumes. Outside AFP
volumes, CNID database updates are skipped.

# Available Commands

List files and directories.

> nad ls [-adlRu] {file|directory [...]}

Copy files and directories.

> nad cp [-aipvf] {src_file} {dst_file}
>
> nad cp -R [-aipvf] {src_file|src_directory ...} {dst_directory}

Move files and directories.

> nad mv [-finv] {src_file} {dst_file}
>
> nad mv [-finv] {src_file|src_directory ...} {dst_directory}

Remove files and directories.

> nad rm [-Rv] {file|directory}

Create directories.

> nad mkdir [-pv] {directory [...]}

Remove empty directories.

> nad rmdir [-pv] {directory [...]}

Set metadata on files.

> nad set [-t type] [-c creator] [-l label] [-f flags] [-a attributes] {file}

Find files and directories.

> nad find [-v volume_path] {name}

Convert between Netatalk metadata, MacBinary, and BinHex.

> nad bin [\-\-header] [\-\-filename NAME] [\-\-stdout] [\-\-adouble] [\-\-verbose] FILE [...]
>
> nad hex [\-\-header] [\-\-filename NAME] [\-\-stdout] [\-\-adouble] [\-\-verbose] FILE [...]
>
> nad unbin [\-\-header] [\-\-filename NAME] [\-\-adouble] [\-\-verbose] [SOURCEFILE [...]]
>
> nad unhex [\-\-header] [\-\-filename NAME] [\-\-adouble] [\-\-verbose] [SOURCEFILE [...]]

Show version.

> nad -v | \-\-version

# nad ls

List files and directories in an AFP volume.
AppleDouble metadata and CNIDs are displayed in long output mode.

Options:

**-a**

> Include directory entries whose names begin with a dot (.)

**-d**

> Directories are listed as plain files

**-l**

> Long output, list AFP info

**-R**

> List subdirectories recursively

**-u**

> List UNIX info

*Long output description*

    <unixinfo> <FinderFlags> <AFP Attributes> <Color> <Type> <Creator> <CNID from AppleDouble> <name>

    FinderFlags (valid for (f)iles and/or (d)irectories):

      d = On Desktop                      (f/d)
      e = Hidden extension                (f/d)
      m = Shared (can run multiple times) (f)
      n = No INIT resources               (f)
      i = Inited                          (f/d)
      c = Custom icon                     (f/d)
      t = Stationery                      (f)
      s = Name locked                     (f/d)
      b = Bundle                          (f/d)
      v = Invisible                       (f/d)
      a = Alias file                      (f/d)

    AFP Attributes:

      y = System                          (f/d)
      w = No write                        (f)
      p = Needs backup                    (f/d)
      r = No rename                       (f/d)
      l = No delete                       (f/d)
      o = No copy                         (f)

    Note: any letter appearing in uppercase means the flag is set but it's a directory for which the flag is not allowed.

# nad cp

Copy files and directories in an AFP volume.

In the first synopsis form, the cp utility copies the contents of the
src_file to the dst_file. In the second synopsis form, the contents of
each named src_file is copied to the destination dst_directory. The
names of the files themselves are not changed. If cp detects an attempt
to copy a file to itself, the copy will fail.

Source and destination paths must be inside configured AFP volumes unless
**\-\-force** is used. When a copy targeting an AFP volume is detected, its
CNID database daemon is connected and all copies will also go through the
CNID database. AppleDouble data are also copied and created as needed when the
target is an AFP volume.

Options:

**-a**

> Archive mode. Same as **-Rp**.

**-f**

> For each existing destination pathname, remove it and create a new file,
without prompting for confirmation regardless of its permissions. (The
**-f** option overrides any previous **-i** or **-n** options.)

**-i**

> Cause cp to write a prompt to the standard error output before copying a
file that would overwrite an existing file. If the response from the
standard input begins with the character 'y' or 'Y', the file copy is
attempted. (The **-i** option overrides any previous **-f** or **-n** options.)

**-n**

> Do not overwrite an existing file. (The **-n** option overrides any previous
**-f** or **-i** options.)

**-p**

> Cause cp to preserve the following attributes of each source file in the
copy: modification time, access time, file flags, file mode, user ID,
and group ID, as allowed by permissions. If the user ID and group ID
cannot be preserved, no error message is displayed and the exit value is
not altered.

**-R**

> If src_file designates a directory, cp copies the directory and the
entire subtree connected at that point. If the src_file ends in a /, the
contents of the directory are copied rather than the directory itself.

**-v**

> Cause cp to be verbose, showing files as they are copied.

**-x**

> File system mount points are not traversed.

# nad mv

Move files and directories.

Move files around within an AFP volume, updating the CNID database as
needed. Source and destination paths must be inside configured AFP volumes
unless **\-\-force** is used. If either condition below is true, the files are
copied and removed from the source.

- source or destination is not an AFP volume

- source AFP volume != destination AFP volume

Options:

**-f**

> Do not prompt for confirmation before overwriting the destination path.
(The **-f** option overrides any previous **-i** or **-n** options.)

**-i**

> Cause mv to write a prompt to standard error before moving a file that
would overwrite an existing file. If the response from the standard
input begins with the character \`y' or \`Y', the move is attempted.
(The **-i** option overrides any previous **-f** or **-n** options.)

**-n**

> Do not overwrite an existing file. (The **-n** option overrides any previous
**-f** or **-i** options.)

**-v**

> Cause mv to be verbose, showing files after they are moved.

# nad rm

Remove files and directories in an AFP volume.

The rm utility attempts to remove the non-directory type files specified
on the command line.
The corresponding CNIDs are deleted from the volume's database,
and AppleDouble metadata is removed.

The options are as follows:

**-R**

> Attempt to remove the file hierarchy rooted in each file argument.

**-v**

> Be verbose when deleting files, showing them as they are removed.

# nad mkdir

Create directories in an AFP volume.
The corresponding CNIDs are added to the volume's database,
and AppleDouble metadata is initialized.

The options are as follows:

**-p**

> Create intermediate directories as required.
If an intermediate or the target directory already exists, it is not
considered an error.

**-v**

> Be verbose when creating directories, showing them as they are created.

# nad rmdir

Remove empty directories in an AFP volume.
The corresponding CNIDs are deleted from the volume's database,
and AppleDouble metadata is removed.

The volume root directory and its parent directories cannot be removed.

The options are as follows:

**-p**

> Each operand is treated as a pathname of which all components will be
removed, if they are empty, starting with the last most component.
Will not remove the volume root directory or its parents.

**-v**

> Be verbose when removing directories, showing them as they are removed.

# nad set

Set metadata on files.

The set utility alters metadata on files within an AFP volume.

The options are as follows:

**-t** *type*

> Change a file's four character file type.

**-c** *creator*

> Change a file's four character creator type.

**-l** *label*

> Change a file's color label. See list below for available colors.

**-f** *flags*

> Change a file's Finder flags. See list below for available flags.
Uppercase letter sets the flag, lowercase removes the flag.

**-a** *attributes*

> Change a file's attributes. See list below for available attributes.
Uppercase letter sets the flag, lowercase removes the flag.

## Flag descriptions

    Color Labels:

      none | grey | green | violet | blue | yellow | red | orange

    Finder Flags (valid for (f)iles and/or (d)irectories):

      d = On Desktop                      (f/d)
      e = Hidden extension                (f/d)
      m = Shared (can run multiple times) (f)
      n = No INIT resources               (f)
      i = Inited                          (f/d)
      c = Custom icon                     (f/d)
      t = Stationery                      (f)
      s = Name locked                     (f/d)
      b = Bundle                          (f/d)
      v = Invisible                       (f/d)
      a = Alias file                      (f/d)

    AFP Attributes:

      y = System                          (f/d)
      w = No write                        (f)
      p = Needs backup                    (f/d)
      r = No rename                       (f/d)
      l = No delete                       (f/d)
      o = No copy                         (f)

# nad find

Find files and directories.

This returns a list of paths that wholly or partially match the given name.
Inside AFP volumes, the CNID database is searched. With **\-\-force** outside
AFP volumes, the filesystem is traversed from the selected path.

It takes one option:

**-v** *path*

> Use path to the shared volume to search rather than the current working
directory. With **\-\-force** outside AFP volumes, use path as the filesystem
traversal root.

# nad bin, hex, unbin, unhex

Encode and decode MacBinary and BinHex, while reading and storing Mac OS
metadata (resource forks and FinderInfo) in the metadata format used by
Netatalk. Inside AFP volumes, the configured Netatalk volume metadata format
is used and CNID metadata is updated.

The command reads *afp.conf*, resolves each relevant path to a configured
AFP volume, and uses that volume's metadata format when the path is inside
an AFP volume. AppleDouble v2 volumes (**ea = ad**) use AppleDouble
sidecar files. Extended Attributes volumes (**ea = sys**) use Netatalk's
EA/sys-xattr metadata path.

When operating with **\-\-force** outside an AFP volume, **nad** uses
filesystem EA metadata by default and silently skips CNID database updates.
Use **\-\-adouble** to use AppleDouble sidecar metadata outside AFP volumes
instead.

The filename used for an output file is the filename encoded in the
source file, unless **\-\-filename** is supplied. MacBinary output files are
created with a *.bin* extension. BinHex output files are created with a
*.hqx* extension.

Commands:

**bin**

> Convert Netatalk metadata, data fork, and resource fork to MacBinary
(*.bin*).

**hex**

> Convert Netatalk metadata, data fork, and resource fork to BinHex
(*.hqx*).

**unbin**

> Convert MacBinary (*.bin*) input to the current AFP volume's metadata
format, or filesystem EA metadata outside an AFP volume.

**unhex**

> Convert BinHex (*.hqx*) input to the current AFP volume's metadata
format, or filesystem EA metadata outside an AFP volume.

Options:

**\-\-header**

> Print header information for the selected input format instead of performing
a conversion. For **bin** and **hex**, this prints Netatalk metadata. For
**unbin** and **unhex**, this prints the archive header.

**\-\-filename** *NAME*

> Use *NAME* in the converted file header instead of the name encoded in the
source file. An appropriate file extension is still added to the output file.

**\-\-stdout**

> Write generated MacBinary or BinHex data to standard output.

**\-\-adouble**

> Write AppleDouble sidecar metadata instead of filesystem EA metadata outside
AFP volumes. Configured AFP volumes continue to use the metadata format from
*afp.conf*.

**\-\-verbose**

> Print diagnostic conversion details. Without this option, normal conversion
is silent unless an error occurs.

**-h**, **\-\-help**

> Display archive command help and exit.

If no *SOURCEFILE* is given, or if *SOURCEFILE* is '-', **unbin** and
**unhex** read archive data from standard input. Options must appear before
positional *FILE* arguments.

# Examples

List files in a shared AFP volume:

    $ nad ls -al /srv/afpshare
    ---Nic-s--- ------ --- ---- ----           2   .
    ----------- ------ --- ---- ----           1   ..
    ----i---b-- ------ --- APPL SBMC          40   AppleShare IP Browser
    ---------v- ------ --- ---- ----          36   Network Trash Folder
    -------s-v- ------ --- ---- ----          35   TheVolumeSettingsFolder
    ----ic-s--- ------ gre PNGf GKON          39   picture 1.png
    ----ic----- ------ --- PNGf GKON          41   picture 2.png
    ----ic----- ------ --- PNGf GKON          42   picture 3.png

Legend:

- column 1: Finder flags
- column 2: AFP attributes
- column 3: color label
- column 4: type
- column 5: creator
- column 6: CNID from AppleDouble
- column 7: file name

Find files with "Report" in the name in the shared AFP volume at /srv/afpshare:

    $ nad find -v /srv/afpshare Report
    /srv/afpshare/Documents/2025/Report January 2025.doc
    /srv/afpshare/Documents/2024/Report December 2024.doc
    /srv/afpshare/Documents/Report Template.doc

Convert a file to MacBinary:

    nad bin /srv/afpshare/picture

Convert a file to BinHex:

    nad hex /srv/afpshare/picture

Convert MacBinary input into filesystem EA metadata outside an AFP volume:

    nad --force unbin /tmp/picture.bin

Convert BinHex input to the current AFP volume's metadata format:

    nad unhex file.hqx

Inspect a MacBinary header:

    $ nad unbin --header MacTCP_Ping_2.0.2.sea_.bin
    name:               MacTCP Ping 2.0.2.sea
    comment:
    creator:            'aust'
    type:               'APPL'
    fork length[0]:     24419
    fork length[1]:     18774
    creation date:      Wed May 17 19:41:57 1995
    modification date:  Wed May 17 19:41:58 1995
    backup date:        (not set)

# See also

dbd(1), addump(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
