/*
 * Test program for include/atalk/byteorder.h macros.
 *
 * Verifies that the byte-by-byte shift macros produce identical results
 * to direct pointer dereferences on architectures where unaligned access
 * is permitted (e.g. x86). This confirms that removing Samba's original
 * CAREFUL_ALIGNMENT=0 optimization has no side effects.
 *
 * Build:  cc -I include -o test_byteorder test/test_byteorder.c
 * Run:    ./test_byteorder
 */

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Auto-detect host byte order when building standalone (without config.h).
 * byteorder.h keys off WORDS_BIGENDIAN to select the correct macro variants.
 */
#ifndef WORDS_BIGENDIAN
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#endif
#endif

#include <atalk/byteorder.h>

static int failures = 0;

#define CHECK(desc, got, expected) do { \
    if ((got) != (expected)) { \
        fprintf(stderr, "FAIL: %s: got 0x%llx, expected 0x%llx\n", \
                (desc), (unsigned long long)(got), \
                (unsigned long long)(expected)); \
        failures++; \
    } \
} while (0)

/*
 * 1. Round-trip: write with SSVAL/SIVAL/SLVAL, read back with SVAL/IVAL/LVAL.
 */
static void test_roundtrip(void)
{
    unsigned char buf[32];
    int offsets[] = {0, 1, 3, 5, 7};  /* aligned and unaligned */
    uint16_t vals16[] = {0x0000, 0x0001, 0x00FF, 0x0100, 0x1234, 0x8000, 0xFFFF};
    uint32_t vals32[] = {0x00000000, 0x00000001, 0x000000FF, 0x0000FF00,
                         0x12345678, 0x80000000, 0xDEADBEEF, 0xFFFFFFFF
                        };
    uint64_t vals64[] = {0x0000000000000000ULL, 0x0000000000000001ULL,
                         0x123456789ABCDEF0ULL, 0x8000000000000000ULL,
                         0xFFFFFFFFFFFFFFFFULL
                        };

    for (int oi = 0; oi < (int)(sizeof(offsets) / sizeof(offsets[0])); oi++) {
        int pos = offsets[oi];

        for (int i = 0; i < (int)(sizeof(vals16) / sizeof(vals16[0])); i++) {
            memset(buf, 0xAA, sizeof(buf));
            SSVAL(buf, pos, vals16[i]);
            uint16_t got = SVAL(buf, pos);
            CHECK("16-bit round-trip", got, vals16[i]);
        }

        for (int i = 0; i < (int)(sizeof(vals32) / sizeof(vals32[0])); i++) {
            memset(buf, 0xAA, sizeof(buf));
            SIVAL(buf, pos, vals32[i]);
            uint32_t got = IVAL(buf, pos);
            CHECK("32-bit round-trip", got, vals32[i]);
        }

        for (int i = 0; i < (int)(sizeof(vals64) / sizeof(vals64[0])); i++) {
            memset(buf, 0xAA, sizeof(buf));
            SLVAL(buf, pos, vals64[i]);
            uint64_t got = LVAL(buf, pos);
            CHECK("64-bit round-trip", got, vals64[i]);
        }
    }
}

/*
 * 2. Compare macro results against direct pointer dereferences.
 *    On little-endian hosts the macros should match *(uint16_t *) etc.
 *    On big-endian hosts the R (reverse) variants should match instead.
 */
static void test_vs_direct_cast(void)
{
    unsigned char buf[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                           0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
                          };
    int offsets[] = {0, 1, 3};

    for (int oi = 0; oi < (int)(sizeof(offsets) / sizeof(offsets[0])); oi++) {
        int pos = offsets[oi];
        uint16_t direct16;
        uint32_t direct32;
        memcpy(&direct16, buf + pos, sizeof(direct16));
        memcpy(&direct32, buf + pos, sizeof(direct32));
        /*
         * memcpy into a native integer gives us the host-native
         * interpretation. The macros should agree.
         */
        CHECK("16-bit vs memcpy native", (uint16_t)SVAL(buf, pos), direct16);
        CHECK("32-bit vs memcpy native", (uint32_t)IVAL(buf, pos), direct32);
    }
}

/*
 * 3. Verify R variants produce byte-swapped values.
 *    After writing val with SSVAL, RSVAL should return SREV(val) — the
 *    byte-reversed value. This holds on both LE and BE hosts.
 */
static void test_reverse_vs_swap(void)
{
    unsigned char buf[16];
    uint16_t vals16[] = {0x0102, 0x1234, 0xFFFF, 0x0001, 0x8000};
    uint32_t vals32[] = {0x01020304, 0x12345678, 0xFFFFFFFF, 0x00000001, 0x80000000};

    for (int i = 0; i < (int)(sizeof(vals16) / sizeof(vals16[0])); i++) {
        uint16_t val = vals16[i];
        memset(buf, 0, sizeof(buf));
        SSVAL(buf, 0, val);
        uint16_t reversed = RSVAL(buf, 0);
        uint16_t expected = SREV(val);
        CHECK("RSVAL vs SREV", reversed, expected);
    }

    for (int i = 0; i < (int)(sizeof(vals32) / sizeof(vals32[0])); i++) {
        uint32_t val = vals32[i];
        memset(buf, 0, sizeof(buf));
        SIVAL(buf, 0, val);
        uint32_t reversed = RIVAL(buf, 0);
        uint32_t expected = IREV(val);
        CHECK("RIVAL vs IREV", reversed, expected);
    }
}

