/*!
 * @file
 * @brief Parsing of AppleTalk addresses in text form
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef NO_DDP

#include <arpa/inet.h>
#include <sys/types.h>
#include <netatalk/at.h>
#include <atalk/util.h>
#include <ctype.h>

/*!
 * @brief Parse the text form of an AppleTalk address
 *
 * The net.node form takes a net up to 65535 and a node up to 255. In the
 * three-part form the first two parts hold the net, a byte each. A leading
 * 0x, or a first digit from A to F, makes every part hexadecimal. AppleTalk
 * addresses can carry leading zeros, so the call has no octal form. A failed
 * parse can leave part of the address in addr.
 *
 * Accepted forms for net 4321 and node 65:
 * @code
 *	4321.65
 *	0x10E1.41
 *	16.225.65
 *	0x10.E1.41
 * @endcode
 *
 * @param[in]  cp    address text
 * @param[out] addr  receives the address with the net in network order, or NULL
 *                   to check only the syntax, which skips the range checks
 * @returns 1 for a valid address, 0 otherwise
 */
int atalk_aton(char *cp, struct at_addr *addr)
{
    uint32_t val, base, n;
    char c;
    val = 0;
    base = 10;

    if (*cp == '0' && (*++cp == 'x' || *cp == 'X')) {
        base = 16, cp++;
    }

    if (!isdigit(*cp) && isxdigit(*cp)) {
        base = 16;
    }

    for (n = 0;; n++) {
        while ((c = *cp) != '\0') {
            if (isascii(c) && isdigit(c)) {
                val = (val * base) + (c - '0');
                cp++;
                continue;
            }

            if (base == 16 && isascii(c) && isxdigit(c)) {
                val =
                    (val << 4) + (c + 10 -
                                  (islower(c) ? 'a' :
                                   'A'));
                cp++;
                continue;
            }

            break;
        }

        if (c != '.' && c != '\0') {
            return 0;
        }

        switch (n) {
        case 0:
            if (addr) {
                if (val > 65535) {
                    return 0;
                }

                addr->s_net = val;
            }

            if (*cp++) {
                val = 0;
            } else {
                break;
            }

            continue;

        case 2:
            if (addr) {
                if (addr->s_net > 255) {
                    return 0;
                }

                addr->s_net <<= 8;
                addr->s_net += addr->s_node;
            }

    /*FALLTHROUGH*/ case 1:
            if (addr) {
                if (val > 255) {
                    return 0;
                }

                addr->s_node = val;
            }

            if (*cp++) {
                val = 0;
            } else {
                break;
            }

            continue;

        default:
            return 0;
        }

        break;
    }

    if (n < 1) {
        return 0;
    }

    if (addr) {
        addr->s_net = htons(addr->s_net);
    }

    return 1;
}

#endif  /* NO_DDP */
