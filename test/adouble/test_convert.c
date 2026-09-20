/*
 * Regression tests for the OS X AppleDouble sidecar conversion bounds.
 *
 * Crafted ._ sidecars are opened through ad_open() with resource-fork
 * write intent, mirroring the afpd call sequence. Malformed entries must
 * be rejected with errno EIO and the sidecar must be left untouched; a
 * well-formed sidecar must be rewritten to the canonical OS X layout with
 * the resource fork payload preserved byte for byte.
 *
 * The conversion relies on these two properties:
 *
 *   source range  [roff, roff + rlen)      is inside the sidecar
 *   destination   [ADEDOFF_RFORK_OSX, +rlen) is inside the sidecar
 *
 * Both are attacker controlled because parse_entries() exempts ADEID_RFORK
 * from its bounds check. The converter validates the source before copying.
 * Since it separately requires roff >= ADEDOFF_RFORK_OSX, that proof also
 * proves the destination range fits.
 *
 * The suite is skipped whenever HAVE_EAFD is enabled because the converter
 * is unreachable, and unconditionally on macOS because ad_path_osx() names
 * a native /..namedfork/rsrc stream there. Such a stream must never be used
 * as an AppleDouble fixture, including in unsupported fallback builds.
 */

#include "config.h"

#include <atalk/adouble.h>
#include <atalk/logger.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

/* Declared in <atalk/util.h>; avoid its unrelated bstring dependency here. */
extern const char *tmpdir(void);

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char dfpath[PATH_MAX];   /* victim data fork */
static char scpath[PATH_MAX];   /* sidecar path as ad_path_osx() reports it */
static char bakpath[PATH_MAX];  /* untouched copy of the crafted sidecar */
static int  failures;

#ifdef HAVE_ADOUBLE_SHORT_READ_INJECTOR
/* Exported for short_read_inject.c. */
int adouble_test_short_header_read;
int adouble_test_short_header_read_fired;
#endif

struct adent {
    uint32_t eid;
    uint32_t off;
    uint32_t len;
};

/* Bail out before doing anything if a fixture could not be written. */
static void fail(const char *name, const char *what)
{
    printf("not ok %s: %s\n", name, what);
    failures++;
}

static void write_sidecar(const struct adent *ents, size_t nents,
                          const char *tail, size_t taillen)
{
    unsigned char hdr[AD_HEADER_LEN];
    uint32_t magic = htonl(AD_MAGIC);
    uint32_t version = htonl(AD_VERSION2);
    uint16_t nent = htons((uint16_t)nents);
    size_t i;
    memset(hdr, 0, sizeof(hdr));
    memcpy(hdr + 4u, &version, sizeof(version));
    memcpy(hdr, &magic, sizeof(magic));
    memcpy(hdr + 24u, &nent, sizeof(nent));
    FILE *f = fopen(scpath, "wb");

    if (f == NULL) {
        fail("setup", "cannot create sidecar");
        exit(1);
    }

    if (fwrite(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        fail("setup", "short write");
        exit(1);
    }

    for (i = 0; i < nents; i++) {
        uint32_t e = htonl(ents[i].eid);
        uint32_t o = htonl(ents[i].off);
        uint32_t l = htonl(ents[i].len);

        if (fwrite(&e, sizeof(e), 1, f) != 1
                || fwrite(&o, sizeof(o), 1, f) != 1
                || fwrite(&l, sizeof(l), 1, f) != 1) {
            fail("setup", "short write");
            exit(1);
        }
    }

    if (taillen && fwrite(tail, 1, taillen, f) != taillen) {
        fail("setup", "short write");
        exit(1);
    }

    if (fclose(f) != 0) {
        fail("setup", "cannot close sidecar");
        exit(1);
    }
}

static void save_sidecar(void)
{
    FILE *src = fopen(scpath, "rb");
    FILE *dst = fopen(bakpath, "wb");
    char buf[4096];
    size_t n;

    if (src == NULL || dst == NULL) {
        fail("setup", "cannot back up sidecar");
        exit(1);
    }

    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            fail("setup", "short write");
            exit(1);
        }
    }

    fclose(src);
    fclose(dst);
}

