#ifndef AFPD_CODEPAGE_H
#define AFPD_CODEPAGE_H 1

/* 
 * header of a codepage --
 * file magic:                   2 bytes
 * file version:                 1 byte
 * size of name:                 1 byte
 * quantum:                      1 byte
 * rules:                        1 byte
 * offset to data:               2 bytes (network byte order)
 * size of data:                 2 bytes (network byte order)
 * name:                         strlen(name) bytes (padded)
 * 
 * data for a version 2 codepage --
 * mapping rule: 1 byte
 * mac/bad code:     <quantum> bytes
 * xlate/rule code:  <quantum> bytes
 * ...
 */

#define CODEPAGE_FILE_ID 0xCFAF
#define CODEPAGE_FILE_VERSION 0x02
#define CODEPAGE_FILE_HEADER_SIZE 10

/* m->u and u->m mappings */
#define CODEPAGE_RULE_MTOU       (1 << 0)
#define CODEPAGE_RULE_UTOM       (1 << 1)
#define CODEPAGE_RULE_BADU       (1 << 2)

/* rules for bad character assignment.
 *
 * bit assignments:
 * bit 0 set:  it's an illegal character
 *     unset:  it's okay
 *
 * bit 1 set:  bad anywhere
 *     unset:  only check at the beginning of each quantum. 
 *
 * bit 2 set:  all quanta are the same length
 *     unset:  quanta beginning with ascii characters are 1-byte long  
 *
 */
#define CODEPAGE_BADCHAR_SET      (1 << 0) 
#define CODEPAGE_BADCHAR_ANYWHERE (1 << 1)  
#define CODEPAGE_BADCHAR_QUANTUM  (1 << 2) 
#endif
