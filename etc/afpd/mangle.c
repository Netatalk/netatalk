/*
 * Copyright (c) 2002 Joe Marcus Clarke (jclarke@marcuscom.com)
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief mangle, demangle (filename)
 *
 * mangle or demangle filenames if they are greater than the max allowed
 * characters for a given version of AFP.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>
#include <stdio.h>

#include <atalk/util.h>

#include "desktop.h"
#include "dircache.h"
#include "mangle.h"
#include "pfd_cache.h"


#define hextoint(c)   (isdigit((unsigned char)(c)) ? (c) - '0' : (c) + 10 - 'A')
#define isuxdigit(x)  (isdigit((unsigned char)(x)) \
                       || (isupper((unsigned char)(x)) \
                           && isxdigit((unsigned char)(x))))

/*!
 * @brief Convert the extension of a name for the mangled form
 *
 * The extension starts at the last dot, which must not be the first byte.
 *
 * @param[in]  vol        volume of the name
 * @param[in]  uname      name on disk
 * @param[out] extension  receives the converted extension, at least
 *                        MAX_EXT_LENGTH + 2 bytes
 * @param[in]  charset    target charset
 * @returns the length of the extension, or 0 without one or when the conversion
 *          fails
 */
static size_t mangle_extension(const struct vol *vol, const char *uname,
                               char *extension, charset_t charset)
{
    char *p = strrchr(uname, '.');

    if (p && p != uname) {
        uint16_t flags = CONV_FORCE | CONV_UNESCAPEHEX;
        size_t len = convert_charset(vol->v_volcharset, charset,
                                     vol->v_maccharset, p, strlen(p),
                                     extension, MAX_EXT_LENGTH + 2, &flags);

        if (len != (size_t) -1) {
            return len;
        }
    }

    return 0;
}

/*!
 * @brief Decide whether a mangled name from the client stands for a name on
 * disk
 *
 * A name with a valid `#ID` can also be a real name, so the extensions must
 * match first. The name on disk must have needed mangling, because it was too
 * long or held unconvertible characters. The prefix that the client sent must
 * then be the name's converted form, or "???" for a name that converts to
 * nothing. Mac OS X may carry the prefix on into the decomposed character
 * the conversion stopped at, so every byte of a longer prefix must start the
 * name.
 *
 * @param[in] vol        volume of the name
 * @param[in] uname      on-disk name of the object that the ID names
 * @param[in] mfilename  mangled name from the client
 * @param[in] prefix     length of the name before MANGLE_CHAR
 * @param[in] ext        extension after the ID
 * @returns uname for a match, or mfilename
 */
static char *demangle_checks(const struct vol *vol, char *uname,
                             char *mfilename, size_t prefix, char *ext)
{
    uint16_t flags;
    /* for convert_charset dest_len parameter +2 */
    static char buffer[MAXPATHLEN + 2];
    size_t len;
    size_t mfilenamelen;
    /* We need to check, whether we really need to demangle the filename 	*/
    /* i.e. it's not just a file with a valid #HEX in the name ...		*/
    /* but we don't want to miss valid demangle as well.			*/
    /* check whether file extensions match */
    /* char buf[] is for convert_charset dest_len parameter +2 */
    char buf[MAX_EXT_LENGTH + 2];
    size_t ext_len = mangle_extension(vol, uname, buf, CH_UTF8_MAC);

    if (ext_len) {
        buf[ext_len] = '\0';

        if (strcmp(ext, buf)) {
            return mfilename;
        }
    } else {
        if (*ext) {
            return mfilename;
        }
    }

    /* First we convert the unix name to our volume maccharset     	*/
    /* This assumes, OSX will not send us a mangled name for *any* 	*/
    /* other reason than a hint/filename mismatch on the OSX side!! */
    /* If the filename needed mangling, we'll get the mac filename	*/
    /* till the first unconvertable char, so these have to	match	*/
    /* the mangled name we got ..					*/
    flags = CONV_IGNORE | CONV_UNESCAPEHEX;

    if ((size_t) -1 == (len = convert_charset(vol->v_volcharset, vol->v_maccharset,
                                              0, uname, strlen(uname), buffer, sizeof(buffer), &flags))) {
        return mfilename;
    }

    /* If the filename is too long we also needed to mangle */
    mfilenamelen = strlen(mfilename);

    if (len >= vol->max_filename || mfilenamelen == MACFILELEN) {
        flags |= CONV_REQMANGLE;
        len = prefix;
    }

    /* Ok, mangling was needed, now we do some further checks    */
    /* this is still necessary, as we might have a file abcde:xx */
    /* with id 12, mangled to abcde#12, and a passed filename    */
    /* abcd#12 						     */
    /* if we only checked if "prefix" number of characters match */
    /* we get a false postive in above case			     */

    if (flags & CONV_REQMANGLE) {
        if (len) {
            /* convert the buffer to UTF8_MAC ... */
            if ((size_t) -1 == (len = convert_charset(vol->v_maccharset, CH_UTF8_MAC, 0,
                                                      buffer, len, buffer, sizeof(buffer), &flags))) {
                return mfilename;
            }

            /* the prefix holds the converted start, all of it unless a name
             * of MACFILELEN bytes cut it */
            if ((prefix < len && mfilenamelen != MACFILELEN)
                    || strncmp(mfilename, buffer, len) != 0) {
                return mfilename;
            }

            if (prefix <= len) {
                return uname;
            }
        } else if (prefix == 3 && strncmp(mfilename, "???", 3) == 0) {
            /* mangle() writes "???" for a name that converts to nothing */
            return uname;
        }

        if (prefix > 0
                && convert_charset(vol->v_volcharset, CH_UTF8_MAC, 0, uname,
                                   strlen(uname), buffer, sizeof(buffer),
                                   &flags) != (size_t) -1
                && strncmp(mfilename, buffer, prefix) == 0) {
            return uname;
        }
    }

    return mfilename;
}