static int sidecar_unchanged(void)
{
    FILE *a = fopen(scpath, "rb");
    FILE *b = fopen(bakpath, "rb");
    char buf1[4096], buf2[4096];
    size_t n1, n2;

    if (a == NULL || b == NULL) {
        return 0;
    }

    do {
        n1 = fread(buf1, 1, sizeof(buf1), a);
        n2 = fread(buf2, 1, sizeof(buf2), b);

        if (n1 != n2 || (n1 > 0 && memcmp(buf1, buf2, n1) != 0)) {
            fclose(a);
            fclose(b);
            return 0;
        }
    } while (n1 > 0);

    fclose(a);
    fclose(b);
    return 1;
}

static int open_rf_write(void)
{
    struct adouble ad;
    int rc;
    memset(&ad, 0, sizeof(ad));
    ad_init_old(&ad, AD_VERSION_EA, 0);
    rc = ad_open(&ad, dfpath,
                 ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF | ADFLAGS_RDWR);

    if (rc == 0) {
        ad_close(&ad, ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF);
    }

    return rc;
}

/* First open with create intent, mirroring afpd: establishes the metadata
 * EA so that later resource-fork opens take the OS X sidecar path. The
 * variadic argument of ad_open() is the creation mode, and ADFLAGS_CREATE
 * alone makes it create the forks. */
static void init_meta(void)
{
    struct adouble ad;
    memset(&ad, 0, sizeof(ad));
    ad_init_old(&ad, AD_VERSION_EA, 0);

    if (ad_open(&ad, dfpath,
                ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_CREATE | ADFLAGS_RDWR,
                0644) == 0) {
        ad_close(&ad, ADFLAGS_DF | ADFLAGS_HF);
    }
}

static off_t file_size(const char *p)
{
    struct stat st;

    if (stat(p, &st) != 0) {
        return -1;
    }

    return st.st_size;
}

static ssize_t read_sidecar(unsigned char *buf, size_t buflen)
{
    int fd = open(scpath, O_RDONLY);
    ssize_t got;

    if (fd < 0) {
        return -1;
    }

    got = read(fd, buf, buflen);
    close(fd);
    return got;
}

/*
 * Why a rejection vector is expected to be rejected, independently of what
 * the converter decides. Bounds vectors carry one of these so that the
 * fixture itself is checked and a vector cannot pass by accident;
 * structural vectors (malformed FinderInfo, missing entry) use REASON_NONE.
 */
enum reject_reason {
    REASON_NONE = 0,   /* rejected structurally, not by a resource fork bound */
    REASON_ROFF_LOW,   /* fork starts before its canonical destination (82) */
    REASON_SOURCE,     /* [roff, roff + rlen) leaves the sidecar */
};

/* Reject check: the open must fail with EIO and the sidecar must be
 * byte-identical, proving the conversion bailed out before writing. */