/*
 * 4. Signed variants preserve sign correctly.
 */
static void test_signed(void)
{
    unsigned char buf[16];
    int16_t svals16[] = {0, 1, -1, INT16_MIN, INT16_MAX, -256, -2};

    for (int i = 0; i < (int)(sizeof(svals16) / sizeof(svals16[0])); i++) {
        SSVALS(buf, 0, svals16[i]);
        int16_t got = SVALS(buf, 0);
        CHECK("16-bit signed round-trip", (uint16_t)got, (uint16_t)svals16[i]);
    }

    int32_t svals32[] = {0, 1, -1, INT32_MIN, INT32_MAX, -256, -65536};

    for (int i = 0; i < (int)(sizeof(svals32) / sizeof(svals32[0])); i++) {
        SIVALS(buf, 0, svals32[i]);
        int32_t got = IVALS(buf, 0);
        CHECK("32-bit signed round-trip", (uint32_t)got, (uint32_t)svals32[i]);
    }

    int64_t svals64[] = {0, 1, -1, INT64_MIN, INT64_MAX};

    for (int i = 0; i < (int)(sizeof(svals64) / sizeof(svals64[0])); i++) {
        SLVALS(buf, 0, svals64[i]);
        int64_t got = LVALS(buf, 0);
        CHECK("64-bit signed round-trip", (uint64_t)got, (uint64_t)svals64[i]);
    }
}

/*
 * 5. Verify exact byte layout in the buffer.
 *    This is the most direct test of the byte order semantics:
 *    on LE hosts, SSVAL should produce LE layout;
 *    on BE hosts, SSVAL should produce BE layout.
 */
static void test_byte_layout(void)
{
    unsigned char buf[16];
    /* Write 0x0102 and check which byte ends up where */
    memset(buf, 0, sizeof(buf));
    SSVAL(buf, 0, 0x0102);
#ifdef WORDS_BIGENDIAN
    /* BE host: expect MSB first */
    CHECK("byte layout [0] BE", buf[0], 0x01);
    CHECK("byte layout [1] BE", buf[1], 0x02);
#else
    /* LE host: expect LSB first */
    CHECK("byte layout [0] LE", buf[0], 0x02);
    CHECK("byte layout [1] LE", buf[1], 0x01);
#endif
    memset(buf, 0, sizeof(buf));
    SIVAL(buf, 0, 0x01020304);
#ifdef WORDS_BIGENDIAN
    CHECK("32-bit byte layout [0] BE", buf[0], 0x01);
    CHECK("32-bit byte layout [1] BE", buf[1], 0x02);
    CHECK("32-bit byte layout [2] BE", buf[2], 0x03);
    CHECK("32-bit byte layout [3] BE", buf[3], 0x04);
#else
    CHECK("32-bit byte layout [0] LE", buf[0], 0x04);
    CHECK("32-bit byte layout [1] LE", buf[1], 0x03);
    CHECK("32-bit byte layout [2] LE", buf[2], 0x02);
    CHECK("32-bit byte layout [3] LE", buf[3], 0x01);
#endif
}

/*
 * 6. R variant round-trip: RSSVAL then RSVAL should return original value.
 */
static void test_reverse_roundtrip(void)
{
    unsigned char buf[16];
    uint16_t vals16[] = {0x0102, 0x1234, 0xFFFF, 0x0000, 0x8000};
    uint32_t vals32[] = {0x01020304, 0x12345678, 0xFFFFFFFF, 0x00000000, 0x80000000};

    for (int i = 0; i < (int)(sizeof(vals16) / sizeof(vals16[0])); i++) {
        RSSVAL(buf, 0, vals16[i]);
        uint16_t got = RSVAL(buf, 0);
        CHECK("16-bit reverse round-trip", got, vals16[i]);
    }

    for (int i = 0; i < (int)(sizeof(vals32) / sizeof(vals32[0])); i++) {
        RSIVAL(buf, 0, vals32[i]);
        uint32_t got = RIVAL(buf, 0);
        CHECK("32-bit reverse round-trip", got, vals32[i]);
    }
}

int main(void)
{
#ifdef WORDS_BIGENDIAN
    printf("Host byte order: big-endian\n");
#else
    printf("Host byte order: little-endian\n");
#endif
    test_roundtrip();
    test_vs_direct_cast();
    test_reverse_vs_swap();
    test_signed();
    test_byte_layout();
    test_reverse_roundtrip();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    } else {
        fprintf(stderr, "%d test(s) FAILED.\n", failures);
        return 1;
    }
}