/*!
 * @brief The CNID that a MANGLE_CHAR of a name introduces
 *
 * The CNID is at most eight uppercase hex digits without a leading zero, at
 * least CNID_START, and ends the name or comes before a '.' that starts an
 * extension of at most MAX_EXT_LENGTH bytes.
 *
 * @param[in]  hash  a MANGLE_CHAR of the name
 * @param[out] ext   receives the end of the CNID's digits
 * @returns the CNID in host order, or 0 when no CNID follows hash
 */
static uint32_t mangled_id(char *hash, char **ext)
{
    char *t = hash + 1;
    uint32_t id = 0;

    if (*t == '0') {
        return 0;
    }

    /* a ninth digit would wrap the id into another object's */
    while (t - hash < MANGLE_LENGTH && isuxdigit(*t)) {
        id = (id * 16) + hextoint(*t);
        t++;
    }

    if ((*t != 0 && *t != '.') || strlen(t) > MAX_EXT_LENGTH
            || id < CNID_START) {
        return 0;
    }

    *ext = t;
    return id;
}

/*!
 * @brief Find the on-disk name that a mangled name in a directory stands for
 *
 * A mangled name is a prefix of the object's name, MANGLE_CHAR, the CNID and
 * the name's extension. The prefix and the extension may hold MANGLE_CHAR as
 * well, so each MANGLE_CHAR that introduces a CNID gets a turn, the last
 * first. The object must sit in did; a cached object whose name still holds
 * it answers from its dircache entry, any other ID from one CNID lookup. The
 * name must equal the mangled name the server builds; with osx,
 * demangle_checks also accepts the forms that Mac OS X sends.
 *
 * @param[in]  vol        volume of the name
 * @param[in]  mfilename  name from the client
 * @param[in]  did        CNID of the directory that holds the object
 * @param[out] osx        receives the CNID in network order with the name on
 *                        disk returned; NULL asks for the exact match alone
 * @returns the name on disk in a static buffer, or mfilename when it is not a
 *          mangled name of an object in did
 */
static char *
private_demangle(const struct vol *vol, char *mfilename, cnid_t did,
                 cnid_t *osx)
{
    char *hash = mfilename + strlen(mfilename);
    /* MANGLE_CHAR, the CNID's digits and the extension end a mangled name */
    const char *first = hash - MIN(hash - mfilename,
                                   MANGLE_LENGTH + MAX_EXT_LENGTH);
    char *ext = NULL;
    char *u_name;
    uint32_t id;
    cnid_t pdid;
    static char buffer[CNID_MAX_PATH_LEN];
    struct dir *dir;
    struct stat st;
    size_t prefix;

    while (hash > first) {
        if (*--hash != MANGLE_CHAR || (id = mangled_id(hash, &ext)) == 0) {
            continue;
        }

        prefix = hash - mfilename;
        id = htonl(id);
        pdid = id;

        /* another session's rename reaches the entry only by its cache
         * hint, and a directory's ctime moves with each change inside it */
        if ((dir = dircache_lookup_did(vol, id)) != NULL
                && pfd_ostat(vol, dir, &st, vol_syml_opt(vol)) == 0
                && st.st_ino == dir->dcache_ino
                && (!(dir->d_flags & DIRF_ISFILE)
                    || st.st_ctime == dir->dcache_ctime)) {
            if (dir->d_pdid != did) {
                continue;
            }

            /* a rename of the object frees the entry's name */
            strlcpy(buffer, cfrombstr(dir->d_u_name), sizeof(buffer));
            u_name = buffer;
        } else if ((u_name = cnid_resolve(vol->v_cdb, &pdid, buffer,
                                          sizeof(buffer))) == NULL
                   || pdid != did) {
            continue;
        }

        /* an AFP 3 client (osx) may also send a form Mac OS X builds */
        if (strcmp(utompath(vol, u_name, id, utf8_encoding(vol->v_obj)),
                   mfilename) == 0
                || (osx != NULL
                    && demangle_checks(vol, u_name, mfilename, prefix,
                                       ext) == u_name)) {
            if (osx != NULL) {
                *osx = id;
            }

            return u_name;
        }
    }

    return mfilename;
}