static void expect_reject(const char *name, enum reject_reason reason,
                          const struct adent *ents, size_t nents,
                          const char *tail, size_t taillen)
{
    off_t size = (off_t)(AD_HEADER_LEN + nents * AD_ENTRY_LEN + taillen);
    int rc;

    if (reason != REASON_NONE) {
        const struct adent *fi = NULL;
        const struct adent *rf = NULL;
        off_t roff, rlen;
        int low, source;
        size_t k;

        for (k = 0; k < nents; k++) {
            if (ents[k].eid == ADEID_FINDERI) {
                fi = &ents[k];
            }

            if (ents[k].eid == ADEID_RFORK) {
                rf = &ents[k];
            }
        }

        /* Bounds fixtures must pass every earlier structural check. */
        if (fi == NULL || rf == NULL
                || fi->off != ADEDOFF_FINDERI_OSX
                || fi->len < ADEDLEN_FINDERI
                || (off_t)fi->off > size
                || (off_t)fi->len > size - (off_t)fi->off) {
            printf("not ok %s: fixture violates an earlier FinderInfo "
                   "precondition\n", name);
            failures++;
            return;
        }

        roff = (off_t)rf->off;
        rlen = (off_t)rf->len;
        low = roff < ADEDOFF_RFORK_OSX;
        source = roff > size || rlen > size - roff;

        if ((reason == REASON_ROFF_LOW && !low)
                || (reason == REASON_SOURCE
                    && (low || roff > size || !source))) {
            printf("not ok %s: fixture does not exhibit the condition it is "
                   "meant to test (off %u, len %u, size %lld)\n",
                   name, rf->off, rf->len, (long long)size);
            failures++;
            return;
        }
    }

    write_sidecar(ents, nents, tail, taillen);
    save_sidecar();
    errno = 0;
    rc = open_rf_write();

    if (rc != -1) {
        printf("not ok %s: ad_open returned %d, expected -1\n", name, rc);
        failures++;
        return;
    }

    if (errno != EIO) {
        printf("not ok %s: errno %d (%s), expected EIO\n",
               name, errno, strerror(errno));
        failures++;
        return;
    }

    if (!sidecar_unchanged()) {
        printf("not ok %s: rejected but the sidecar was modified\n", name);
        failures++;
        return;
    }

    printf("ok %s\n", name);
}

/*
 * Model a sidecar that was 114 bytes at fstat(), then was truncated to the
 * 50-byte fixed header and two-entry table before the 82-byte header pread().
 * The unread FinderInfo bytes must not be synthesized from the zero-filled
 * ad_data buffer and written back as a canonical 82-byte sidecar.
 */
static void expect_short_header_after_stat(void)
{
#ifdef HAVE_ADOUBLE_SHORT_READ_INJECTOR
    const char *name = "reject header truncated after fstat";
    unsigned char tail[ADEDLEN_FINDERI + 32];
    const struct adent ents[] = {
        { ADEID_RFORK,   ADEDOFF_FINDERI_OSX + ADEDLEN_FINDERI + 32, 0 },
        { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32 },
    };
    int rc;
    memset(tail, 'F', sizeof(tail));
    write_sidecar(ents, 2, (char *)tail, sizeof(tail));
    save_sidecar();
    adouble_test_short_header_read_fired = 0;
    adouble_test_short_header_read = 1;
    errno = 0;
    rc = open_rf_write();
    adouble_test_short_header_read = 0;

    if (adouble_test_short_header_read_fired != 1) {
        fail(name, "short-read injector did not intercept the header read");
        return;
    }

    if (rc != -1) {
        printf("not ok %s: ad_open returned %d, expected -1\n", name, rc);
        failures++;
        return;
    }

    if (errno != EIO) {
        printf("not ok %s: errno %d (%s), expected EIO\n",
               name, errno, strerror(errno));
        failures++;
        return;
    }

    if (!sidecar_unchanged()) {
        fail(name, "short header was rebuilt with unread FinderInfo bytes");
        return;
    }

    printf("ok %s\n", name);
#endif
}

static int find_entry(const unsigned char *buf, size_t buflen,
                      uint32_t eid, uint32_t *off, uint32_t *len)
{
    uint16_t nent;
    size_t i;

    if (buflen < AD_HEADER_LEN + 2) {
        return -1;
    }

    memcpy(&nent, buf + 24u, sizeof(nent));
    nent = ntohs(nent);

    for (i = 0; i < nent; i++) {
        size_t eoff = AD_HEADER_LEN + i * AD_ENTRY_LEN;
        uint32_t e, o, l;

        if (eoff + AD_ENTRY_LEN > buflen) {
            return -1;
        }

        memcpy(&e, buf + eoff, 4);
        memcpy(&o, buf + eoff + 4, 4);
        memcpy(&l, buf + eoff + 8, 4);

        if (ntohl(e) == eid) {
            *off = ntohl(o);
            *len = ntohl(l);
            return 0;
        }
    }

    return -1;
}

