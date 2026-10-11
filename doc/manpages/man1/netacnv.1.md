# Name

netacnv — Netatalk character set and filename conversion

# Synopsis

**netacnv** [-vx] [-f *charset*] [-t *charset*] [-m *charset*]
[-o *conversion-option*]... [\-\-] *string*

**netacnv** -h

# Description

**netacnv** converts one command-line string using Netatalk's filename
conversion library and prints its original and converted forms. It is useful
for investigating character encoding, Unicode normalization, filename
escaping, and case conversion as used by the AFP server.

The default source charset is **UTF8-MAC**, the decomposed UTF-8 form used
for Mac filenames. The default target charset is **UTF8**. This conversion
precomposes supported sequences of base characters and combining marks.
Netatalk follows AFP filename normalization rules, including exceptions to
general Unicode normalization.

Filename punctuation receives special treatment. For a **MAC_ROMAN** or
**UTF8-MAC** target, **:** can become **/**, and **/** from either of these
source charsets can become **:**. Escaping options provide additional
handling of these characters.

Charset names may refer to Netatalk's built-in charsets, such as **UTF8**,
**UTF8-MAC**, **MAC_ROMAN**, and **ASCII**, or to encodings supported by the
system's iconv implementation. Each selected charset must support conversion
to and from Netatalk's intermediate Unicode representation. Available
encodings depend on the platform and build configuration.

Quote *string* when it contains spaces or shell metacharacters. Its bytes
must already be in the charset selected by **-f**. Use **\-\-** before a
string beginning with a hyphen. An empty quoted string is accepted.
Use **-x** to supply hexadecimal bytes, including the NUL bytes needed by
wide encodings such as **UTF-16LE** and **UTF-32BE**. The library decodes
source characters before processing filename punctuation and escapes.

# Options

**-f** *charset*

> Set the source charset. The default is **UTF8-MAC**.

**-h**

> Print usage, defaults, the size limit, and available conversion options
to standard output, then exit successfully. No input string is required.

**-m** *charset*

> Select the legacy Mac charset used to interpret or generate the bytes in
**:xx** hex escapes. The default is **MAC_ROMAN**. This option does not set
the source or target charset; use **-f** and **-t** for those. It does not
change which charsets receive Mac filename punctuation mapping.

**-o** *conversion-option*

> Enable a named conversion option from the list below. Names are case
sensitive and must include the **CONV_** prefix. Repeat **-o** to combine
options. Unknown names are errors. Uppercase and lowercase options cannot
be combined, and precomposition and decomposition options cannot be combined.

**-t** *charset*

> Set the target charset. The default is **UTF8**.

**-v**

> Print the selected charset names, byte lengths, hexadecimal bytes, and
conversion flags to standard error. On a conversion failure, the diagnostic
includes the input bytes and flags. Normal output remains on standard output.

**-x**

> Interpret *string* as hexadecimal byte pairs, with uppercase or lowercase
digits. Pairs may be adjacent or separated by ASCII whitespace; whitespace
inside a pair is an error. Empty input is accepted. For example, **41004200**
and **"41 00 42 00"** both supply the UTF-16LE bytes for *AB*. The charset
selected by **-f** describes the decoded bytes. Invalid hex syntax and odd
digit counts are errors, regardless of the conversion flags. The input size
limit applies to the decoded bytes.

When **-f**, **-m**, or **-t** is repeated, the last value takes effect.

# Conversion Options

**CONV_DECOMPOSE**

> Decompose supported precomposed characters into base characters and
combining marks. Cannot be combined with **CONV_PRECOMPOSE**.

**CONV_ESCAPEDOTS**

> Escape the leading dot in a name such as *.hidden* as **:2e**. The
converter reports **CONV_REQESCAPE** when it performs this substitution.

**CONV_ESCAPEHEX**

