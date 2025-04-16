# Name

ad - AppleDouble file utility suite

# Synopsis

**ad** [ ls | cp | mv | rm | set ] [...]

**ad** [ -v | --version ]

# Description

**ad** is an AppleDouble file utility suite with Netatalk compatibility.
AppleDouble data (extended attributes of files, .\_files in same
directories or files in **.AppleDouble** directories) and the CNID
databases are updated as appropriate when files in a shared Netatalk
volume are modified.

# Available Commands

List files and directories.

> ad ls [-dRl[u]] {file|dir [...]}

Copy files and directories.

> ad cp [-aipvf] {src_file} {dst_file}

> ad cp -R [-aipvf] {src_file|src_directory ...} {dst_directory}

Move files and directories.

> ad mv [-finv] {src_file} {dst_file}

> ad mv [-finv] {src_file|src_directory ...} {dst_directory}

Remove files and directories.

> ad rm [-Rv] {file|directory}

Set metadata on files.

> ad set [-t type] [-c creator] [-l label] [-f flags] [-a attributes] {file}

Show version.

> ad -v | --version

# ad ls

List files and directories. Options:

**-d**

> Directories are listed as plain files

**-R**

> list subdirectories recursively

**-l**

> Long output, list AFP info

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

# ad cp

Copy files and directories.

In the first synopsis form, the cp utility copies the contents of the
src_file to the dst_file. In the second synopsis form, the contents of
each named src_file is copied to the destination dst_directory. The
names of the files themselves are not changed. If cp detects an attempt
to copy a file to itself, the copy will fail.

When a copy targeting an AFP volume is detected, its CNID database
daemon is connected and all copies will also go through the CNID
database. AppleDouble data are also copied and created as needed when
the target is an AFP volume.

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

# ad mv

Move files and directories.

Move files around within an AFP volume, updating the CNID database as
needed. If either condition below is true, the files are copied and removed from the source.

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

# ad rm

Remove files and directories.

The rm utility attempts to remove the non-directory type files specified
on the command line. If the files and directories reside on an AFP
volume, the corresponding CNIDs are deleted from the volumes database.

The options are as follows:

**-R**

> Attempt to remove the file hierarchy rooted in each file argument.

**-v**

> Be verbose when deleting files, showing them as they are removed.

# ad set

Set metadata on files.

The set utility alters metadata on files within an AFP volume.

The options are as follows:

**-t** <type\>

> Change a file's four character file type.

**-c** <creator\>

> Change a file's four character creator type.

**-l** <label\>

> Change a file's color label. See list below for available colors.

**-f** <flags\>

> Change a file's Finder flags. See list below for available flags.
Uppercase letter sets the flag, lowercase removes the flag.

**-a** <attributes\>

> Change a file's attributes. See list below for available attributes.
Uppercase letter sets the flag, lowercase removes the flag.

*Flag descriptions*

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

# See also

dbd(1), addump(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