/*
 * Legitimate conversion. Apple's AppleDouble may carry a FinderInfo entry
 * longer than the canonical 32 bytes (packed xattrs); netatalk discards
 * the extra bytes. The resource fork therefore sits at
 * ADEDOFF_FINDERI_OSX + finderlen (114 for a 64 byte FinderInfo entry),
 * NOT at the canonical ADEDOFF_RFORK_OSX, and must be moved down to
 * ADEDOFF_RFORK_OSX with every payload byte preserved.
 */
static void expect_legit_conversion(void)
{
    const uint32_t finderlen = ADEDLEN_FINDERI + 32;
    const uint32_t roff = ADEDOFF_FINDERI_OSX + finderlen; /* 114 */
    const uint32_t rlen = 400;
    unsigned char tail[64 + 400];
    const struct adent ents[] = {
        { ADEID_RFORK,   roff, rlen },
        { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, finderlen },
    };
    unsigned char buf[1024];
    ssize_t got;
    uint32_t off, len;
    size_t i;
    int rc;
    memset(tail, 'F', 64);

    for (i = 0; i < rlen; i++) {
        tail[64 + i] = (unsigned char)(i % 251 + 1);
    }

    write_sidecar(ents, 2, (char *)tail, sizeof(tail));
    rc = open_rf_write();

    if (rc != 0) {
        printf("not ok legit conversion: ad_open returned %d, expected 0\n", rc);
        failures++;
        return;
    }

    if (file_size(scpath) != (off_t)(ADEDOFF_RFORK_OSX + rlen)) {
        printf("not ok legit conversion: sidecar size %lld, expected %u\n",
               (long long)file_size(scpath), ADEDOFF_RFORK_OSX + rlen);
        failures++;
        return;
    }

    got = read_sidecar(buf, sizeof(buf));

    if (got < 0 || (size_t)got < ADEDOFF_RFORK_OSX + rlen) {
        fail("legit conversion", "cannot read converted sidecar");
        return;
    }

    if (find_entry(buf, got, ADEID_FINDERI, &off, &len) != 0
            || off != ADEDOFF_FINDERI_OSX || len != ADEDLEN_FINDERI) {
        fail("legit conversion", "malformed FinderInfo entry in output");
        return;
    }

    if (find_entry(buf, got, ADEID_RFORK, &off, &len) != 0
            || off != ADEDOFF_RFORK_OSX || len != rlen) {
        fail("legit conversion", "malformed resource fork entry in output");
        return;
    }

    /* The first 32 FinderInfo bytes are the canonical FinderInfo and are
     * carried over; the 32 packed-xattr bytes behind them are dropped. */
    for (i = 0; i < ADEDLEN_FINDERI; i++) {
        if (buf[ADEDOFF_FINDERI_OSX + i] != 'F') {
            printf("not ok legit conversion: FinderInfo payload corrupted at %zu\n", i);
            failures++;
            return;
        }
    }

    /* All 400 resource fork bytes must match the original payload. */
    for (i = 0; i < rlen; i++) {
        if (buf[ADEDOFF_RFORK_OSX + i] != tail[64 + i]) {
            printf("not ok legit conversion: resource fork payload corrupted at %zu "
                   "(got 0x%02x, want 0x%02x)\n",
                   i, buf[ADEDOFF_RFORK_OSX + i], tail[64 + i]);
            failures++;
            return;
        }
    }

    printf("ok legit conversion\n");
}

/*
 * An empty resource fork (declared length 0 at the fork offset) must keep
 * converting to the canonical layout.
 */
