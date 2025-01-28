# Name

addump - Dump AppleSingle/AppleDouble format data

# Synopsis

`addump [-a] [ FILE | DIR ]`

`addump [-e] [ FILE | DIR ]`

`addump [-f] [FILE]`

`addump [-d] [FILE]`

`addump [ -h | -help | --help ]`

`addump [ -v | -version | --version ]`

# Description

**addump** dumps AppleSingle/AppleDouble format data.

It can dump various AppleSingle/AppleDouble data created by mailers,
archivers, Mac OS X, Netatalk and so on.

With no <FILE\>|<DIR\>, or when <FILE\>|<DIR\> is '-', read standard
input.

# Options

**-a** \[<FILE\>|<DIR\>\]

> This is the default. Dump AppleSingle/AppleDouble data for <FILE\> or
<DIR\> automatically. If FILE is not in AppleSingle/AppleDouble format,
look for extended attributes, <.AppleDouble/FILE\> and <.\_FILE\>. If
<DIR\>, look for extended attributes, <DIR/.AppleDouble/.Parent\> and
<.\_DIR\>.

**-e** <FILE\>|<DIR\>

> Dump extended attributes of <FILE\> or <DIR\>.

**-f** \[<FILE\>\]

> Dump <FILE\>. Assume FinderInfo to be FileInfo.

**-d** \[<FILE\>\]

> Dump <FILE\>. Assume FinderInfo to be DirInfo.

**-h**, **-help**, **--help**

> Display the help and exit

**-v**, **-version**, **--version**

> Show version and exit

# Note

There is no way to detect whether FinderInfo is FileInfo or DirInfo. By
default, addump examines whether the file, directory or parent directory
is .AppleDouble, filename is .\_\*, filename is .Parent, and so on.

When setting option **-e**, **-f** or **-d**, assume FinderInfo and don't look for
other data.

# See Also

ad(1), getfattr(1), attr(1), runat(1), getextattr(8),
lsextattr(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
