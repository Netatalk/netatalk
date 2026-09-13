/* Shared SRP parameters, verifier record format, and permission policy. */
#ifndef _ATALK_SRP_H
#define _ATALK_SRP_H 1

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SRP_SALT_LEN      16
#define SRP_NBYTES        192   /* 1536-bit prime */
#define SRP_SHA1_LEN      20
#define SRP_GROUP_INDEX   0x0002  /* RFC 5054 group #2 */
#define SRP_USERNAME_MAX_LEN 255
#define SRP_DISABLED_CHAR '*'

/* Hex lengths exclude separators and the terminating NUL. */
#define SRP_HEX_SALT_LEN  (SRP_SALT_LEN * 2)
#define SRP_HEX_V_LEN     (SRP_NBYTES * 2)
/* salt:verifier\n, excluding the terminating NUL. */
#define SRP_FIELDS_LEN   (SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN + 1)
/* :salt:verifier\n, excluding the username and terminating NUL. */
#define SRP_FORMAT_LEN   (1 + SRP_FIELDS_LEN)

/*
 * RFC 5054 group #2: 1536-bit MODP group. N is the safe prime, g = 2.
 */
static const unsigned char srp_N_bytes[SRP_NBYTES] = {
    0x9D, 0xEF, 0x3C, 0xAF, 0xB9, 0x39, 0x27, 0x7A,
    0xB1, 0xF1, 0x2A, 0x86, 0x17, 0xA4, 0x7B, 0xBB,
    0xDB, 0xA5, 0x1D, 0xF4, 0x99, 0xAC, 0x4C, 0x80,
    0xBE, 0xEE, 0xA9, 0x61, 0x4B, 0x19, 0xCC, 0x4D,
    0x5F, 0x4F, 0x5F, 0x55, 0x6E, 0x27, 0xCB, 0xDE,
    0x51, 0xC6, 0xA9, 0x4B, 0xE4, 0x60, 0x7A, 0x29,
    0x15, 0x58, 0x90, 0x3B, 0xA0, 0xD0, 0xF8, 0x43,
    0x80, 0xB6, 0x55, 0xBB, 0x9A, 0x22, 0xE8, 0xDC,
    0xDF, 0x02, 0x8A, 0x7C, 0xEC, 0x67, 0xF0, 0xD0,
    0x81, 0x34, 0xB1, 0xC8, 0xB9, 0x79, 0x89, 0x14,
    0x9B, 0x60, 0x9E, 0x0B, 0xE3, 0xBA, 0xB6, 0x3D,
    0x47, 0x54, 0x83, 0x81, 0xDB, 0xC5, 0xB1, 0xFC,
    0x76, 0x4E, 0x3F, 0x4B, 0x53, 0xDD, 0x9D, 0xA1,
    0x15, 0x8B, 0xFD, 0x3E, 0x2B, 0x9C, 0x8C, 0xF5,
    0x6E, 0xDF, 0x01, 0x95, 0x39, 0x34, 0x96, 0x27,
    0xDB, 0x2F, 0xD5, 0x3D, 0x24, 0xB7, 0xC4, 0x86,
    0x65, 0x77, 0x2E, 0x43, 0x7D, 0x6C, 0x7F, 0x8C,
    0xE4, 0x42, 0x73, 0x4A, 0xF7, 0xCC, 0xB7, 0xAE,
    0x83, 0x7C, 0x26, 0x4A, 0xE3, 0xA9, 0xBE, 0xB8,
    0x7F, 0x8A, 0x2F, 0xE9, 0xB8, 0xB5, 0x29, 0x2E,
    0x5A, 0x02, 0x1F, 0xFF, 0x5E, 0x91, 0x47, 0x9E,
    0x8C, 0xE7, 0xA2, 0x8C, 0x24, 0x42, 0xC6, 0xF3,
    0x15, 0x18, 0x0F, 0x93, 0x49, 0x9A, 0x23, 0x4D,
    0xCF, 0x76, 0xE3, 0xFE, 0xD1, 0x35, 0xF9, 0xBB,
};

static const unsigned char srp_g_byte = 0x02;

/* Validate a NUL-terminated username for a verifier record. */
static inline int srp_valid_username(const char *name)
{
    size_t length = strnlen(name, SRP_USERNAME_MAX_LEN + 1);
    return length > 0 && length <= SRP_USERNAME_MAX_LEN &&
           strchr(name, ':') == NULL && strchr(name, '\n') == NULL &&
           strchr(name, '\r') == NULL;
}

/*
 * Validate NUL-terminated salt:verifier\n fields, including fully disabled
 * records. Authentication must reject disabled records separately.
 */
static inline int srp_valid_fields(const char *fields)
{
    int salt_disabled = 1, verifier_disabled = 1;

    if (strlen(fields) != SRP_FIELDS_LEN ||
            fields[SRP_HEX_SALT_LEN] != ':' ||
            fields[SRP_FIELDS_LEN - 1] != '\n') {
        return 0;
    }

    for (size_t i = 0; i < SRP_HEX_SALT_LEN; i++) {
        salt_disabled &= fields[i] == SRP_DISABLED_CHAR;
    }

    for (size_t i = 0; i < SRP_HEX_V_LEN; i++) {
        verifier_disabled &= fields[SRP_HEX_SALT_LEN + 1 + i] == SRP_DISABLED_CHAR;
    }

    if (salt_disabled || verifier_disabled) {
        return salt_disabled && verifier_disabled;
    }

    for (size_t i = 0; i < SRP_HEX_SALT_LEN; i++) {
        if (!isxdigit((unsigned char)fields[i])) {
            return 0;
        }
    }

    for (size_t i = 0; i < SRP_HEX_V_LEN; i++) {
        if (!isxdigit((unsigned char)fields[SRP_HEX_SALT_LEN + 1 + i])) {
            return 0;
        }
    }

    return 1;
}

/* Owner permissions may vary; group and other must have no access. */
static inline int srp_verifier_mode_is_safe(mode_t mode)
{
    return (mode & (S_IRWXG | S_IRWXO)) == 0;
}

#endif
