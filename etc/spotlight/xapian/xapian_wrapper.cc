/*
 * Xapian implementation for the Netatalk Spotlight backend.
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "etc/spotlight/xapian/sl_xapian.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

#include <xapian.h>
#include <magic.h>

namespace
{

const Xapian::valueno VALUE_PATH = 0;
const Xapian::valueno VALUE_TYPE = 1;
const Xapian::valueno VALUE_MTIME = 2;
const Xapian::valueno VALUE_SIZE = 3;
const Xapian::valueno VALUE_MIME = 4;
const Xapian::valueno VALUE_GENERATION = 5;
const size_t TEXT_MAX_BYTES = 16 * 1024 * 1024;
const size_t TEXT_SAMPLE_BYTES = 4096;
const char METADATA_SCHEMA_VERSION[] = "netatalk.schema_version";
const char METADATA_VOLUME_UUID[] = "netatalk.volume_uuid";
const char METADATA_VOLUME_PATH[] = "netatalk.volume_path";
const char METADATA_LAST_COMPLETE_GENERATION[] =
    "netatalk.last_complete_generation";
const char METADATA_RECONCILE_IN_PROGRESS[] =
    "netatalk.reconcile_in_progress";
const char METADATA_DIRTY[] = "netatalk.dirty";
const char INDEX_SCHEMA_VERSION[] = "1";

class XapianTransaction
{
public:
    explicit XapianTransaction(Xapian::WritableDatabase &db) : db(db)
    {
        db.begin_transaction();
        active = true;
    }

    ~XapianTransaction()
    {
        if (active) {
            try {
                db.cancel_transaction();
            } catch (...) {
                // Destructors must not throw.
            }
        }
    }

    XapianTransaction(const XapianTransaction &) = delete;
    XapianTransaction &operator=(const XapianTransaction &) = delete;

    void commit()
    {
        db.commit_transaction();
        active = false;
    }

    void cancel()
    {
        db.cancel_transaction();
        active = false;
    }

private:
    Xapian::WritableDatabase &db;
    bool active = false;
};

class MagicCookie
{
public:
    MagicCookie() = default;
    ~MagicCookie()
    {
        if (cookie != nullptr) {
            magic_close(cookie);
        }
    }

    MagicCookie(const MagicCookie &) = delete;
    MagicCookie &operator=(const MagicCookie &) = delete;

    bool open(std::string &errmsg)
    {
        cookie = magic_open(MAGIC_MIME_TYPE);

        if (cookie == nullptr) {
            errmsg = "magic_open failed";
            return false;
        }

        if (magic_load(cookie, nullptr) != 0) {
            errmsg = magic_error(cookie) ? magic_error(cookie) : "magic_load failed";
            magic_close(cookie);
            cookie = nullptr;
            return false;
        }

        return true;
    }

    magic_t get() const
    {
        return cookie;
    }

private:
    magic_t cookie = nullptr;
};

void set_error(char *errbuf, size_t errlen, const std::string &msg)
{
    if (errbuf == nullptr || errlen == 0) {
        return;
    }

    snprintf(errbuf, errlen, "%s", msg.c_str());
}

bool mkdir_one(const std::string &path, std::string &errmsg)
{
    if (path.empty()) {
        return true;
    }

    if (mkdir(path.c_str(), 0775) == 0) {
        return true;
    }

    int saved_errno = errno;

    if (saved_errno == EEXIST) {
        struct stat st;

        if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            return true;
        }

        if (stat(path.c_str(), &st) != 0) {
            saved_errno = errno;
            errmsg = path + ": " + strerror(saved_errno);
        } else {
            errmsg = path + ": exists but is not a directory";
        }

        return false;
    }

    errmsg = path + ": " + strerror(saved_errno);
    return false;
}

bool mkdir_p(const std::string &path, std::string &errmsg)
{
    std::string partial;

    if (path.empty()) {
        errmsg = "empty path";
        return false;
    }

    if (path[0] == '/') {
        partial = "/";
    }

    size_t start = (path[0] == '/') ? 1 : 0;

    while (start < path.size()) {
        size_t slash = path.find('/', start);

        if (std::string part = path.substr(start, slash - start); !part.empty()) {
            if (partial.size() > 1 && partial[partial.size() - 1] != '/') {
                partial += '/';
            }

            partial += part;

            if (!mkdir_one(partial, errmsg)) {
                return false;
            }
        }

        if (slash == std::string::npos) {
            break;
        }

        start = slash + 1;
    }

    return true;
}

std::string dirname_of(const std::string &path)
{
    size_t slash = path.rfind('/');

    if (slash == std::string::npos) {
        return ".";
    }

    if (slash == 0) {
        return "/";
    }

    return path.substr(0, slash);
}

std::string basename_of(const std::string &path)
{
    size_t slash = path.rfind('/');

    if (slash == std::string::npos) {
        return path;
    }

    return path.substr(slash + 1);
}

bool metadata_name(const char *name)
{
    return strcmp(name, ".") == 0
           || strcmp(name, "..") == 0
           || strcmp(name, ".AppleDB") == 0
           || strcmp(name, ".AppleDesktop") == 0
           || strcmp(name, ".AppleDouble") == 0
           || strcmp(name, ".Parent") == 0
           || strncmp(name, "._", 2) == 0;
}

bool path_in_scope(const std::string &path, const std::string &scope)
{
    if (scope.empty() || scope == "/") {
        return true;
    }

    if (path == scope) {
        return true;
    }

    return path.size() > scope.size()
           && path.compare(0, scope.size(), scope) == 0
           && path[scope.size()] == '/';
}

std::string fnv1a_hex(const std::string &s)
{
    uint64_t hash = 1469598103934665603ULL;
    char buf[32];

    for (char c : s) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ULL;
    }

    snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(hash));
    return std::string(buf);
}

std::string unique_term_for_path(const std::string &path)
{
    return std::string("Q") + fnv1a_hex(path);
}

std::string number_value(uint64_t n)
{
    std::ostringstream os;
    os << n;
    return os.str();
}

bool parse_uint64_value(const std::string &s, uint64_t &out)
{
    if (s.empty()) {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    unsigned long long value = strtoull(s.c_str(), &end, 10);

    if (errno != 0 || end == s.c_str() || *end != '\0') {
        return false;
    }

    out = static_cast<uint64_t>(value);
    return true;
}

std::string next_generation_id(const Xapian::Database &db)
{
    uint64_t last = 0;
    uint64_t in_progress = 0;
    parse_uint64_value(db.get_metadata(METADATA_LAST_COMPLETE_GENERATION), last);
    parse_uint64_value(db.get_metadata(METADATA_RECONCILE_IN_PROGRESS),
                       in_progress);
    return number_value(std::max(last, in_progress) + 1);
}

bool truthy_metadata(const std::string &value)
{
    return value == "1" || value == "true" || value == "yes";
}

bool index_metadata_valid(const Xapian::Database &db,
                          const std::string &volume_path,
                          const std::string &volume_uuid)
{
    if (db.get_metadata(METADATA_SCHEMA_VERSION) != INDEX_SCHEMA_VERSION) {
        return false;
    }

    if (db.get_metadata(METADATA_VOLUME_PATH) != volume_path) {
        return false;
    }

    if (!volume_uuid.empty()
            && db.get_metadata(METADATA_VOLUME_UUID) != volume_uuid) {
        return false;
    }

    if (db.get_metadata(METADATA_LAST_COMPLETE_GENERATION).empty()) {
        return false;
    }

    if (!db.get_metadata(METADATA_RECONCILE_IN_PROGRESS).empty()) {
        return false;
    }

    return !truthy_metadata(db.get_metadata(METADATA_DIRTY));
}

void set_complete_metadata(Xapian::WritableDatabase &db,
                           const std::string &volume_path,
                           const std::string &volume_uuid,
                           const std::string &generation)
{
    db.set_metadata(METADATA_SCHEMA_VERSION, INDEX_SCHEMA_VERSION);
    db.set_metadata(METADATA_VOLUME_UUID, volume_uuid);
    db.set_metadata(METADATA_VOLUME_PATH, volume_path);
    db.set_metadata(METADATA_LAST_COMPLETE_GENERATION, generation);
    db.set_metadata(METADATA_RECONCILE_IN_PROGRESS, "");
    db.set_metadata(METADATA_DIRTY, "0");
}

std::string lower_string(const std::string &s)
{
    std::string out = s;

    for (char &c : out) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }

    return out;
}

std::string trim_mime(const std::string &mime)
{
    size_t start = 0;
    size_t end = mime.find(';');
    std::string s = lower_string(mime.substr(0, end));

    while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) {
        start++;
    }

    end = s.size();

    while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) {
        end--;
    }

    return s.substr(start, end - start);
}

template <size_t N>
bool has_suffix(const std::string &s, const char (&suffix)[N])
{
    return s.size() >= N - 1
           && s.compare(s.size() - (N - 1), N - 1, suffix) == 0;
}

template <size_t N>
bool starts_with(const std::string &s, const char (&prefix)[N])
{
    return s.compare(0, N - 1, prefix) == 0;
}

std::string exact_term(const char *prefix, const std::string &value)
{
    return std::string(prefix) + lower_string(value);
}

void add_exact_term(Xapian::Document &doc,
                    const char *prefix,
                    const std::string &value)
{
    if (!value.empty()) {
        doc.add_term(exact_term(prefix, value));
    }
}

void add_category_term(Xapian::Document &doc, const char *category)
{
    add_exact_term(doc, "XK", category);
}

bool source_mime_or_name(const std::string &mime,
                         const std::string &lower_path)
{
    static const char *source_mimes[] = {
        "application/ecmascript",
        "application/javascript",
        "application/json",
        "application/x-csh",
        "application/x-perl",
        "application/x-php",
        "application/x-python",
        "application/x-ruby",
        "application/x-shellscript",
        "application/x-sh",
        "application/x-tcl",
        "text/css",
        "text/ecmascript",
        "text/javascript",
        "text/x-asm",
        "text/x-c",
        "text/x-c++",
        "text/x-cmake",
        "text/x-diff",
        "text/x-java",
        "text/x-makefile",
        "text/x-objcsrc",
        "text/x-patch",
        "text/x-python",
        "text/x-script",
        "text/x-shellscript",
        nullptr
    };

    for (size_t i = 0; source_mimes[i] != nullptr; i++) {
        if (mime == source_mimes[i]) {
            return true;
        }
    }

    return has_suffix(lower_path, ".c")
           || has_suffix(lower_path, ".cc")
           || has_suffix(lower_path, ".cpp")
           || has_suffix(lower_path, ".cxx")
           || has_suffix(lower_path, ".h")
           || has_suffix(lower_path, ".hh")
           || has_suffix(lower_path, ".hpp")
           || has_suffix(lower_path, ".java")
           || has_suffix(lower_path, ".js")
           || has_suffix(lower_path, ".json")
           || has_suffix(lower_path, ".m")
           || has_suffix(lower_path, ".mm")
           || has_suffix(lower_path, ".php")
           || has_suffix(lower_path, ".pl")
           || has_suffix(lower_path, ".py")
           || has_suffix(lower_path, ".rb")
           || has_suffix(lower_path, ".sh")
           || has_suffix(lower_path, ".swift");
}

bool presentation_mime(const std::string &mime)
{
    return mime == "application/vnd.ms-powerpoint"
           || mime == "application/vnd.oasis.opendocument.presentation"
           || mime ==
    "application/vnd.openxmlformats-officedocument.presentationml.presentation"
           || mime == "application/x-iwork-keynote-sffkey";
}

bool font_mime(const std::string &mime)
{
    return starts_with(mime, "font/")
           || starts_with(mime, "application/font")
           || starts_with(mime, "application/x-font")
           || mime == "application/vnd.ms-fontobject";
}

bool executable_mime(const std::string &mime)
{
    return mime == "application/x-executable"
           || mime == "application/x-mach-binary"
           || mime == "application/x-pie-executable"
           || mime == "application/x-sharedlib"
           || mime == "application/x-dosexec"
           || mime == "application/vnd.microsoft.portable-executable";
}

bool document_mime(const std::string &mime)
{
    return starts_with(mime, "text/")
           || mime == "application/msword"
           || mime == "application/pdf"
           || mime == "application/rtf"
           || mime == "application/vnd.oasis.opendocument.text"
           || mime ==
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
           || mime == "application/xhtml+xml"
           || mime == "application/xml"
           || presentation_mime(mime);
}

std::string detect_mime(magic_t magic,
                        const std::string &path,
                        const struct stat &st)
{
    if (S_ISDIR(st.st_mode)) {
        return "inode/directory";
    }

    if (!S_ISREG(st.st_mode) || access(path.c_str(), R_OK) != 0) {
        return "";
    }

    const char *mime = magic != nullptr ? magic_file(magic, path.c_str()) : nullptr;

    if (mime == nullptr || mime[0] == '\0') {
        return "application/octet-stream";
    }

    return trim_mime(mime);
}

void index_document_mime(Xapian::Document &doc,
                         const std::string &path,
                         const struct stat &st,
                         magic_t magic)
{
    std::string mime = detect_mime(magic, path, st);
    std::string lower_path = lower_string(path);

    if (!mime.empty()) {
        doc.add_value(VALUE_MIME, mime);
        add_exact_term(doc, "XM", mime);
    }

    if (S_ISDIR(st.st_mode)) {
        add_category_term(doc, "folder");

        if (has_suffix(lower_path, ".app")) {
            add_category_term(doc, "software");
        }

        return;
    }

    if (starts_with(mime, "image/")) {
        add_category_term(doc, "image");
    }

    if (starts_with(mime, "audio/")) {
        add_category_term(doc, "audio");
    }

    if (starts_with(mime, "video/")) {
        add_category_term(doc, "video");
    }

    if (starts_with(mime, "text/")) {
        add_category_term(doc, "text");
    }

    if (mime == "application/pdf") {
        add_category_term(doc, "pdf");
    }

    if (source_mime_or_name(mime, lower_path)) {
        add_category_term(doc, "source");
        add_category_term(doc, "text");
    }

    if (executable_mime(mime) || (S_ISREG(st.st_mode) && (st.st_mode & 0111))) {
        add_category_term(doc, "executable");
        add_category_term(doc, "software");
    }

    if (font_mime(mime)) {
        add_category_term(doc, "font");
    }

    if (presentation_mime(mime)) {
        add_category_term(doc, "presentation");
    }

    if (document_mime(mime) || source_mime_or_name(mime, lower_path)) {
        add_category_term(doc, "document");
    }
}

bool read_text_file(const std::string &path,
                    const struct stat &st,
                    std::string &text)
{
    if (!S_ISREG(st.st_mode) || st.st_size <= 0
            || static_cast<uint64_t>(st.st_size) > TEXT_MAX_BYTES
            || (st.st_mode & S_IROTH) == 0) {
        return false;
    }

    int fd = open(path.c_str(),
                  O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);

    if (fd < 0) {
        return false;
    }

    if (struct stat fst; fstat(fd, &fst) != 0
            || !S_ISREG(fst.st_mode)
            || fst.st_dev != st.st_dev
            || fst.st_ino != st.st_ino
            || fst.st_size != st.st_size) {
        close(fd);
        return false;
    }

    std::vector<char> buf(static_cast<size_t>(st.st_size));
    size_t off = 0;

    while (off < buf.size()) {
        ssize_t n = read(fd, &buf[off], buf.size() - off);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }

            close(fd);
            return false;
        }

        if (n == 0) {
            break;
        }

        off += static_cast<size_t>(n);
    }

    close(fd);

    if (memchr(buf.data(), '\0', std::min(off, TEXT_SAMPLE_BYTES)) != nullptr) {
        return false;
    }

    if (memchr(buf.data(), '\0', off) != nullptr) {
        return false;
    }

    text.assign(buf.data(), off);
    return true;
}

void index_document_text(const Xapian::Document &doc,
                         const std::string &path,
                         const struct stat &st)
{
    std::string name = basename_of(path);
    Xapian::TermGenerator tg;
    tg.set_document(doc);
    tg.index_text(name, 10, "S");

    if (S_ISREG(st.st_mode)) {
        std::string text;

        if (read_text_file(path, st, text)) {
            tg.increase_termpos();
            tg.index_text(text, 1, "B");
        }
    }
}

void upsert_document(Xapian::WritableDatabase &db,
                     const std::string &path,
                     const struct stat &st,
                     magic_t magic,
                     const std::string *generation = nullptr)
{
    if (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)) {
        db.delete_document(unique_term_for_path(path));
        return;
    }

    Xapian::Document doc;
    std::string unique = unique_term_for_path(path);
    doc.add_term(unique);
    doc.set_data(path);
    doc.add_value(VALUE_PATH, path);
    doc.add_value(VALUE_TYPE, S_ISDIR(st.st_mode) ? "d" : "f");
    doc.add_value(VALUE_MTIME, number_value(static_cast<uint64_t>(st.st_mtime)));
    doc.add_value(VALUE_SIZE, number_value(static_cast<uint64_t>(st.st_size)));

    if (generation != nullptr) {
        doc.add_value(VALUE_GENERATION, *generation);
    }

    index_document_mime(doc, path, st, magic);
    index_document_text(doc, path, st);
    db.replace_document(unique, doc);
}

void preserve_existing_documents(Xapian::WritableDatabase &db,
                                 const std::string &scope,
                                 const std::string &generation)
{
    Xapian::docid last = db.get_lastdocid();

    for (Xapian::docid id = 1; id <= last; id++) {
        try {
            Xapian::Document doc = db.get_document(id);

            if (std::string path = doc.get_value(VALUE_PATH);
                    !path.empty() && path_in_scope(path, scope)) {
                doc.add_value(VALUE_GENERATION, generation);
                db.replace_document(id, doc);
            }
        } catch (const Xapian::DocNotFoundError &) {
            // gaps are normal in Xapian docid sequences; skip silently
        }
    }
}

bool walk_volume(Xapian::WritableDatabase &db,
                 const std::string &path,
                 dev_t volume_dev,
                 magic_t magic,
                 const std::string &generation,
                 std::string &errmsg)
{
    struct stat st;

    if (lstat(path.c_str(), &st) != 0) {
        preserve_existing_documents(db, path, generation);
        return true;
    }

    if (st.st_dev != volume_dev) {
        return true;
    }

    upsert_document(db, path, st, magic, &generation);

    if (!S_ISDIR(st.st_mode)) {
        return true;
    }

    DIR *dir = opendir(path.c_str());

    if (dir == nullptr) {
        preserve_existing_documents(db, path, generation);
        return true;
    }

    const struct dirent *de;

    for (;;) {
        errno = 0;
        de = readdir(dir);

        if (de == nullptr) {
            break;
        }

        if (metadata_name(de->d_name)) {
            continue;
        }

        std::string child = path;

        if (!child.empty() && child[child.size() - 1] != '/') {
            child += '/';
        }

        child += de->d_name;

        if (!walk_volume(db, child, volume_dev, magic, generation, errmsg)) {
            closedir(dir);
            return false;
        }
    }

    if (errno != 0) {
        preserve_existing_documents(db, path, generation);
        closedir(dir);
        return true;
    }

    closedir(dir);
    return true;
}

bool walk_subtree(Xapian::WritableDatabase &db,
                  const std::string &path,
                  dev_t volume_dev,
                  magic_t magic,
                  const std::string *generation,
                  std::string &errmsg)
{
    struct stat st;

    if (lstat(path.c_str(), &st) != 0) {
        errmsg = path + ": " + strerror(errno);
        return false;
    }

    if (st.st_dev != volume_dev) {
        return true;
    }

    upsert_document(db, path, st, magic, generation);

    if (!S_ISDIR(st.st_mode)) {
        return true;
    }

    DIR *dir = opendir(path.c_str());

    if (dir == nullptr) {
        errmsg = path + ": " + strerror(errno);
        return false;
    }

    const struct dirent *de;

    for (;;) {
        errno = 0;
        de = readdir(dir);

        if (de == nullptr) {
            break;
        }

        if (metadata_name(de->d_name)) {
            continue;
        }

        std::string child = path;

        if (!child.empty() && child[child.size() - 1] != '/') {
            child += '/';
        }

        child += de->d_name;

        if (!walk_subtree(db, child, volume_dev, magic, generation, errmsg)) {
            closedir(dir);
            return false;
        }
    }

    if (errno != 0) {
        errmsg = path + ": " + strerror(errno);
        closedir(dir);
        return false;
    }

    closedir(dir);
    return true;
}

void delete_stale_documents(Xapian::WritableDatabase &db,
                            const std::string &volume_path,
                            const std::string &generation)
{
    Xapian::docid last = db.get_lastdocid();

    for (Xapian::docid id = 1; id <= last; id++) {
        try {
            Xapian::Document doc = db.get_document(id);

            if (std::string path = doc.get_value(VALUE_PATH);
                    !path.empty()
                    && path_in_scope(path, volume_path)
                    && doc.get_value(VALUE_GENERATION) != generation) {
                db.delete_document(id);
            }
        } catch (const Xapian::DocNotFoundError &) {
            // gaps are normal in Xapian docid sequences; skip silently
        }
    }
}

void delete_prefix_documents(Xapian::WritableDatabase &db,
                             const std::string &prefix)
{
    if (prefix.empty()) {
        return;
    }

    Xapian::docid last = db.get_lastdocid();

    for (Xapian::docid id = 1; id <= last; id++) {
        try {
            if (std::string docpath = db.get_document(id).get_value(VALUE_PATH);
                    docpath == prefix
                    || (docpath.size() > prefix.size()
                        && docpath.compare(0, prefix.size(), prefix) == 0
                        && docpath[prefix.size()] == '/')) {
                db.delete_document(id);
            }
        } catch (const Xapian::DocNotFoundError &) {
            // gaps are normal in Xapian docid sequences; skip silently
        }
    }
}

void chmod_database_path(const std::string &db_path);

void mark_dirty(Xapian::WritableDatabase &db)
{
    db.set_metadata(METADATA_DIRTY, "1");
}

std::string strip_wildcards(const std::string &s)
{
    size_t start = 0;
    size_t end = s.size();

    while (start < end && s[start] == '*') {
        start++;
    }

    while (end > start && s[end - 1] == '*') {
        end--;
    }

    return s.substr(start, end - start);
}

void collect_values_after_key(const std::string &q,
                              const char *key,
                              std::vector<std::string> &values)
{
    size_t pos = 0;
    std::string needle(key);

    while ((pos = q.find(needle, pos)) != std::string::npos) {
        if (size_t after_key = pos + needle.size();
                after_key < q.size()
                && (isalnum(static_cast<unsigned char>(q[after_key]))
                    || q[after_key] == '_')) {
            pos = after_key;
            continue;
        }

        size_t quote1 = q.find('"', pos + needle.size());

        if (quote1 == std::string::npos) {
            break;
        }

        size_t quote2 = q.find('"', quote1 + 1);

        if (quote2 == std::string::npos) {
            break;
        }

        if (std::string value = strip_wildcards(q.substr(quote1 + 1,
                                                quote2 - quote1 - 1)); !value.empty()) {
            values.push_back(value);
        }

        pos = quote2 + 1;
    }
}

std::vector<std::string> collect_any_values(const std::string &q)
{
    std::vector<std::string> values;
    size_t pos = 0;

    while ((pos = q.find("*==", pos)) != std::string::npos) {
        size_t quote1 = q.find('"', pos + 3);

        if (quote1 == std::string::npos) {
            break;
        }

        size_t quote2 = q.find('"', quote1 + 1);

        if (quote2 == std::string::npos) {
            break;
        }

        if (std::string value = strip_wildcards(q.substr(quote1 + 1,
                                                quote2 - quote1 - 1)); !value.empty()) {
            values.push_back(value);
        }

        pos = quote2 + 1;
    }

    return values;
}

Xapian::Query parse_prefixed(const Xapian::Database &db,
                             const std::string &term,
                             const std::string &prefix)
{
    Xapian::QueryParser qp;
    qp.set_database(db);
    qp.set_default_op(Xapian::Query::OP_AND);
    unsigned flags = Xapian::QueryParser::FLAG_WILDCARD
                     | Xapian::QueryParser::FLAG_PARTIAL;
    return qp.parse_query(term, flags, prefix);
}

void append_mime_term(std::vector<std::string> &terms, const char *mime)
{
    terms.push_back(exact_term("XM", mime));
}

void append_category_term(std::vector<std::string> &terms,
                          const char *category)
{
    terms.push_back(exact_term("XK", category));
}

void append_type_terms_for_value(const std::string &raw_value,
                                 std::vector<std::string> &terms)
{
    std::string value = lower_string(raw_value);

    if (value.find('/') != std::string::npos) {
        append_mime_term(terms, value.c_str());
        return;
    }

    if (value == "9" || value == "public.folder") {
        append_category_term(terms, "folder");
    } else if (value == "13" || value == "public.image") {
        append_category_term(terms, "image");
    } else if (value == "10" || value == "public.audio") {
        append_category_term(terms, "audio");
    } else if (value == "7" || value == "public.movie" || value == "public.video") {
        append_category_term(terms, "video");
    } else if (value == "8") {
        append_category_term(terms, "executable");
    } else if (value == "12") {
        append_category_term(terms, "presentation");
    } else if (value == "4") {
        append_category_term(terms, "font");
    } else if (value == "public.content") {
        append_category_term(terms, "document");
    } else if (value == "public.text") {
        append_category_term(terms, "text");
    } else if (value == "public.source-code") {
        append_category_term(terms, "source");
    } else if (value == "com.apple.application") {
        append_category_term(terms, "software");
    } else if (value == "11" || value == "com.adobe.pdf") {
        append_mime_term(terms, "application/pdf");
    } else if (value == "public.jpeg") {
        append_mime_term(terms, "image/jpeg");
    } else if (value == "public.tiff") {
        append_mime_term(terms, "image/tiff");
    } else if (value == "com.compuserve.gif") {
        append_mime_term(terms, "image/gif");
    } else if (value == "public.png") {
        append_mime_term(terms, "image/png");
    } else if (value == "com.microsoft.bmp") {
        append_mime_term(terms, "image/bmp");
        append_mime_term(terms, "image/x-ms-bmp");
    } else if (value == "public.mp3") {
        append_mime_term(terms, "audio/mpeg");
    } else if (value == "public.mpeg-4-audio") {
        append_mime_term(terms, "audio/aac");
        append_mime_term(terms, "audio/mp4");
        append_mime_term(terms, "audio/x-aac");
    } else if (value == "public.plain-text") {
        append_mime_term(terms, "text/plain");
    } else if (value == "public.rtf") {
        append_mime_term(terms, "application/rtf");
        append_mime_term(terms, "text/rtf");
    } else if (value == "public.html") {
        append_mime_term(terms, "application/xhtml+xml");
        append_mime_term(terms, "text/html");
    } else if (value == "public.xml") {
        append_mime_term(terms, "application/xml");
        append_mime_term(terms, "text/xml");
    }
}

Xapian::Query no_match_query()
{
    return Xapian::Query("XNOMATCH__");
}

Xapian::Query type_query_for_value(const std::string &value)
{
    std::vector<std::string> terms;
    append_type_terms_for_value(value, terms);

    if (terms.empty()) {
        return no_match_query();
    }

    if (terms.size() == 1) {
        return Xapian::Query(terms[0]);
    }

    std::vector<Xapian::Query> queries;

    for (const auto &term : terms) {
        queries.push_back(Xapian::Query(term));
    }

    return Xapian::Query(Xapian::Query::OP_OR,
                         queries.begin(), queries.end());
}

bool build_query(const Xapian::Database &db,
                 const char *qstring,
                 Xapian::Query &query)
{
    std::string q = qstring ? qstring : "";
    std::vector<Xapian::Query> clauses;
    std::vector<std::string> name_terms;
    std::vector<std::string> text_terms;
    std::vector<std::string> type_terms;
    std::vector<std::string> any_terms;
    collect_values_after_key(q, "kMDItemFSName", name_terms);
    collect_values_after_key(q, "kMDItemDisplayName", name_terms);
    collect_values_after_key(q, "_kMDItemFileName", name_terms);
    collect_values_after_key(q, "kMDItemTextContent", text_terms);
    collect_values_after_key(q, "kMDItemKind", type_terms);
    collect_values_after_key(q, "kMDItemContentType", type_terms);
    collect_values_after_key(q, "kMDItemContentTypeTree", type_terms);
    collect_values_after_key(q, "_kMDItemGroupId", type_terms);
    any_terms = collect_any_values(q);

    for (const auto &term : name_terms) {
        clauses.push_back(parse_prefixed(db, term, "S"));
    }

    for (const auto &term : text_terms) {
        clauses.push_back(parse_prefixed(db, term, "B"));
    }

    for (const auto &term : type_terms) {
        clauses.push_back(type_query_for_value(term));
    }

    for (const auto &term : any_terms) {
        Xapian::Query nameq = parse_prefixed(db, term, "S");
        Xapian::Query bodyq = parse_prefixed(db, term, "B");
        clauses.push_back(Xapian::Query(Xapian::Query::OP_OR, nameq, bodyq));
    }

    if (clauses.empty()) {
        return false;
    }

    query = Xapian::Query(Xapian::Query::OP_AND,
                          clauses.begin(), clauses.end());
    return true;
}

bool ensure_database_parent(const std::string &db_path,
                            char *errbuf,
                            size_t errlen)
{
    std::string parent = dirname_of(db_path);
    std::string errmsg;

    if (mkdir_p(parent, errmsg)) {
        return true;
    }

    set_error(errbuf, errlen,
              std::string("could not create Xapian database parent ")
              + parent + ": " + errmsg);
    return false;
}

void chmod_database_path(const std::string &db_path)
{
    int dirfd = open(db_path.c_str(),
                     O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

    if (dirfd < 0) {
        return;
    }

    fchmod(dirfd, 0777);
    DIR *dir = fdopendir(dirfd);

    if (dir == nullptr) {
        close(dirfd);
        return;
    }

    const struct dirent *de;

    while ((de = readdir(dir)) != nullptr) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }

        struct stat st;

        if (fstatat(dirfd, de->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
            continue;
        }

        if (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)) {
            continue;
        }

        int flags = O_RDONLY | O_NOFOLLOW | O_CLOEXEC;

        if (S_ISDIR(st.st_mode)) {
            flags |= O_DIRECTORY;
        }

        int fd = openat(dirfd, de->d_name, flags);

        if (fd < 0) {
            continue;
        }

        fchmod(fd, S_ISDIR(st.st_mode) ? 0777 : 0666);
        close(fd);
    }

    closedir(dir);
}

} // namespace

extern "C" int sl_xapian_reconcile(const char *db_path,
                                   const char *volume_path,
                                   const char *volume_uuid,
                                   char *errbuf,
                                   size_t errlen)
{
    try {
        std::string magic_error;
        MagicCookie magic;

        if (db_path == nullptr || volume_path == nullptr) {
            set_error(errbuf, errlen, "missing Xapian database or volume path");
            return -1;
        }

        if (!ensure_database_parent(db_path, errbuf, errlen)) {
            return -1;
        }

        if (!magic.open(magic_error)) {
            set_error(errbuf, errlen,
                      std::string("could not initialize libmagic: ") + magic_error);
            return -1;
        }

        Xapian::WritableDatabase db(db_path, Xapian::DB_CREATE_OR_OPEN);
        std::string generation = next_generation_id(db);
        std::string walk_error;
        struct stat root_st;

        if (lstat(volume_path, &root_st) != 0) {
            set_error(errbuf, errlen,
                      std::string(volume_path) + ": " + strerror(errno));
            return -1;
        }

        XapianTransaction tx(db);
        db.set_metadata(METADATA_RECONCILE_IN_PROGRESS, generation);
        db.set_metadata(METADATA_DIRTY, "1");

        if (!walk_volume(db, volume_path, root_st.st_dev, magic.get(),
                         generation, walk_error)) {
            tx.cancel();
            set_error(errbuf, errlen,
                      std::string("could not reconcile Xapian index: ")
                      + walk_error);
            return -1;
        }

        delete_stale_documents(db, volume_path, generation);
        set_complete_metadata(db, volume_path, volume_uuid ? volume_uuid : "",
                              generation);
        tx.commit();
        chmod_database_path(db_path);
        return 0;
    } catch (const std::exception &e) {
        set_error(errbuf, errlen, e.what());
        return -1;
    } catch (...) {
        set_error(errbuf, errlen, "unknown Xapian exception");
        return -1;
    }
}

extern "C" int sl_xapian_index_ready(const char *db_path,
                                     const char *volume_path,
                                     const char *volume_uuid,
                                     char *errbuf,
                                     size_t errlen)
{
    try {
        if (db_path == nullptr || volume_path == nullptr) {
            set_error(errbuf, errlen, "missing Xapian database or volume path");
            return -1;
        }

        Xapian::Database db(db_path);
        const char *uuid = volume_uuid ? volume_uuid : "";
        bool ready = index_metadata_valid(db, volume_path, uuid);
        return ready ? 1 : 0;
    } catch (const Xapian::DatabaseOpeningError &) {
        return 0;
    } catch (const std::exception &e) {
        set_error(errbuf, errlen, e.what());
        return -1;
    } catch (...) {
        set_error(errbuf, errlen, "unknown Xapian exception");
        return -1;
    }
}

extern "C" int sl_xapian_query(const char *db_path,
                               const char *scope,
                               const char *qstring,
                               size_t offset,
                               size_t limit,
                               char ***paths,
                               size_t *count,
                               int *more,
                               char *errbuf,
                               size_t errlen)
{
    try {
        if (paths == nullptr || count == nullptr || more == nullptr) {
            set_error(errbuf, errlen, "missing Xapian query output pointer");
            return -1;
        }

        *paths = nullptr;
        *count = 0;
        *more = 0;

        if (db_path == nullptr || scope == nullptr || qstring == nullptr) {
            set_error(errbuf, errlen, "missing Xapian query input");
            return -1;
        }

        Xapian::Database db(db_path);
        Xapian::Query query;

        if (!build_query(db, qstring, query)) {
            return 0;
        }

        Xapian::doccount ask = limit == 0 ? 10000 : static_cast<Xapian::doccount>
                               (limit);
        Xapian::Enquire enquire(db);
        enquire.set_query(query);
        Xapian::MSet matches = enquire.get_mset(static_cast<Xapian::doccount>(offset),
                                                ask + 1);
        std::vector<std::string> out;

        for (auto it = matches.begin(); it != matches.end(); ++it) {
            if (out.size() >= ask) {
                *more = 1;
                break;
            }

            if (std::string path = it.get_document().get_value(VALUE_PATH);
                    !path.empty() && path_in_scope(path, scope)
                    && access(path.c_str(), F_OK) == 0) {
                out.push_back(path);
            }
        }

        if (out.empty()) {
            return 0;
        }

        char **arr = static_cast<char **>(calloc(out.size(), sizeof(char *)));

        if (arr == nullptr) {
            set_error(errbuf, errlen, "out of memory");
            return -1;
        }

        for (size_t i = 0; i < out.size(); i++) {
            arr[i] = strdup(out[i].c_str());

            if (arr[i] == nullptr) {
                sl_xapian_free_paths(arr, i);
                set_error(errbuf, errlen, "out of memory");
                return -1;
            }
        }

        *paths = arr;
        *count = out.size();
        return 0;
    } catch (const std::exception &e) {
        set_error(errbuf, errlen, e.what());
        return -1;
    } catch (...) {
        set_error(errbuf, errlen, "unknown Xapian exception");
        return -1;
    }
}

extern "C" int sl_xapian_upsert_path(const char *db_path,
                                     const char *path,
                                     char *errbuf,
                                     size_t errlen)
{
    try {
        std::string magic_error;
        MagicCookie magic;

        if (db_path == nullptr || path == nullptr) {
            return 0;
        }

        struct stat st;

        if (lstat(path, &st) != 0) {
            return sl_xapian_delete_path(db_path, path, errbuf, errlen);
        }

        if (!ensure_database_parent(db_path, errbuf, errlen)) {
            return -1;
        }

        if (!magic.open(magic_error)) {
            set_error(errbuf, errlen,
                      std::string("could not initialize libmagic: ") + magic_error);
            return -1;
        }

        Xapian::WritableDatabase db(db_path, Xapian::DB_CREATE_OR_OPEN);
        upsert_document(db, path, st, magic.get());
        db.commit();
        chmod_database_path(db_path);
        return 0;
    } catch (const std::exception &e) {
        set_error(errbuf, errlen, e.what());
        return -1;
    } catch (...) {
        set_error(errbuf, errlen, "unknown Xapian exception");
        return -1;
    }
}

extern "C" int sl_xapian_delete_path(const char *db_path,
                                     const char *path,
                                     char *errbuf,
                                     size_t errlen)
{
    try {
        if (db_path == nullptr || path == nullptr) {
            return 0;
        }

        if (!ensure_database_parent(db_path, errbuf, errlen)) {
            return -1;
        }

        Xapian::WritableDatabase db(db_path, Xapian::DB_CREATE_OR_OPEN);
        db.delete_document(unique_term_for_path(path));
        db.commit();
        chmod_database_path(db_path);
        return 0;
    } catch (const std::exception &e) {
        set_error(errbuf, errlen, e.what());
        return -1;
    } catch (...) {
        set_error(errbuf, errlen, "unknown Xapian exception");
        return -1;
    }
}

extern "C" int sl_xapian_delete_prefix(const char *db_path,
                                       const char *path,
                                       char *errbuf,
                                       size_t errlen)
{
    try {
        if (db_path == nullptr || path == nullptr) {
            return 0;
        }

        if (!ensure_database_parent(db_path, errbuf, errlen)) {
            return -1;
        }

        Xapian::WritableDatabase db(db_path, Xapian::DB_CREATE_OR_OPEN);
        delete_prefix_documents(db, path);
        db.commit();
        chmod_database_path(db_path);
        return 0;
    } catch (const std::exception &e) {
        set_error(errbuf, errlen, e.what());
        return -1;
    } catch (...) {
        set_error(errbuf, errlen, "unknown Xapian exception");
        return -1;
    }
}

extern "C" int sl_xapian_reindex_subtree(const char *db_path,
        const char *volume_path,
        const char *path,
        const char *oldpath,
        char *errbuf,
        size_t errlen)
{
    try {
        std::string magic_error;
        MagicCookie magic;

        if (db_path == nullptr || volume_path == nullptr
                || path == nullptr || path[0] == '\0'
                || oldpath == nullptr || oldpath[0] == '\0') {
            set_error(errbuf, errlen, "missing Xapian subtree reindex input");
            return -1;
        }

        if (!ensure_database_parent(db_path, errbuf, errlen)) {
            sl_xapian_mark_dirty(db_path, nullptr, 0);
            return -1;
        }

        if (!magic.open(magic_error)) {
            set_error(errbuf, errlen,
                      std::string("could not initialize libmagic: ") + magic_error);
            sl_xapian_mark_dirty(db_path, nullptr, 0);
            return -1;
        }

        struct stat root_st;

        if (lstat(volume_path, &root_st) != 0) {
            set_error(errbuf, errlen,
                      std::string(volume_path) + ": " + strerror(errno));
            sl_xapian_mark_dirty(db_path, nullptr, 0);
            return -1;
        }

        Xapian::WritableDatabase db(db_path, Xapian::DB_CREATE_OR_OPEN);
        std::string generation = db.get_metadata(METADATA_LAST_COMPLETE_GENERATION);
        const std::string *generation_ptr = generation.empty() ? nullptr : &generation;
        std::string walk_error;
        XapianTransaction tx(db);
        delete_prefix_documents(db, oldpath);

        if (!walk_subtree(db, path, root_st.st_dev, magic.get(),
                          generation_ptr, walk_error)) {
            tx.cancel();
            mark_dirty(db);
            db.commit();
            chmod_database_path(db_path);
            set_error(errbuf, errlen,
                      std::string("could not reindex moved subtree: ") + walk_error);
            return -1;
        }

        tx.commit();
        chmod_database_path(db_path);
        return 0;
    } catch (const std::exception &e) {
        set_error(errbuf, errlen, e.what());
        sl_xapian_mark_dirty(db_path, nullptr, 0);
        return -1;
    } catch (...) {
        set_error(errbuf, errlen, "unknown Xapian exception");
        sl_xapian_mark_dirty(db_path, nullptr, 0);
        return -1;
    }
}

extern "C" int sl_xapian_mark_dirty(const char *db_path,
                                    char *errbuf,
                                    size_t errlen)
{
    try {
        if (db_path == nullptr) {
            set_error(errbuf, errlen, "missing Xapian database path");
            return -1;
        }

        if (!ensure_database_parent(db_path, errbuf, errlen)) {
            return -1;
        }

        Xapian::WritableDatabase db(db_path, Xapian::DB_CREATE_OR_OPEN);
        mark_dirty(db);
        db.commit();
        chmod_database_path(db_path);
        return 0;
    } catch (const std::exception &e) {
        set_error(errbuf, errlen, e.what());
        return -1;
    } catch (...) {
        set_error(errbuf, errlen, "unknown Xapian exception");
        return -1;
    }
}

extern "C" void sl_xapian_free_paths(char **paths, size_t count)
{
    if (paths == nullptr) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(paths[i]);
    }

    free(paths);
}
