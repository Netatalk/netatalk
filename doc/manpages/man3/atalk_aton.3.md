# Name

atalk_aton — AppleTalk address parsing

# Synopsis

    #include <sys/types.h>
    #include <netatalk/at.h>

    atalk_aton(	cp, 	 
        ata);	 
    char * cp;
    struct at_addr * ata;

# Description

The `atalk_aton()` routine converts an ASCII representation of an
AppleTalk address to a format appropriate for system calls. Acceptable
ASCII representations include both hex and base 10, in triples or
doubles. For instance, the address \`0x1f6b.77' has a network part of
\`8043' and a node part of \`119'. This same address could be written
\`8043.119', \`31.107.119', or \`0x1f.6b.77'. If the address is in hex
and the first digit is one of \`A-F', a leading \`0x' is redundant.

# See Also

`atalk(4)`.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