static void expect_empty_rfork(void)
{
    /* FinderInfo declares 64 bytes, so 64 payload bytes must follow the
     * entry table for the sidecar to be well formed; the resource fork
     * declares nothing and sits at the end of the file. The conversion
     * drops the 32 packed-xattr bytes, so the canonical result is
     * ADEDOFF_RFORK_OSX bytes long. */
    const int expected_size = ADEDOFF_RFORK_OSX;
    unsigned char tail[ADEDLEN_FINDERI + 32];
    const struct adent ents[] = {
        { ADEID_RFORK,   ADEDOFF_FINDERI_OSX + ADEDLEN_FINDERI + 32, 0 },
        { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32 },
    };
    unsigned char buf[1024];
    ssize_t got;
    uint32_t off, len;
    size_t i;
    int rc;
    memset(tail, 'F', sizeof(tail));
    write_sidecar(ents, 2, (char *)tail, sizeof(tail));
    rc = open_rf_write();

    if (rc != 0) {
        printf("not ok empty rfork: ad_open returned %d, expected 0\n", rc);
        failures++;
        return;
    }

    if (file_size(scpath) != expected_size) {
        printf("not ok empty rfork: sidecar size %lld, expected %d\n",
               (long long)file_size(scpath), expected_size);
        failures++;
        return;
    }

    got = read_sidecar(buf, sizeof(buf));

    if (got < 0 || (size_t)got < (size_t)expected_size) {
        fail("empty rfork", "cannot read converted sidecar");
        return;
    }

    if (find_entry(buf, got, ADEID_FINDERI, &off, &len) != 0
            || off != ADEDOFF_FINDERI_OSX || len != ADEDLEN_FINDERI) {
        fail("empty rfork", "malformed FinderInfo entry in output");
        return;
    }

    if (find_entry(buf, got, ADEID_RFORK, &off, &len) != 0
            || off != ADEDOFF_RFORK_OSX || len != 0) {
        fail("empty rfork", "malformed resource fork entry in output");
        return;
    }

    for (i = 0; i < ADEDLEN_FINDERI; i++) {
        if (buf[ADEDOFF_FINDERI_OSX + i] != 'F') {
            printf("not ok empty rfork: FinderInfo payload corrupted at %zu\n", i);
            failures++;
            return;
        }
    }

    printf("ok empty rfork conversion\n");
}

/*
 * The declared resource fork length is what the rebuilt header reports.
 * The sidecar below carries 64 FinderInfo bytes (32 canonical + 32 packed
 * xattrs) and declares a 96 byte resource fork; the conversion must move
 * exactly those 96 bytes so that the payload survives and the rebuilt
 * entry matches the data. It also pins down that the 32 packed-xattr bytes
 * are dropped rather than becoming resource fork data.
 */
static void expect_declared_len_wins(void)
{
    const uint32_t rlen = 96;
    const uint32_t roff = ADEDOFF_FINDERI_OSX + 64;
    unsigned char tail[64 + 96];
    const struct adent ents[] = {
        { ADEID_RFORK,   roff, rlen },
        { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32 },
    };
    unsigned char buf[1024];
    ssize_t got;
    uint32_t off, len;
    size_t i;
    int rc;
    /* Layout from the first byte after the entry table: 32 bytes of
     * canonical FinderInfo, 32 bytes of packed xattrs that must be
     * discarded, then the resource fork payload. */
    memset(tail, 'X', 64);

    for (i = 0; i < 32; i++) {
        tail[i] = 'F';
    }

    for (i = 0; i < rlen; i++) {
        tail[64 + i] = (unsigned char)(i % 241 + 1);
    }

    write_sidecar(ents, 2, (char *)tail, sizeof(tail));
    rc = open_rf_write();

    if (rc != 0) {
        printf("not ok declared len: ad_open returned %d, expected 0\n", rc);
        failures++;
        return;
    }

    if (file_size(scpath) != (off_t)(ADEDOFF_RFORK_OSX + rlen)) {
        printf("not ok declared len: sidecar size %lld, expected %u\n",
               (long long)file_size(scpath), ADEDOFF_RFORK_OSX + rlen);
        failures++;
        return;
    }

    got = read_sidecar(buf, sizeof(buf));

    if (got < 0 || (size_t)got < ADEDOFF_RFORK_OSX + rlen) {
        fail("declared len", "cannot read converted sidecar");
        return;
    }

    if (find_entry(buf, got, ADEID_RFORK, &off, &len) != 0
            || off != ADEDOFF_RFORK_OSX || len != rlen) {
        fail("declared len", "resource fork length not taken from the sidecar");
        return;
    }

    for (i = 0; i < rlen; i++) {
        if (buf[ADEDOFF_RFORK_OSX + i] != (unsigned char)(i % 241 + 1)) {
            printf("not ok declared len: resource fork payload corrupted at %zu "
                   "(got 0x%02x, want 0x%02x)\n",
                   i, buf[ADEDOFF_RFORK_OSX + i], (unsigned char)(i % 241 + 1));
            failures++;
            return;
        }
    }

    printf("ok declared resource fork length is honoured\n");
}