> Escape **:** as **:3a** and **/** as **:2f**. Characters that cannot be
represented in the target charset are converted through the charset selected
by **-m** and written as **:xx** byte escapes. Conversion still fails if that
charset cannot represent them. If **CONV_IGNORE** is also selected, it takes
precedence over escaping an unrepresentable character.

**CONV_FORCE**

> Allow the converter to return a partial result after an encoding or
output-space error. The unconverted suffix may be discarded. Argument
validation and other conversion failures can still cause a nonzero exit.

**CONV_IGNORE**

> Stop at an invalid source sequence or a character that cannot be
represented in the target charset, and return the converted prefix. The
converter sets **CONV_REQMANGLE** to indicate that the name needs mangling.
This option truncates the result rather than skipping the offending character.

**CONV_PRECOMPOSE**

> Combine supported base characters and combining marks into precomposed
characters. Cannot be combined with **CONV_DECOMPOSE**. Target charset
normalization still applies: a **UTF8-MAC** target produces decomposed text.

**CONV_TOLOWER**

> Convert characters to lowercase using Netatalk's Unicode case tables.
Cannot be combined with **CONV_TOUPPER**.

**CONV_TOUPPER**

> Convert characters to uppercase using Netatalk's Unicode case tables.
Cannot be combined with **CONV_TOLOWER**.

**CONV_UNESCAPEHEX**

> Decode **:xx** byte escapes using the charset selected by **-m**. Hex
digits may be uppercase or lowercase. Malformed escapes cause a conversion
error unless the selected error policy permits a partial result.

**CONV__EILSEQ**

> Replace invalid source code units with underscores and continue converting.
For byte encodings, one byte is replaced at a time; for UTF-16 and UTF-32,
replacement preserves the source code-unit alignment. A truncated final code
unit is replaced with one underscore.
This option has two underscores between **CONV** and **EILSEQ**. It does
not replace characters that are unrepresentable in the target charset.

# Output

On success, standard output contains:

    from: original string
    to: converted string

Both values are written using their byte lengths. With **-x**, the original
value contains the decoded source bytes, rather than the hex argument. Either
value can contain embedded NUL bytes or other bytes that a terminal does not
display meaningfully. Use **-v** to inspect the hex representation. Byte
lengths exclude added terminators; NUL bytes supplied as input are included.

Verbose flags include the requested options and any result flags reported
by the library. **CONV_REQMANGLE** indicates that filename mangling is
required; **CONV_REQESCAPE** indicates reported escape handling. These are
result flags and cannot be supplied with **-o**. **netacnv** reports them
but does not perform filename mangling.

# Examples

Convert UTF-16LE bytes, including encoded NUL bytes, to UTF-8:

    netacnv -x -v -f UTF-16LE -t UTF8 '41 00 42 00'

The verbose byte lines on standard error are:

    from bytes (4 bytes): 41 00 42 00
    to bytes (2 bytes): 41 42

Convert an ASCII string to uppercase:

    $ netacnv -f UTF8 -t UTF8 -o CONV_TOUPPER hello
    from: hello
    to: HELLO

Decode a Mac Roman hex escape into UTF-8:

    $ netacnv -f UTF8 -t UTF8 -m MAC_ROMAN -o CONV_UNESCAPEHEX ':8e'
    from: :8e
    to: é

Inspect the bytes produced by Unicode decomposition. This example assumes
the shell argument is the precomposed UTF-8 encoding of *é*:

    netacnv -v -f UTF8 -t UTF8-MAC 'é'

The verbose byte lines on standard error are:

    from bytes (2 bytes): c3 a9
    to bytes (3 bytes): 65 cc 81

Inspect a partial conversion to ASCII and its mangling flag:

    netacnv -v -f UTF8 -t ASCII -o CONV_IGNORE 'café'

Convert a string beginning with a hyphen:

    $ netacnv -- -filename
    from: -filename
    to: -filename

# Limitations

Input and output are each limited to the platform's **MAXPATHLEN** bytes.
The actual limit is printed by **-h**. Normalization and escaping can expand
the output, so an input within the limit can still fail with insufficient
space. Internal normalization buffers also impose limits on expansion.

Input is a single command-line argument. Without **-x**, it cannot contain
embedded NUL bytes; **-x** removes this restriction. Wide source and target
encodings are supported when the selected converter can represent the text.
Netatalk's built-in **UCS-2** charset uses the host's native byte order; use
an explicit encoding such as **UTF-16LE** for portable byte sequences.

The tool depends on the system's iconv implementation for charset support.

# Exit Status

**0**

> Conversion succeeded, or help was displayed. A partial conversion
accepted by **CONV_IGNORE** or **CONV_FORCE** can also succeed.

**1**

> An argument, charset, conversion, or output error occurred.

# See Also

**afpd**(8), **iconv**(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
