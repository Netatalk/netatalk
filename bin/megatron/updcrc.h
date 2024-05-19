#ifndef _UPDCRC_H
#define _UPDCRC_H 1

#define W	16	/* bits in CRC:16	16	16	*/

    /* data type that holds a W-bit unsigned integer */
#if W <= 16
#  define WTYPE	unsigned short
#else /* W <= 16 */
#  define WTYPE   uint32_t
#endif /* W <= 16 */

WTYPE updcrc(WTYPE icrc, unsigned char *icp, int icnt);

#endif /* _UPDCRC_H */