/* ad_refresh() normally has no pathname. Force conversion on an already-open
 * sidecar to cover both NULL-safe logging and the metadata-path guard. */
static void expect_null_path_refresh(void)
{
    enum { rlen = 32 };
    unsigned char canonical_tail[ADEDLEN_FINDERI + rlen];
    unsigned char foreign_tail[ADEDLEN_FINDERI + 32 + rlen];
    const struct adent canonical[] = {
        { ADEID_RFORK,   ADEDOFF_RFORK_OSX, rlen },
        { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI },
    };
    const struct adent foreign[] = {
        { ADEID_RFORK,   ADEDOFF_FINDERI_OSX + ADEDLEN_FINDERI + 32, rlen },
        { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32 },
    };
    struct adouble ad;
    unsigned char buf[AD_DATASZ_OSX + rlen];
    size_t i;
    ssize_t got;
    int opened = 0;
    int ok = 0;
    memset(canonical_tail, 'F', ADEDLEN_FINDERI);
    memset(foreign_tail, 'F', ADEDLEN_FINDERI);
    memset(foreign_tail + ADEDLEN_FINDERI, 'X', 32);

    for (i = 0; i < rlen; i++) {
        canonical_tail[ADEDLEN_FINDERI + i] = (unsigned char)(0x80 + i);
        foreign_tail[ADEDLEN_FINDERI + 32 + i] = (unsigned char)(0x80 + i);
    }

    write_sidecar(canonical, 2, (char *)canonical_tail,
                  sizeof(canonical_tail));
    memset(&ad, 0, sizeof(ad));
    ad_init_old(&ad, AD_VERSION_EA, 0);

    if (ad_open(&ad, dfpath,
                ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF | ADFLAGS_RDWR) != 0) {
        fail("NULL-path refresh", "cannot open canonical sidecar");
        return;
    }

    opened = 1;
    write_sidecar(foreign, 2, (char *)foreign_tail, sizeof(foreign_tail));

    if (ad_refresh(NULL, &ad) != 0) {
        fail("NULL-path refresh", "ad_refresh(NULL, ...) failed");
        goto out;
    }

    got = read_sidecar(buf, sizeof(buf));

    if (got != (ssize_t)sizeof(buf)) {
        fail("NULL-path refresh", "converted sidecar has the wrong size");
        goto out;
    }

    for (i = 0; i < rlen; i++) {
        if (buf[ADEDOFF_RFORK_OSX + i] != (unsigned char)(0x80 + i)) {
            fail("NULL-path refresh", "resource fork payload was corrupted");
            goto out;
        }
    }

    ok = 1;
out:

    if (opened) {
        ad_close(&ad, ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF);
    }

    if (ok) {
        printf("ok ad_refresh(NULL, ...) conversion\n");
    }
}