/*!
 * @brief Resolve a mangled name in a directory, accepting only the exact form
 * the server builds
 *
 * @param[in] vol        volume of the name
 * @param[in] mfilename  name from the client
 * @param[in] did        CNID of the directory that holds the object
 * @returns the name on disk, or mfilename
 */
char *
demangle(const struct vol *vol, char *mfilename, cnid_t did)
{
    return private_demangle(vol, mfilename, did, NULL);
}

/*!
 * @brief Resolve a mangled name in a directory, also accepting the forms that
 * Mac OS X sends
 *
 * @param[in]  vol        volume of the name
 * @param[in]  mfilename  name from the client
 * @param[in]  did        CNID of the directory that holds the object
 * @param[out] fileid     receives the CNID, in network order, with the name
 *                        a mangled form resolves to
 * @returns the name on disk, or mfilename
 */
char *
demangle_osx(const struct vol *vol, char *mfilename, cnid_t did, cnid_t *fileid)
{
    return private_demangle(vol, mfilename, did, fileid);
}

/*!
 * @brief Shorten or replace a filename the client cannot take, tagging it with
 * the CNID
 *
 * A name over the client's limit keeps a prefix of its UTF-8 or Mac charset
 * form. MANGLE_CHAR, the CNID in hexadecimal, and the extension that
 * mangle_extension() found follow the prefix. The result lives in the static
 * buffer of its encoding, which bit 1 of flags chooses, since a reply packs
 * both names of one object.
 *
 * @bug Early Mac OS X (10.0 to 10.4) limited names to 255 bytes. The conversion
 * therefore runs volcharset to UTF16-MAC to UTF8-MAC truncated at 255 bytes.
 * Later releases take 510 bytes of UTF16-MAC, which would allow volcharset to
 * truncated UTF16-MAC to UTF8-MAC.
 * @note A UTF-8 filename does not always round-trip.
 *
 * @param[in] vol          volume the file belongs to
 * @param[in] filename     Mac name, too long or holding characters the client
 *                         cannot map
 * @param[in] filenamelen  length of filename in bytes
 * @param[in] uname        Unix name of the file
 * @param[in] id           CNID of the file, or 0
 * @param[in] flags        bit 0 forces mangling, bit 1 selects the UTF-8 length
 *                         limit and buffer
 * @returns filename when it fits, NULL when id is 0, or the mangled name
 */
char *
mangle(const struct vol *vol, char *filename, size_t filenamelen, char *uname,
       cnid_t id, int flags)
{
    char *m = NULL;
    /* way > maxlen */
    static char mfilename[2][MAXPATHLEN + 2];
    char mangle_suffix[MANGLE_LENGTH + 1];
    /* for convert_charset dest_len parameter +2 */
    char ext[MAX_EXT_LENGTH + 2];
    size_t ext_len;
    size_t maxlen;
    int k;
    /* was vol->max_filename */
    maxlen = (flags & 2) ? UTF8FILELEN_EARLY : MACFILELEN;

    /* Do we really need to mangle this filename? */
    if (!(flags & 1) && filenamelen <= maxlen) {
        return filename;
    }

    if (!id) {
        /* we don't have the file id! only catsearch call mangle with id == 0 */
        return NULL;
    }

    /* First, attempt to locate a file extension. */
    ext_len = mangle_extension(vol, uname, ext,
                               (flags & 2) ? CH_UTF8_MAC : vol->v_maccharset);
    m = mfilename[(flags & 2) != 0];
    k = snprintf(mangle_suffix, sizeof(mangle_suffix), "%c%X", MANGLE_CHAR,
                 ntohl(id));

    if (filenamelen + k + ext_len > maxlen) {
        uint16_t opt = CONV_FORCE | CONV_UNESCAPEHEX;
        size_t prefix_len = maxlen - k - ext_len;
        size_t n = convert_charset(vol->v_volcharset,
                                   (flags & 2) ? CH_UTF8_MAC : vol->v_maccharset,
                                   vol->v_maccharset, uname, strlen(uname),
                                   m, prefix_len + 2, &opt);
        m[n != (size_t) -1 ? n : 0] = 0;
    } else {
        strlcpy(m, filename, filenamelen + 1);
    }

    if (*m == 0) {
        strlcat(m, "???", MAXPATHLEN);
    }

    strlcat(m, mangle_suffix, MAXPATHLEN);

    if (ext_len) {
        strncat(m, ext, ext_len);
    }

    return m;
}
