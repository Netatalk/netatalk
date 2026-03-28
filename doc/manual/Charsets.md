# Character Sets

## Introduction to character sets

Internally, computers only work with numbers, not characters.
Each letter is assigned a number, and a character set —
often called a *charset* or *codepage* —
defines the mappings between numbers and letters.

When two or more computer systems communicate, they must use the same character set.
In the 1960s, the ASCII (American Standard Code for Information Interchange)
character set was defined by the American Standards Association.
The original form of ASCII represented 128 characters,
more than enough to cover the English alphabet and numerals.
To this day, ASCII has been the foundational character encoding used by computers.

Later extensions defined 256 characters to accommodate international
characters and some graphical symbols.
In this mode of encoding, each character takes exactly one byte.
Obviously, 256 characters was still not enough to represent all the characters
used across the world's languages in a single character set.

As a result, localized character sets were defined, such as the ISO-8859 family.
Most operating system vendors introduced their own character sets:
IBM defined *codepage 437 (DOSLatinUS)*, Apple introduced *MacRoman*, and so on.
Characters assigned numbers larger than 127 were referred to as *extended* characters.
These character sets conflict with one another,
as they use the same number for different characters, or vice versa.

Almost all of these character sets defined 256 characters,
with the first 128 (0–127) mappings identical to ASCII.
As a result, communication between systems using different codepages was
effectively limited to the ASCII subset.

To solve this problem, new, larger character sets were defined.
To make room for more mappings, these character sets use at least 2 bytes per character.
They are therefore referred to as *multibyte* character sets.

One standardized multibyte encoding scheme is [Unicode](http://www.unicode.org/).
A key advantage of using a multibyte charset is that you only need one.
There is no need to ensure that two computers use the same charset when communicating.

## Character sets used by Apple

In the past, Apple clients used single-byte charsets for network communication.
Over the years Apple defined a number of codepages;
Western users most commonly used the *MacRoman* codepage.

Codepages defined by Apple include:

- MacArabic, MacFarsi

- MacCentralEurope

- MacChineseSimple

- MacChineseTraditional

- MacCroatian

- MacCyrillic

- MacDevanagari

- MacGreek

- MacHebrew

- MacIcelandic

- MacJapanese

- MacKorean

- MacRoman

- MacRomanian

- MacThai

- MacTurkish

Starting with Mac OS X and AFP 3.x, [UTF-8](http://www.utf-8.com/) is used.
UTF-8 encodes Unicode characters in an ASCII-compatible way;
each Unicode character is encoded into 1–4 bytes.
UTF-8 is therefore not a charset itself, but rather an encoding of the Unicode charset.

To complicate matters, Unicode defines several
*[normalization](http://www.unicode.org/reports/tr15/index.html)* forms.
While [Samba](http://www.samba.org) uses *precomposed* Unicode
(which most UNIX tools prefer as well),
Apple chose *decomposed* normalization.

For example, take the character 'ä' (lowercase a with diaeresis).
Using precomposed normalization, Unicode maps this character to 0xE4.
In decomposed normalization, 'ä' is mapped to two characters:
0x61 for 'a' and 0x308 for *COMBINING DIAERESIS*.

Netatalk refers to precomposed UTF-8 as *UTF8* and to decomposed UTF-8 as
*UTF8-MAC*.

## afpd and character sets

To support both AFP 3.x and older AFP 2.x clients simultaneously,
**afpd** must convert between the various charsets in use.
AFP 3.x clients always use UTF8-MAC,
while AFP 2.x clients use one of the Apple codepages.

At the time of writing, Netatalk supports the following Apple codepages:

- MAC_CENTRALEUROPE

- MAC_CHINESE_SIMP

- MAC_CHINESE_TRAD

- MAC_CYRILLIC

- MAC_GREEK

- MAC_HEBREW

- MAC_JAPANESE

- MAC_KOREAN

- MAC_ROMAN

- MAC_TURKISH

**afpd** handles three different character set options:

unix charset

> The codepage used internally by your operating system.
If not specified, it defaults to **UTF8**.
If **LOCALE** is specified and your system supports UNIX locales,
**afpd** tries to detect the codepage.
**afpd** uses this codepage to read its configuration files,
so you can use extended characters for volume names, login messages, etc.

mac charset

> Older Mac OS clients (up to AFP 2.2) use codepages to communicate with **afpd**.
However, the AFP protocol has no support for negotiating the codepage in use.
If not specified otherwise, **afpd** assumes *MacRoman*.
If your clients use a different codepage, such as *MacCyrillic*,
you **must** configure this explicitly.

vol charset

> The charset that **afpd** uses for filenames on disk.
By default, this is the same as **unix charset**.
If you have [iconv](http://www.gnu.org/software/libiconv/) installed,
you can use any iconv-provided charset as well.

**afpd** needs a way to preserve extended Macintosh characters,
or characters illegal in UNIX filenames, when saving files on a UNIX filesystem.
Earlier versions used the so-called CAP encoding.
An extended character (>0x7F) was converted to a *:xx* hex sequence —
for example, the Apple Logo (MacRoman: 0xF0) was saved as *:f0*.
Some special characters are also converted to *:xx* notation: */* is encoded as *:2f*,
and if **usedots** is not specified, a leading dot *.* is encoded as *:2e*.

Even though this version now uses **UTF8** as the default encoding for filenames,
*/* is still converted to *:*. For western users,
another useful setting is **vol charset = ISO-8859-15**.

If a character cannot be converted from the **mac charset** to the selected **vol charset**,
you will receive a -50 error on the Mac.
*Note*: Whenever possible, use the default UTF8 volume format.