int main(void)
{
    char directory[PATH_MAX];
    char origcwd[PATH_MAX];
    unsigned char tail8k[8192];
    unsigned char tail1m[1024 * 1024];
    char *tmpdirpath;
    int len;
    int df;
#if defined(HAVE_EAFD) || defined(__APPLE__)
    /*
     * The resource fork lives in a native OS extended attribute here, so
     * ad_open_rf() reads it directly and never calls
     * ad_header_read_osx()/ad_convert_osx(). Every vector below -- the
     * rejection cases as well as the conversion cases -- would fail for
     * want of the code under test, so skip the suite rather than report
     * failures that say nothing about the converter.
     *
     * The suite is driven by meson's default "exitcode" protocol, where 77
     * is the skip code (the TAP "1..0 # SKIP" directive only applies with
     * protocol: 'tap').
     */
    printf("ad_convert_osx() test is unsafe or unreachable with the native "
           "resource-fork backend, skipping\n");
    return 77;
#endif
    len = snprintf(directory, sizeof(directory),
                   "%s/netatalk-test-convert-XXXXXX", tmpdir());

    if (len <= 0 || (size_t)len >= sizeof(directory)) {
        fail("setup", "tmpdir() path too long");
        return 1;
    }

    tmpdirpath = mkdtemp(directory);

    if (tmpdirpath == NULL) {
        perror("mkdtemp");
        return 1;
    }

    /* afpd opens forks with the bare filename after moving into the
     * file's directory; ad_convert_osx() derives the data fork name from
     * the sidecar path with the same assumption. Mirror that here. */
    if (getcwd(origcwd, sizeof(origcwd)) == NULL) {
        perror("getcwd");
        return 1;
    }

    if (chdir(tmpdirpath) != 0) {
        perror("chdir");
        return 1;
    }

    snprintf(dfpath, sizeof(dfpath), "victim.bin");
    snprintf(scpath, sizeof(scpath), "%s", ad_path_osx(dfpath, 0));
    snprintf(bakpath, sizeof(bakpath), "sidecar.bak");
    setuplog("default:severe", "/dev/stderr", false);
    /* Data fork the sidecar belongs to. */
    df = open(dfpath, O_RDWR | O_CREAT, 0644);

    if (df < 0) {
        perror("open data fork");
        return 1;
    }

    if (write(df, "victim data\n", 12) != 12) {
        perror("write data fork");
        return 1;
    }

    close(df);
    init_meta();
    /*
     * 1. The advisory's 1 GiB resource fork length. FinderInfo is a valid
     *    64-byte entry at its canonical offset and the resource fork starts
     *    after it at 114, so every earlier precondition passes. The declared
     *    resource fork length is 0x40000000 although only the small fixture
     *    tail follows. On upstream 031f665 the resulting memmove() raised
     *    SIGBUS.
     *    Pins down: source range.
     */
    {
        const struct adent ents[] = {
            { ADEID_RFORK,   ADEDOFF_FINDERI_OSX + 64, 0x40000000u },
            { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32 },
        };
        memset(tail8k, 'F', sizeof(tail8k));
        expect_reject("reject source range past end of sidecar (SIGBUS vector)",
                      REASON_SOURCE, ents, 2, (char *)tail8k, sizeof(tail8k));
    }
    /*
     * 2. FinderInfo at its canonical offset with a non-canonical length
     *    (which is what forces the conversion), resource fork immediately
     *    after that entry, and a declared length of 0xffffffff in a small
     *    sidecar. parse_entries() exempts ADEID_RFORK from its
     *    bounds check, so this length reaches the converter untouched.
     *    Pins down: source range.
     */
    {
        unsigned char tail[ADEDLEN_FINDERI + 32];
        const struct adent ents[] = {
            { ADEID_RFORK,   ADEDOFF_FINDERI_OSX + 64, 0xffffffffu },
            { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32 },
        };
        memset(tail, 'F', sizeof(tail));
        expect_reject("reject declared rfork length 0xffffffff",
                      REASON_SOURCE, ents, 2, (char *)tail, sizeof(tail));
    }
    /*
     * 3. Resource fork offset below its canonical destination. Moving the
     *    fork down from 50 to 82 would run over the header and the
     *    rebuilt entry table.
     *    Pins down: the fork must not start before the canonical
     *    destination.
     */
    {
        const struct adent ents[] = {
            { ADEID_RFORK,   ADEDOFF_FINDERI_OSX, 8142 },
            { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32 },
        };
        memset(tail8k, 'F', sizeof(tail8k));
        expect_reject("reject rfork offset below canonical destination",
                      REASON_ROFF_LOW, ents, 2, (char *)tail8k, sizeof(tail8k));
    }
    /* Because roff >= ADEDOFF_RFORK_OSX is checked first, a valid source
     * range implies a valid destination range. There is intentionally no
     * separate destination-overflow vector for an unreachable branch. */
    /* 4. FinderInfo shorter than the canonical 32 bytes: malformed, must
     *    not be rewritten. The resource fork entry itself fits. */
    {
        unsigned char tail[32];
        const struct adent ents[] = {
            { ADEID_RFORK,   ADEDOFF_RFORK_OSX, 0 },
            { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, 1 },
        };
        memset(tail, 'F', sizeof(tail));
        expect_reject("reject short FinderInfo",
                      REASON_NONE, ents, 2, (char *)tail, sizeof(tail));
    }
    /* 5. No resource fork entry at all: the fork offset is 0, below the
     *    canonical destination. */
    {
        unsigned char tail[76];
        const struct adent ents[] = {
            { ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32 },
        };
        memset(tail, 'F', sizeof(tail));
        expect_reject("reject sidecar without resource fork entry",
                      REASON_NONE, ents, 1, (char *)tail, sizeof(tail));
    }
    /* 6. Non-canonical FinderInfo offset (>= 1024): the header buffer only
     *    holds the first AD_DATASZ_OSX bytes of the sidecar, so the entry
     *    can name bytes that were never read. */
    {
        const struct adent ents[] = {
            { ADEID_RFORK,   ADEDOFF_RFORK_OSX, 512 },
            { ADEID_FINDERI, 4096, ADEDLEN_FINDERI + 32 },
        };
        memset(tail8k, 'F', sizeof(tail8k));
        expect_reject("reject non-canonical FinderInfo offset",
                      REASON_NONE, ents, 2, (char *)tail8k, sizeof(tail8k));
    }
    /* 7. FinderInfo offset far beyond the header buffer on a large
     *    sidecar: valid_data_len covers the whole file, so a pure bounds
     *    check against the file size would accept this. */
    {
        const struct adent ents[] = {
            { ADEID_RFORK,   ADEDOFF_RFORK_OSX, 512 },
            { ADEID_FINDERI, 500000, ADEDLEN_FINDERI + 32 },
        };
        memset(tail1m, 'F', sizeof(tail1m));
        expect_reject("reject FinderInfo offset past header buffer",
                      REASON_NONE, ents, 2, (char *)tail1m, sizeof(tail1m));
    }
    /* 8. A concurrent truncate after fstat() can leave only the 50-byte entry
     *    table for pread(). It must not convert zero-filled FinderInfo. */
    expect_short_header_after_stat();
    /* 9. Legit conversion: 64 byte FinderInfo, resource fork at 114 with
     *    400 payload bytes, all of which must survive. */
    expect_legit_conversion();
    /* 10. An empty resource fork keeps converting to the canonical
     *     layout. */
    expect_empty_rfork();
    /* 11. The declared resource fork length is what the rebuilt header
     *     reports. */
    expect_declared_len_wins();
    /* 12. ad_refresh() reaches conversion without a pathname. */
    expect_null_path_refresh();
    unlink(dfpath);
    unlink(scpath);
    unlink(bakpath);

    if (chdir(origcwd) != 0) {
        perror("chdir back");
    }

    rmdir(tmpdirpath);

    if (failures) {
        printf("%d test(s) failed\n", failures);
        return 1;
    }

    printf("all conversion bounds tests passed\n");
    return 0;
}
