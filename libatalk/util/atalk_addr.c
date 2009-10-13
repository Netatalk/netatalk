#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/util.h>
#include <ctype.h>

/* 
 * Check whether "cp" is a valid ascii representation
 * of an AppleTalk address and convert to a binary address.
 * Examples of accepted forms are (in decimal, net of 4321,
 * node of 65):
 *
 *	4321.65
 *	0x10E1.41
 *	16.225.65
 *	0x10.E1.41
 *
 * If hex is used, and the first digit is one of A-F, the leading
 * 0x is redundant. Returns 1 if the address is valid, 0 if not.
 *
 * Unlike Internet addresses, AppleTalk addresses can have leading
 * 0's. This means that we can't support octal addressing.
 */

int atalk_aton(char *cp, struct at_addr *addr)
{
    u_int32_t val, base, n;
    char c;

    val = 0; base = 10;
    if ( *cp == '0' && ( *++cp == 'x' || *cp == 'X' )) {
	base = 16, cp++;
    }
    if ( !isdigit( *cp ) && isxdigit( *cp )) {
	base = 16;
    }

    for ( n = 0;; n++ ) {
	while (( c = *cp ) != '\0') {
	    if ( isascii( c ) && isdigit( c )) {
		val = (val * base) + (c - '0');
		cp++;
		continue;
	    }

	    if ( base == 16 && isascii( c ) && isxdigit( c )) {
		val = ( val << 4 ) + ( c + 10 - ( islower( c ) ? 'a' : 'A' ));
		cp++;
		continue;
	    }
	    break;
	}

	if ( c != '.' && c != '\0' ) {
	    return( 0 );
	}

	switch ( n ) {
	case 0:
	    if ( addr ) {
		if ( val > 65535 ) {
		    return( 0 );
		}
		addr->s_net = val;
	    }
	    if ( *cp++ ) {
		val = 0;
	    } else {
		break;
	    }
	    continue;

	case 2:
	    if ( addr ) {
		if ( addr->s_net > 255 ) {
		    return( 0 );
		}
		addr->s_net <<= 8;
		addr->s_net += addr->s_node;
	    }
	    /*FALLTHROUGH*/

	case 1:
	    if ( addr ) {
		if ( val > 255 ) {
		    return( 0 );
		}
		addr->s_node = val;
	    }
	    if ( *cp++ ) {
		val = 0;
	    } else {
		break;
	    }
	    continue;

	default:
	    return( 0 );
	}
	break;
    }

    if ( n < 1 ) {
	return( 0 );
    }
    if ( addr ) {
	addr->s_net = htons( addr->s_net );
    }
    return (1);
}
