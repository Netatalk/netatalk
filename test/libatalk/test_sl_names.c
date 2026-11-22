/*
 * TAP tests for Spotlight's name comparisons
 *
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*!
 * @file
 * @brief Spotlight name comparisons: Apple's value syntax and modifiers, the
 *        query forms the Finder sends, and the database seeds
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/logger.h>
#include <atalk/sl_names.h>
#include <atalk/util.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define SL_C  SL_NAMES_CASE
#define SL_D  SL_NAMES_DIACRITIC
#define SL_W  SL_NAMES_WORD

static int tests_run;
static int tests_failed;

/* A value, its modifiers, a name and whether the value matches it */
struct match_case {
    const char *value;
    unsigned int flags;
    const char *name;
    bool want;
};

static const struct match_case match_cases[] = {
    /* Apple's anchoring examples */
    {"paris", 0, "paris", true},
    {"paris", 0, "paris.txt", false},
    {"paris*", 0, "paris.txt", true},
    {"paris*", 0, "my paris", false},
    {"*paris", 0, "my paris", true},
    {"*paris", 0, "paris.txt", false},
    {"*paris*", 0, "my paris.txt", true},
    /* c and d */
    {"paris", 0, "Paris", false},
    {"paris", SL_C, "Paris", true},
    {"cafe", 0, "caf\xc3\xa9", false},
    {"cafe", SL_D, "caf\xc3\xa9", true},
    {"cafe", SL_D, "cafe\xcc\x81", true},
    {"caf\xc3\xa9", 0, "cafe\xcc\x81", true},
    {"cafe", 0, "cafe\xcc\x81", false},
    /* two marks compose in turn: u, diaeresis, acute is ǘ */
    {"\xc7\x98", 0, "u\xcc\x88\xcc\x81", true},
    /* d drops the points of Hebrew and the harakat, hamza and madda of
     * Arabic, which a word typed without them lacks */
    {"\xd8\xb3\xd8\xa3*", SL_C | SL_D, "\xd8\xb3\xd8\xa3\xd9\x8e\xd9\x84", true},
    {
        "\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d*", SL_C | SL_D | SL_W,
        "\xd7\xa9\xd6\xb8\xd7\x81\xd7\x9c\xd7\x95\xd6\xb9\xd7\x9d.txt", true
    },
    {
        "\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d*", SL_C,
        "\xd7\xa9\xd6\xb8\xd7\x81\xd7\x9c\xd7\x95\xd6\xb9\xd7\x9d.txt", false
    },
    {
        "\xd8\xb3\xd9\x84\xd8\xa7\xd9\x85*", SL_C | SL_D | SL_W,
        "\xd8\xb3\xd9\x8e\xd9\x84\xd8\xa7\xd9\x85.txt", true
    },
    {
        "\xd8\xa3\xd8\xad\xd9\x85\xd8\xaf*", SL_C | SL_D,
        "\xd8\xa7\xd8\xad\xd9\x85\xd8\xaf.txt", true
    },
    /* '?' takes a letter with its harakat */
    {"?", 0, "\xd9\x83\xd9\x8e", true},
    {"\xd9\x83?", 0, "\xd9\x83\xd9\x8e", false},
    /* c and d on capitals */
    {"CAF\xc3\x89", SL_C | SL_D, "cafe", true},
    {"\xc3\xa9lan", SL_C, "\xc3\x89lan", true},
    {"\xc3\xa9lan", 0, "\xc3\x89lan", false},
    /* ? */
    {"IMG_??34.JPG", 0, "IMG_1234.JPG", true},
    {"IMG_??34.JPG", 0, "IMG_134.JPG", false},
    {"caf?", 0, "caf\xc3\xa9", true},
    {"caf?", 0, "cafe\xcc\x81", true},
    /* a mark that composes with nothing: '?' takes it with its letter */
    {"a?b", 0, "ax\xcc\x81" "b", true},
    {"a?b", 0, "ax\xcc\x81\xcc\xa3" "b", true},
    /* escapes */
    {"a\\*b", 0, "a*b", true},
    {"a\\*b", 0, "axb", false},
    {"a\\?b", 0, "a?b", true},
    {"a\\?b", 0, "axb", false},
    {"say \\\"hi\\\"", 0, "say \"hi\"", true},
    {"it\\'s", 0, "it's", true},
    {"back\\\\slash", 0, "back\\slash", true},
    /* % and _ */
    {"50%*", 0, "50% off.pdf", true},
    {"50%*", 0, "50 off.pdf", false},
    {"my_file*", 0, "my_file.doc", true},
    {"my_file*", 0, "myXfile.doc", false},
    /* w */
    {"foo*", SL_W, "my foo bar.txt", true},
    {"foo*", SL_W, "bar-foo.txt", true},
    {"foo*", SL_W, "xfoo.txt", false},
    {"two words*", SL_W, "my two words.txt", true},
    {"word", SL_W, "one word here", true},
    {"word", SL_W, "one words here", false},
    {"*port", SL_W, "airport news", true},
    /* precomposed and decomposed names fold alike, and d drops only the
     * marks it knows */
    {"\xd8\xa3*", SL_C, "\xd8\xa7\xd9\x94\xd8\xa8", true},
    {
        "*\xec\x82\xac\xec\xa7\x84*", SL_C,
        "\xe1\x84\x89\xe1\x85\xa1\xe1\x84\x8c\xe1\x85\xb5\xe1\x86\xab.jpg", true
    },
    {
        "\xe0\xa6\x95\xe0\xa7\x8b*", SL_C | SL_D | SL_W,
        "\xe0\xa6\x95\xe0\xa7\x87\xe0\xa6\x95.txt", false
    },
    {
        "\xe0\xa6\x95\xe0\xa7\x8b*", SL_C | SL_D,
        "\xe0\xa6\x95\xe0\xa7\x87\xe0\xa6\xbe.txt", true
    },
    {
        "\xe0\xa6\x95\xe0\xa7\x87*", SL_C,
        "\xe0\xa6\x95\xe0\xa7\x8b\xe0\xa6\x9f.txt", false
    },
    /* c beyond Latin-1 */
    {
        "\xd0\xbc\xd0\xbe\xd1\x81\xd0\xba\xd0\xb2\xd0\xb0*", SL_C,
        "\xd0\x9c\xd0\x9e\xd0\xa1\xd0\x9a\xd0\x92\xd0\x90.txt", true
    },
    {"\xf0\x90\x90\xa8*", SL_C, "\xf0\x90\x90\x80x", true},
    {"\xf0\x90\x90\xa8*", 0, "\xf0\x90\x90\x80x", false},
    /* letters the lowercase map leaves apart: final sigma, long s and the
     * micro sign; the Angstrom sign lowers to U+00E5 */
    {
        "\xce\xbb\xcf\x8c\xce\xb3\xce\xbf\xcf\x82*", SL_C | SL_D,
        "\xce\x9b\xce\x9f\xce\x93\xce\x9f\xce\xa3.pdf", true
    },
    {"sun*", SL_C, "\xc5\xbfun.txt", true},
    {"\xc2\xb5m*", SL_C, "\xce\x9cm.txt", true},
    {"asa*", SL_C | SL_D, "\xe2\x84\xab" "sa.txt", true},
    {"\xc3\xa5sa*", SL_C, "\xe2\x84\xab" "sa.txt", true},
    /* bytes that are not UTF-8 stay apart */
    {"caf\xe9", 0, "caf\xe9", true},
    {"caf\xe9", 0, "caf\xfc", false},
    {"*report*", SL_C, "abc\xc0\x80report.txt", true},
    {"abc?", 0, "abc\xc0\x80", false},
    /* w in scripts written without spaces, and at non-ASCII punctuation */
    {
        "\xe3\x82\xbf\xe3\x83\xaf\xe3\x83\xbc*", SL_C | SL_D | SL_W,
        "\xe6\x9d\xb1\xe4\xba\xac\xe3\x82\xbf\xe3\x83\xaf\xe3\x83\xbc.jpg",
        true
    },
    {
        "\xe8\xb3\x87\xe6\x96\x99*", SL_C | SL_D | SL_W,
        "\xe4\xbc\x9a\xe8\xad\xb0\xe8\xb3\x87\xe6\x96\x99.pdf", true
    },
    {
        "\xe9\x87\x8d\xe8\xa6\x81*", SL_C | SL_D | SL_W,
        "\xe3\x80\x90\xe9\x87\x8d\xe8\xa6\x81\xe3\x80\x91"
        "\xe4\xbc\x9a\xe8\xad\xb0.pdf", true
    },
    {
        "\xe0\xb8\x81\xe0\xb8\xb4\xe0\xb8\x99*", SL_W,
        "\xe0\xb8\x89\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x81\xe0\xb8\xb4"
        "\xe0\xb8\x99.txt", true
    },
    {"\xe0\xb8\x81", SL_W, "\xe0\xb8\x81\xe0\xb8\xb4\xe0\xb8\x99", false},
    {"2024*", SL_W, "R\xc3\xa9sum\xc3\xa9\xe2\x80\x93" "2024.pdf", true},
    {
        "\xd0\xb1\xd1\x83\xd0\xba*", SL_W,
        "\xd0\xba\xd0\xbd\xd0\xb8\xd0\xb3\xd0\xb0\xd0\xb1\xd1\x83\xd0\xba.txt",
        false
    },
    /* the punctuation and symbols of other scripts end a word, and a letter
     * outside ASCII continues one */
    {"report*", SL_W, "first\xd8\x8creport.txt", true},
    {"report*", SL_W, "why\xcd\xbereport.txt", true},
    {"report*", SL_W, "\xe0\xa4\x95\xe0\xa5\xa4report.txt", true},
    {"100*", SL_W, "price\xe2\x82\xac" "100.txt", true},
    {"m*", SL_W, "\xc2\xb5m.txt", false},
    /* an emoji outside the BMP, or a symbol in its emoji form, ends a word as
     * well; a letter outside the BMP continues one */
    {"report*", SL_C | SL_D | SL_W, "\xf0\x9f\x94\xa5Report.pdf", true},
    {"report*", SL_C | SL_D | SL_W, "\xe2\x9d\xa4\xef\xb8\x8fReport.pdf", true},
    {"report*", SL_C | SL_D | SL_W, "\xf0\x9d\x90\x80report.pdf", false},
    /* the variation selector of an emoji form folds away */
    {"*\xe2\x9d\xa4*", 0, "\xe2\x9d\xa4\xef\xb8\x8fx", true},
    {"*\xe2\x9d\xa4", SL_W, "x\xe2\x9d\xa4\xef\xb8\x8f", true},
    {"\xe2\x9d\xa4?", 0, "\xe2\x9d\xa4\xef\xb8\x8fx", true},
    {"\xe2\x9d\xa4?", 0, "\xe2\x9d\xa4\xef\xb8\x8f", false},
    {"?.txt", 0, "\xe2\x9d\xa4\xef\xb8\x8f.txt", true},
    {"??.txt", 0, "\xe2\x9d\xa4\xef\xb8\x8f.txt", false},
    /* so do an ideograph's variation selectors and Mongolian's free ones */
    {
        "\xe8\x91\x9b\xe9\xa3\xbe*", 0,
        "\xe8\x91\x9b\xf3\xa0\x84\x80\xe9\xa3\xbe.txt", true
    },
    {"\xe1\xa0\xa0\xe1\xa0\xa1", 0, "\xe1\xa0\xa0\xe1\xa0\x8b\xe1\xa0\xa1", true},
    /* a character of no word stands alone, so a value may start at one
     * after a space; Hangul's compatibility jamo, written with spaces,
     * continue a word */
    {"(2)*", SL_C | SL_D | SL_W, "photo (2).jpg", true},
    {"#12*", SL_C | SL_D | SL_W, "Invoice #12.pdf", true},
    {"report*", SL_C | SL_D | SL_W, "\xe3\x85\x8b\xe3\x85\x8breport.txt", false},
    /* U+1176, past the modern vowels, composes with no leading consonant;
     * U+1100 and U+1175 still do */
    {"\xe1\x84\x80\xe1\x85\xb6", 0, "\xea\xb9\x8c", false},
    {"\xe1\x84\x80\xe1\x85\xb5", 0, "\xea\xb8\xb0", true},
    /* != */
    {"*.txt", SL_NAMES_NEGATE, "a.pdf", true},
    {"*.txt", SL_NAMES_NEGATE, "a.txt", false},
};

/* A value, its modifiers and the LIKE pattern it compiles to */
struct like_case {
    const char *value;
    unsigned int flags;
    const char *like;
};

static const struct like_case like_cases[] = {
    /* letters and non-ASCII characters give way, for any collation, and a
     * variation selector or a mark d drops can stand beside any character */
    {"report*", SL_C, "%"},
    {"*.pdf", 0, "%.%"},
    {"IMG_2024*", SL_C | SL_W, "%!_%2%0%2%4%"},
    {"50%", 0, "%5%0%!%%"},
    {"a?b", 0, "%"},
    {"my_file", 0, "%!_%"},
    {"a!b", 0, "%!!%"},
    {"caf\xc3\xa9 1.txt", 0, "% %1%.%"},
    {"*.txt", SL_NAMES_NEGATE, "%"},
    {"2023*", SL_C | SL_D, "%2%0%2%3%"},
    {"*5%x", SL_D, "%5%!%%"},
};

/* A query, a name and whether the query matches it */
struct query_case {
    const char *query;
    const char *name;
    bool want;
};

static const struct query_case query_cases[] = {
    /* both places of the modifiers, = and == */
    {"kMDItemFSName == \"report*\"cd", "Report 2024.pdf", true},
    {"kMDItemFSName ==[cd] \"report*\"", "Report 2024.pdf", true},
    {"kMDItemFSName == \"report*\"", "Report 2024.pdf", false},
    {"kMDItemDisplayName = \"*REPORT*\"c", "my report.txt", true},
    {"_kMDItemFileName == \"*.pdf\"", "a.pdf", true},
    /* the Finder's forms the Spotlight spectests send */
    {
        "kMDItemFSName==\"phrase632-alpha beta*\"cd",
        "phrase632-alpha beta.txt", true
    },
    {
        "kMDItemFSName==\"\\\"phrase632-alpha beta\\\"*\"cd",
        "phrase632-alpha beta.txt", true
    },
    {
        "kMDItemFSName==\"\\\"phrase632-alpha beta\\\"*\"cd",
        "phrase632-alpha phrase632-beta.txt", false
    },
    {
        "*==\"phrase632-beta*\"cdw"
        "||kMDItemTextContent==\"phrase632-beta*\"cdw",
        "phrase632-alpha phrase632-beta.txt", true
    },
    {
        "*==\"phrase632-beta*\"cdw"
        "||kMDItemTextContent==\"phrase632-beta*\"cdw",
        "phrase632-alpha beta.txt", false
    },
    {
        "kMDItemDisplayName==\"*kMDItemFSName*\"cd"
        "||kMDItemTextContent==\"alpha*\"cd",
        "alpha.txt", false
    },
    {"kMDItemFSName==\"paginate530-f-*\"cd", "paginate530-f-00001.txt", true},
    /* != and the logic */
    {"kMDItemFSName != \"*.txt\"", "a.pdf", true},
    {"kMDItemFSName != \"*.txt\"", "a.txt", false},
    {
        "kMDItemFSName==\"*.pdf\"c && kMDItemFSName!=\"draft*\"c",
        "final.pdf", true
    },
    {
        "kMDItemFSName==\"*.pdf\"c && kMDItemFSName!=\"draft*\"c",
        "Draft.pdf", false
    },
    {
        "(kMDItemFSName==\"a*\"||kMDItemFSName==\"b*\")"
        "&&kMDItemFSName==\"*.txt\"", "b.txt", true
    },
    {
        "(kMDItemFSName==\"a*\"||kMDItemFSName==\"b*\")"
        "&&kMDItemFSName==\"*.txt\"", "b.pdf", false
    },
    {
        "kMDItemFSName==\"report*\"cd&&kMDItemContentType==\"public.text\"",
        "report.txt", true
    },
    {"kMDItemFSName==\"*\\\"two words\\\"*\"cd", "my two words.txt", true},
    /* without the trailing '*' the escaped quotes are quotes */
    {"kMDItemFSName==\"\\\"two words\\\"\"cd", "\"two words\"", true},
    {"kMDItemFSName==\"\\\"two words\\\"\"cd", "two words", false},
    {"kMDItemFSName==\"*\\\"two words\\\"\"cd", "my \"two words\"", true},
    /* comparisons of other modifiers fold the name apart */
    {"kMDItemFSName==\"*Ab*\"&&kMDItemFSName==\"*ab*\"c", "xAbx", true},
    {"kMDItemFSName==\"*Ab*\"&&kMDItemFSName==\"*ab*\"c", "xabx", false},
    /* the Finder's pairs for five words: a pair counts once, so the fifth
     * word stays within the 8 comparisons kept */
    {
        "kMDItemFSName==\"w1*\"cd||kMDItemDisplayName==\"w1*\"cd"
        "||kMDItemFSName==\"w2*\"cd||kMDItemDisplayName==\"w2*\"cd"
        "||kMDItemFSName==\"w3*\"cd||kMDItemDisplayName==\"w3*\"cd"
        "||kMDItemFSName==\"w4*\"cd||kMDItemDisplayName==\"w4*\"cd"
        "||kMDItemFSName==\"w5*\"cd||kMDItemDisplayName==\"w5*\"cd",
        "w5.txt", true
    },
    /* a volume that folds names to one case converts the query alike */
    {"KMDITEMFSNAME==\"REPORT*\"CDW", "Q3 report.pdf", true},
    {"kmditemfsname==\"report*\"cdw", "Q3 REPORT.PDF", true},
    {"_KMDITEMFILENAME==\"REPORT*\"", "report.pdf", false},
};

/*!
 * @brief Print one TAP result
 *
 * @param[in] passed  whether the check passed
 * @param[in] name    what it checked
 */
static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

/*!
 * @brief Check each value against its name
 */
static void test_values(void)
{
    char label[256];

    for (size_t i = 0; i < ARRAY_SIZE(match_cases); i++) {
        const struct match_case *t = &match_cases[i];
        struct sl_name_pattern *p;
        p = sl_name_pattern_new(t->value, strlen(t->value), t->flags);
        snprintf(label, sizeof(label), "value '%s' flags %u on '%s'",
                 t->value, t->flags, t->name);
        report(p != NULL && sl_name_pattern_match(p, t->name, strlen(t->name))
               == t->want, label);
        sl_name_pattern_free(p);
    }
}

/*!
 * @brief Check the LIKE pattern of each value
 */
static void test_like(void)
{
    char label[256];

    for (size_t i = 0; i < ARRAY_SIZE(like_cases); i++) {
        const struct like_case *t = &like_cases[i];
        struct sl_name_pattern *p;
        p = sl_name_pattern_new(t->value, strlen(t->value), t->flags);
        snprintf(label, sizeof(label), "value '%s' flags %u takes LIKE '%s'",
                 t->value, t->flags, t->like);
        report(p != NULL && strcmp(sl_name_pattern_like(p), t->like) == 0,
               label);
        sl_name_pattern_free(p);
    }
}

/*!
 * @brief Check each query against its name
 */
static void test_queries(void)
{
    char label[512];

    for (size_t i = 0; i < ARRAY_SIZE(query_cases); i++) {
        const struct query_case *t = &query_cases[i];
        struct sl_name_query *q = sl_name_query_parse(t->query, 0, 8);
        snprintf(label, sizeof(label), "query %s on '%s'", t->query, t->name);
        report(q != NULL && sl_name_query_match(q, t->name, strlen(t->name))
               == t->want, label);
        sl_name_query_free(q);
    }
}

/*!
 * @brief Check a name longer than the matcher keeps folded for a query
 */
static void test_long_name(void)
{
    static const char query[] = "kMDItemFSName==\"*REPORT*\"c"
                                "&&kMDItemFSName==\"*.txt\"";
    char name[300];
    struct sl_name_query *q = sl_name_query_parse(query, 0, 8);
    bool txt;
    memset(name, 'x', sizeof(name));
    memcpy(name + sizeof(name) - 10, "report.txt", 10);
    txt = q != NULL && sl_name_query_match(q, name, sizeof(name));
    memcpy(name + sizeof(name) - 4, ".pdf", 4);
    report(txt && !sl_name_query_match(q, name, sizeof(name)),
           "a 300-byte name matches as a short one");
    sl_name_query_free(q);
}

/*!
 * @brief Check a name longer than a folding the matcher keeps, compared
 *        under c, without modifiers and under c again
 *
 * The last value differs from the first, as the parser drops a repeat of a
 * comparison from its chain.
 */
static void test_longer_name(void)
{
    static const char query[] = "kMDItemFSName==\"REPORT*\"c"
                                "&&kMDItemFSName==\"RePort*\""
                                "&&kMDItemFSName==\"REPORTX*\"c";
    char name[600];
    struct sl_name_query *q = sl_name_query_parse(query, 0, 8);
    memset(name, 'X', sizeof(name));
    memcpy(name, "RePort", 6);
    report(q != NULL && sl_name_query_match(q, name, sizeof(name)),
           "a 600-byte name matches each comparison as its own folding");
    sl_name_query_free(q);
}

/*!
 * @brief Check that I with dot above folds alike precomposed and decomposed
 */
static void test_dotted_i(void)
{
    static const char pre[] = "\xc4\xb0zmir";
    static const char dec[] = "I\xcc\x87zmir";
    struct sl_name_query *c = sl_name_query_value(pre, sizeof(pre) - 1, SL_C);
    struct sl_name_query *plain = sl_name_query_value("izmir", 5, SL_C);
    struct sl_name_query *cd = sl_name_query_value("izmir", 5, SL_C | SL_D);
    report(c != NULL && sl_name_query_match(c, pre, sizeof(pre) - 1)
           && sl_name_query_match(c, dec, sizeof(dec) - 1),
           "under c a precomposed and a decomposed I with dot above compare "
           "alike");
    report(plain != NULL && !sl_name_query_match(plain, pre, sizeof(pre) - 1)
           && !sl_name_query_match(plain, dec, sizeof(dec) - 1),
           "under c alone an i misses I with dot above");
    report(cd != NULL && sl_name_query_match(cd, pre, sizeof(pre) - 1)
           && sl_name_query_match(cd, dec, sizeof(dec) - 1),
           "under cd an i matches I with dot above in both forms");
    sl_name_query_free(c);
    sl_name_query_free(plain);
    sl_name_query_free(cd);
}

/*!
 * @brief Count the seeds of a query
 *
 * @param[in] query  the query
 * @param[in] min    literal bytes a comparison needs
 * @param[in] max    comparisons the query keeps
 * @returns the seeds, -1 when the parse yields no query
 */
static int seeds(const char *query, size_t min, int max)
{
    const struct sl_name_pattern *s[8];
    struct sl_name_query *q = sl_name_query_parse(query, min, max);
    int n = q != NULL ? sl_name_query_seeds(q, s, 8, NULL, NULL) : -1;
    sl_name_query_free(q);
    return n;
}

/*!
 * @brief Check which comparisons a query keeps and which seeds cover it
 */
static void test_policy(void)
{
    static const char call[] = "InRange(kMDItemFSContentChangeDate,"
                               "$time.iso(2024-01-01T00:00:00Z),"
                               "$time.iso(2024-12-31T00:00:00Z))"
                               "&&kMDItemFSName==\"a*\"";
    static const char repeated[] = "kMDItemFSName==\"report*\"cd"
                                   "||kMDItemDisplayName==\"report*\"cd";
    static const char across[] = "(kMDItemFSName==\"report*\"cd"
                                 "&&kMDItemFSName==\"x*\"cd)"
                                 "||kMDItemFSName==\"report*\"cd";
    static const char room[] = "kMDItemFSName==\"w1*\"cd"
                               "||kMDItemDisplayName==\"w1*\"cd"
                               "||kMDItemFSName==\"w2*\"cd"
                               "||kMDItemDisplayName==\"w2*\"cd"
                               "||kMDItemFSName==\"w3*\"cd";
    static const char spelled[] = "kMDItemFSName==\"Alpha*\"c"
                                  "||kMDItemFSName==\"alpha*\"c"
                                  "||kMDItemFSName==\"caf\xc3\xa9*\"c"
                                  "||kMDItemFSName==\"cafe\xcc\x81*\"c"
                                  "||kMDItemFSName==\"beta*\"c";
    report(seeds("kMDItemFSName==\"ab*\"cd", 3, 8) == -1,
           "a value under the minimum is dropped");
    report(seeds("kMDItemFSName==\"ab*\"cd||kMDItemFSName==\"alpha*\"cd", 3,
                 8) == 1, "the comparison after a short one is kept");
    report(seeds("kMDItemTextContent==\"alpha*\"cd", 0, 8) == -1,
           "a content comparison is not a name comparison");
    report(seeds("kMDItemFSName==\"a*\"||kMDItemFSName==\"b*\"", 0, 8) == 2,
           "an OR seeds each side");
    report(seeds("kMDItemFSName==\"a*\"&&kMDItemFSName==\"b*\"", 0, 8) == 1,
           "an AND seeds one side");
    report(seeds("kMDItemFSName!=\"a*\"", 0, 8) == 0,
           "a negation reads every name");
    report(seeds("kMDItemFSName==\"a*\"||kMDItemFSName!=\"b*\"", 0, 8) == 0,
           "an OR with a negation reads every name");
    report(seeds("kMDItemFSName==\"a*\"||kMDItemFSName==\"b*\"", 0, 1) == 1,
           "comparisons past the maximum are dropped");
    report(seeds(call, 0, 8) == 1, "a function call is read past");
    report(seeds("kMDItemFSName==\"ab*\"cd&&kMDItemFSName!=\"*.tmp\"c", 3, 8)
           == -1, "a value under the minimum makes its AND match no name");
    report(seeds("kMDItemFSName==\"a*\"&&kMDItemFSName==\"b*\"", 0, 1) == -1,
           "a comparison past the maximum makes its AND match no name");
    report(seeds(repeated, 0, 8) == 1, "a repeated comparison seeds once");
    report(seeds(across, 0, 8) == 1,
           "a comparison repeated across an AND seeds once");
    report(seeds("kMDItemFSName==\"report*\"cd||kMDItemFSName==\"report*\"c",
                 0, 8) == 2, "comparisons with other modifiers seed apart");
    report(seeds("kMDItemFSName==\"abc", 0, 8) == -1,
           "a value without its closing quote fails the query");
    report(seeds("kMDItemFSName==\"\xcc\x81\xcc\x81*\"cdw", 3, 8) == -1,
           "a value of marks alone folds under the minimum under d");
    report(seeds("kMDItemFSName==\"a*\")||kMDItemFSName==\"b*\"", 0, 8) == -1,
           "a query that does not read to its end matches no name");
    report(seeds(room, 0, 2) == 2,
           "a repeated comparison takes no room past the maximum");
    report(seeds(spelled, 0, 3) == 3,
           "a comparison spelled another way takes no room again");
}

/*!
 * @brief A backend test that narrows by a seed of 6 folded bytes or more
 *
 * @param[in] seed  the seed
 * @param[in] ctx   unused
 * @returns whether the seed narrows
 */
static bool narrows_six(const struct sl_name_pattern *seed, void *ctx)
{
    (void)ctx;
    return sl_name_pattern_literals(seed) >= 6;
}

/* Calls of narrows_never() */
static int narrows_calls;

/*!
 * @brief A backend that narrows by no seed, counting the calls
 *
 * @param[in] seed  the seed
 * @param[in] ctx   unused
 * @returns false
 */
static bool narrows_never(const struct sl_name_pattern *seed, void *ctx)
{
    (void)seed;
    (void)ctx;
    narrows_calls++;
    return false;
}

/*!
 * @brief Check the side of an AND a search runs by, and that the choice
 *        reads each side once
 */
static void test_seed_choice(void)
{
    static const char either[] = "kMDItemFSName==\"*ab*\"c"
                                 "&&kMDItemFSName==\"*report*\"c";
    static const char term[] = "kMDItemFSName==\"*abc00*\"c";
    const struct sl_name_pattern *s[8];
    struct sl_name_query *q = sl_name_query_parse(either, 0, 8);
    char chain[20 * sizeof(term) + 20 * 2];
    size_t len = 0;
    int n = q != NULL ? sl_name_query_seeds(q, s, 8, narrows_six, NULL) : 0;
    report(n == 1 && sl_name_pattern_literals(s[0]) == 6,
           "an AND seeds the side the backend narrows by");
    n = q != NULL ? sl_name_query_seeds(q, s, 8, NULL, NULL) : 0;
    report(n == 1 && sl_name_pattern_literals(s[0]) == 2,
           "an AND seeds its left side when the backend takes either");
    sl_name_query_free(q);
    q = sl_name_query_parse("kMDItemFSName==\"*report*\"c"
                            "||kMDItemFSName==\"*ab*\"c", 0, 8);
    n = q != NULL ? sl_name_query_seeds(q, s, 8, narrows_six, NULL) : -1;
    report(n == 0, "an OR with a side the backend does not narrow by seeds "
                   "nothing");
    sl_name_query_free(q);

    /* distinct comparisons, as a chain of repeats joins into one */
    for (int i = 0; i < 20; i++) {
        len += (size_t)snprintf(chain + len, sizeof(chain) - len,
                                "%skMDItemFSName==\"*abc%02d*\"c",
                                i ? "&&" : "", i);
    }

    q = sl_name_query_parse(chain, 0, 32);
    narrows_calls = 0;
    n = q != NULL ? sl_name_query_seeds(q, s, 8, narrows_never, NULL) : 0;
    report(q != NULL && n == 0 && narrows_calls <= 2 * 20,
           "an AND of 20 comparisons no backend narrows by seeds nothing and "
           "asks it 40 times at most");
    sl_name_query_free(q);
}

/*!
 * @brief Check the queries the parser refuses whole
 *
 * A missing closing parenthesis, quote or bracket, a call left open, an
 * operator, a group or a name attribute without its operand, a comparison
 * without its value, nesting past 64 and more than 32 name comparisons,
 * repeats counted, match no name.
 */
static void test_refused(void)
{
    static const char term[] = "kMDItemFSName==\"*abc*\"c";
    static const char *const no_operand[] = {
        "kMDItemFSName==\"*abc*\"c&&", "kMDItemFSName==\"*abc*\"c||",
        "kMDItemFSName==\"*abc*\"c||()",
        "kMDItemFSName||kMDItemFSName==\"*abc*\"c",
        "kMDItemFSName \"*abc*\"c||kMDItemFSName==\"*abc*\"c",
        "kMDItemFSName==[c \"*abc*\"",
        "kMDItemFSName==\"*abc*\"c||kMDItemFSName==\"abc",
        "kMDItemFSName==\"*abc*\"c||InRange(",
        "kMDItemFSName==\"*abc*\"c||kMDItemFSName==",
        "kMDItemFSName==\"*abc*\"c||!",
    };
    char chain[33 * sizeof(term) + 33 * 2];
    char nested[2 * 65 + sizeof(term)];
    char label[96];
    size_t len = 0;
    struct sl_name_query *q = sl_name_query_parse("(kMDItemFSName==\"*abc*\"c",
                                                  0, 8);
    report(q == NULL, "a query without its closing parenthesis matches no "
                      "name");
    sl_name_query_free(q);

    for (int depth = 64; depth <= 65; depth++) {
        memset(nested, '(', (size_t)depth);
        memcpy(nested + depth, term, sizeof(term) - 1);
        memset(nested + depth + sizeof(term) - 1, ')', (size_t)depth);
        nested[2 * depth + sizeof(term) - 1] = '\0';
        q = sl_name_query_parse(nested, 0, 8);
        snprintf(label, sizeof(label), "a query nested %d deep %s", depth,
                 depth == 64 ? "is read" : "matches no name");
        report(depth == 64 ? q != NULL && sl_name_query_match(q, "xabcx", 5)
               : q == NULL, label);
        sl_name_query_free(q);
    }

    for (size_t i = 0; i < ARRAY_SIZE(no_operand); i++) {
        q = sl_name_query_parse(no_operand[i], 0, 8);
        snprintf(label, sizeof(label), "query '%s' matches no name",
                 no_operand[i]);
        report(q == NULL, label);
        sl_name_query_free(q);
    }

    for (int i = 0; i < 32; i++) {
        len += (size_t)snprintf(chain + len, sizeof(chain) - len, "%s%s",
                                i ? "||" : "", term);
    }

    q = sl_name_query_parse(chain, 0, 8);
    report(q != NULL && sl_name_query_match(q, "xabcx", 5),
           "a query of 32 name comparisons is read");
    sl_name_query_free(q);
    snprintf(chain + len, sizeof(chain) - len, "||%s", term);
    q = sl_name_query_parse(chain, 0, 8);
    report(q == NULL, "a query of 33 name comparisons matches no name");
    sl_name_query_free(q);
}

/* A search term, whether it is a whole name, a name, and whether the term
 * matches the name */
struct term_case {
    const char *term;
    bool exact;
    const char *name;
    bool match;
};

static const struct term_case term_cases[] = {
    {"a*b?c", true, "a*b?c", true},
    {"a*b?c", true, "axxbyc", false},
    {"A\\B", true, "a\\b", true},
    {"a\\*b", false, "xa*by", true},
    {"a\\*b", false, "axxb", false},
    {"ab\\", false, "xab\\y", true},
    {"ab\\", false, "xaby", false},
};

/*!
 * @brief Check the queries made from search terms, for whole names and for
 *        the names that contain the term
 */
static void test_term_query(void)
{
    char label[160];

    for (size_t i = 0; i < ARRAY_SIZE(term_cases); i++) {
        const struct term_case *t = &term_cases[i];
        struct sl_name_query *q = sl_name_query_term(t->term, strlen(t->term),
                                                     t->exact, SL_C);
        snprintf(label, sizeof(label), "term '%s'%s %s '%s'", t->term,
                 t->exact ? " as a whole name" : "",
                 t->match ? "matches" : "does not match", t->name);
        report(q != NULL
               && sl_name_query_match(q, t->name, strlen(t->name)) == t->match,
               label);
        sl_name_query_free(q);
    }
}

/*!
 * @brief Check a query of one comparison made from its value
 */
static void test_value_query(void)
{
    struct sl_name_query *q = sl_name_query_value("*rep*", 5, SL_C);
    report(q != NULL && sl_name_query_match(q, "Report.txt", 10)
           && !sl_name_query_match(q, "notes.txt", 9),
           "a query of one value matches as the value does");
    sl_name_query_free(q);
}

/* A value, its modifiers, and the name index's GLOB and LIKE patterns */
struct index_case {
    const char *value;
    unsigned int flags;
    const char *glob;
    const char *like;
};

static const struct index_case index_cases[] = {
    {"Caf\xc3\xa9*", SL_C, "cafe*", "cafe%"},
    {"*cafe*", SL_C, "*cafe*", "%cafe%"},
    {"*cafe", SL_C, "*cafe", "%cafe"},
    {"cafe*", SL_C | SL_W, "*cafe*", "%cafe%"},
    {"x?y", 0, "x*y", "x%y"},
    {"a[b", 0, "a?b", "a[b"},
    {"50%_!", 0, "50%_!", "50!%!_!!"},
    {"r\xe9s", 0, "r\xef\xbf\xbds", "r\xef\xbf\xbds"},
};

/* A value, its modifiers and a name it matches */
struct index_hold_case {
    const char *value;
    unsigned int flags;
    const char *name;
};

static const struct index_hold_case index_holds[] = {
    {"re*rt", SL_C, "Report"},
    {"*caf\xc3\xa9*b?r*", SL_C | SL_D, "my CAFE\xcc\x81 bar.txt"},
    {"\xce\xa3*\xcf\x82", SL_C, "\xcf\x83\xce\xb9\xcf\x83"},
    {"\xc4\xb0stan*", SL_C | SL_D, "istanbul"},
    {"*\xea\xb0\x80?", SL_C, "\xe1\x84\x80\xe1\x85\xa1\xe1\x84\x80"},
    {"*50%*?_*", 0, "a 50% x_y"},
    /* a mark that d drops between two jamo, or a syllable and its final
     * consonant, which compose without it */
    {"\xe1\x84\x80\xcc\x81*", SL_C, "\xe1\x84\x80\xcc\x81\xe1\x85\xa1"},
    {"\xea\xb0\x80\xcc\xa3*", SL_C, "\xea\xb0\x80\xcc\xa3\xe1\x86\xa8"},
    {"*\xe1\x85\xa1", SL_C, "\xe1\x84\x80\xcc\xa3\xe1\x85\xa1"},
    /* أ before a fatha, whose marks d and the index drop */
    {"*\xd8\xb3\xd8\xa3*", SL_C | SL_D, "\xd8\xb3\xd8\xa3\xd9\x8e\xd9\x84"},
};

/* A value, its modifiers and a name it matches whose bytes the value's LIKE
 * pattern must take: variation selectors fold away under any modifiers, and
 * a mark under d */
static const struct index_hold_case like_holds[] = {
    {"*1.2*", 0, "v1\xef\xb8\x8f.2.txt"},
    {"12*", SL_C, "1\xef\xb8\x8e" "2.txt"},
    {"50%", 0, "\xef\xb8\x8f" "50%"},
    {"2023", SL_W, "20\xef\xb8\x8f" "23 report"},
    {"*5#", 0, "x5\xf3\xa0\x84\x80#"},
    {"2023*", SL_C | SL_D, "2\xcc\x81" "023.txt"},
};

/*!
 * @brief Match a GLOB pattern of '*', '?' and literal bytes, as the index's
 *        patterns hold them, against UTF-8 text, '?' taking one character
 *
 * @param[in] p  the pattern
 * @param[in] s  the text
 * @returns whether the pattern takes the text
 */
static bool glob_takes(const char *p, const char *s)
{
    if (*p == '*') {
        for (;; s++) {
            if (glob_takes(p + 1, s)) {
                return true;
            }

            if (*s == '\0') {
                return false;
            }
        }
    }

    if (*p == '\0' || *s == '\0') {
        return *p == *s;
    }

    if (*p == '?') {
        do {
            s++;
        } while ((*s & 0xc0) == 0x80);

        return glob_takes(p + 1, s);
    }

    return *p == *s && glob_takes(p + 1, s + 1);
}

/*!
 * @brief Match a LIKE pattern of '%', '!' escapes and literal bytes against
 *        text, as a column of folded names compares its bytes
 *
 * @param[in] p  the pattern
 * @param[in] s  the text
 * @returns whether the pattern takes the text
 */
static bool like_takes(const char *p, const char *s)
{
    if (*p == '%') {
        for (;; s++) {
            if (like_takes(p + 1, s)) {
                return true;
            }

            if (*s == '\0') {
                return false;
            }
        }
    }

    if (*p == '!' && p[1] != '\0') {
        p++;
    }

    if (*p == '\0' || *s == '\0') {
        return *p == *s;
    }

    return *p == *s && like_takes(p + 1, s + 1);
}

/*!
 * @brief Check that the LIKE pattern of a value takes the names it matches
 */
static void test_like_holds(void)
{
    char label[160];

    for (size_t i = 0; i < ARRAY_SIZE(like_holds); i++) {
        const struct index_hold_case *t = &like_holds[i];
        const size_t len = strlen(t->name);
        struct sl_name_pattern *p;
        p = sl_name_pattern_new(t->value, strlen(t->value), t->flags);
        snprintf(label, sizeof(label),
                 "value '%s' flags %u matches '%s', and its LIKE pattern "
                 "takes the name", t->value, t->flags, t->name);
        report(p != NULL && sl_name_pattern_match(p, t->name, len)
               && like_takes(sl_name_pattern_like(p), t->name), label);
        sl_name_pattern_free(p);
    }
}

/*!
 * @brief Check the patterns the name index takes for a value, that they hold
 *        the folded form of names the value matches, and a name folded as
 *        the index holds it
 */
static void test_index_patterns(void)
{
    char label[160];
    char *fold;

    for (size_t i = 0; i < ARRAY_SIZE(index_cases); i++) {
        const struct index_case *t = &index_cases[i];
        struct sl_name_pattern *p;
        p = sl_name_pattern_new(t->value, strlen(t->value), t->flags);
        snprintf(label, sizeof(label),
                 "value '%s' flags %u takes GLOB '%s' and LIKE '%s' over "
                 "folded names", t->value, t->flags, t->glob, t->like);
        report(p != NULL
               && strcmp(sl_name_pattern_index_glob(p), t->glob) == 0
               && strcmp(sl_name_pattern_index_like(p), t->like) == 0, label);
        sl_name_pattern_free(p);
    }

    for (size_t i = 0; i < ARRAY_SIZE(index_holds); i++) {
        const struct index_hold_case *t = &index_holds[i];
        const size_t len = strlen(t->name);
        struct sl_name_pattern *p;
        p = sl_name_pattern_new(t->value, strlen(t->value), t->flags);
        fold = sl_name_fold(t->name, len);
        snprintf(label, sizeof(label),
                 "value '%s' flags %u matches '%s', and its GLOB and LIKE "
                 "patterns take the name folded", t->value, t->flags,
                 t->name);
        report(p != NULL && fold != NULL
               && sl_name_pattern_match(p, t->name, len)
               && glob_takes(sl_name_pattern_index_glob(p), fold)
               && like_takes(sl_name_pattern_index_like(p), fold), label);
        free(fold);
        sl_name_pattern_free(p);
    }

    fold = sl_name_fold("\xc3\x84pfel R\xe9s", 10);
    report(fold != NULL && strcmp(fold, "apfel r\xef\xbf\xbds") == 0,
           "a name folds for case and diacritics, a byte that starts no "
           "UTF-8 character to U+FFFD");
    free(fold);
}

/*!
 * @brief Check a query and a term in Latin-1, against names in Latin-1: c
 *        and d fold their letters, and no letter matches another
 */
static void test_latin1(void)
{
    struct sl_name_query *q;
    char *folded;
    charset_t latin;
    set_charset_name(CH_UNIX, "UTF8");
    set_charset_name(CH_MAC, "MAC_ROMAN");

    if ((latin = add_charset("ISO-8859-1")) == (charset_t) -1) {
        printf("ok %d - Latin-1 names # SKIP no ISO-8859-1\n", ++tests_run);
        return;
    }

    /* "*ÉTÉ*"c on été.txt and àtà.txt, "*ete*"cd on été.txt */
    q = sl_name_query_parse_in("kMDItemFSName==\"*\xc9T\xc9*\"c", 0, 8, latin);
    report(q != NULL && sl_name_query_match(q, "\xe9t\xe9.txt", 7)
           && !sl_name_query_match(q, "\xe0t\xe0.txt", 7),
           "a Latin-1 query folds its letters' case, and none matches "
           "another");
    sl_name_query_free(q);
    q = sl_name_query_parse_in("kMDItemFSName==\"*ete*\"cd", 0, 8, latin);
    report(q != NULL && sl_name_query_match(q, "\xe9t\xe9.txt", 7),
           "a Latin-1 query folds its letters' diacritics");
    sl_name_query_free(q);
    q = sl_name_query_term_in("\xc9T\xc9.TXT", 7, true, SL_C, latin);
    report(q != NULL && sl_name_query_match(q, "\xe9t\xe9.txt", 7)
           && !sl_name_query_match(q, "\xe0t\xe0.txt", 7),
           "a Latin-1 whole name matches its case variants alone");
    sl_name_query_free(q);
    folded = sl_name_fold_in("\xc9t\xc9.TXT", 7, latin);
    report(folded != NULL && strcmp(folded, "ete.txt") == 0,
           "a Latin-1 name folds as its UTF-8 form does");
    free(folded);
}

/*!
 * @brief Check that a name which does not convert from the volume's charset
 *        matches by its bytes and writes no log line, as a search compares
 *        every name of a volume that other tools fill
 */
static void test_unconverted_name(void)
{
    char dir[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 8];
    struct sl_name_query *q;
    struct stat st;
    char *folded;
    charset_t sjis;
    int length;

    if ((sjis = add_charset("SHIFT_JIS")) == (charset_t) -1) {
        printf("ok %d - a name that does not convert # SKIP no Shift_JIS\n",
               ++tests_run);
        return;
    }

    length = snprintf(dir, sizeof(dir), "%s/sl_names.XXXXXX", tmpdir());

    if (length < 0 || (size_t)length >= sizeof(dir) || mkdtemp(dir) == NULL) {
        printf("Bail out! no scratch directory: %s\n", strerror(errno));
        exit(1);
    }

    snprintf(path, sizeof(path), "%s/log", dir);
    setuplog("default:error", path, false);
    /* a lead byte at the end, which no Shift_JIS decoder takes */
    q = sl_name_query_parse_in("kMDItemFSName==\"*abc*\"c", 0, 8, sjis);
    folded = sl_name_fold_in("ABC.TXT\x81", 8, sjis);
    report(q != NULL && sl_name_query_match(q, "abc.txt\x81", 8)
           && folded != NULL && strncmp(folded, "abc.txt", 7) == 0,
           "a name that does not convert folds and matches its bytes");
    report(stat(path, &st) == 0 && st.st_size == 0,
           "a name that does not convert writes no log line");
    setuplog("default:severe", "/dev/stderr", false);
    sl_name_query_free(q);
    free(folded);
    unlink(path);
    rmdir(dir);
}

/*!
 * @brief Write a query whose name value starts some bytes before the end of
 *        the matcher's first piece, MAXPATHLEN / 2 bytes, after a content
 *        comparison of padding
 *
 * @param[in] value   the value, in the query's charset
 * @param[in] before  bytes of the value before the piece's end
 * @returns the query, to free(), or NULL when out of memory
 */
static char *straddling_query(const char *value, size_t before)
{
    static const char head[] = "kMDItemTextContent==\"";
    static const char mid[] = "\"||kMDItemFSName==\"";
    const size_t pad = MAXPATHLEN / 2 - before - (sizeof(head) - 1)
                       - (sizeof(mid) - 1);
    const size_t vlen = strlen(value);
    char *query = malloc(sizeof(head) + pad + sizeof(mid) + vlen + 3);
    char *p = query;

    if (query != NULL) {
        memcpy(p, head, sizeof(head) - 1);
        p += sizeof(head) - 1;
        memset(p, 'a', pad);
        p += pad;
        memcpy(p, mid, sizeof(mid) - 1);
        p += sizeof(mid) - 1;
        memcpy(p, value, vlen);
        memcpy(p + vlen, "\"c", 3);
    }

    return query;
}

/*!
 * @brief Check queries that convert to UTF-8 in pieces or a character at a time
 *
 * Latin-1 runs of wildcards and of letters longer than one conversion holds,
 * Latin-1 and Shift_JIS values across a piece's end, Mac Japanese characters
 * that decode to more units than one conversion holds, a long Shift_JIS run,
 * and a Shift_JIS character whose second byte is a backslash.
 */
static void test_converted_query(void)
{
    static const char head[] = "kMDItemFSName==\"\xe9";
    static const char tail[] = ".txt\"c";
    static const char either[] = "kMDItemFSName==\"*report*\"c"
                                 "||kMDItemFSName==\"";
    static const char sjis_either[] = "kMDItemFSName==\"*\x95\x5c*\"c"
                                      "||kMDItemFSName==\"*";
    static const char mac_either[] = "kMDItemFSName==\"report*\"c"
                                     "||kMDItemTextContent==\"";
    static const char latin_value[] = "\xe9t\xe9 0123456789 \xe0\xfc\xf1.txt";
    static const char sjis_value[] = "\x95\x5c\x95\x5c\x95\x5c\x95\x5c"
                                     "\x95\x5c\x95\x5c\x95\x5c\x95\x5c"
                                     "\x95\x5c\x95\x5c\x95\x5c\x95\x5c.txt";
    const size_t run = 5000;
    const size_t mac_run = 1000;
    char *query = malloc(sizeof(either) + run + sizeof(tail));
    struct sl_name_query *q = NULL;
    charset_t cs;

    if (query == NULL) {
        report(false, "a long Latin-1 query takes its memory");
    } else if ((cs = add_charset("ISO-8859-1")) == (charset_t) -1) {
        printf("ok %d - a long Latin-1 query # SKIP no ISO-8859-1\n",
               ++tests_run);
    } else {
        memcpy(query, head, sizeof(head) - 1);
        memset(query + sizeof(head) - 1, '*', run);
        memcpy(query + sizeof(head) - 1 + run, tail, sizeof(tail));
        q = sl_name_query_parse_in(query, 0, 8, cs);
        report(q != NULL && sl_name_query_match(q, "\xe9t\xe9.txt", 7),
               "a Latin-1 query longer than one conversion holds converts "
               "whole");
        sl_name_query_free(q);
        memcpy(query, either, sizeof(either) - 1);
        memset(query + sizeof(either) - 1, 'a', run);
        memcpy(query + sizeof(either) - 1 + run, "\"", 2);
        q = sl_name_query_parse_in(query, 0, 8, cs);
        report(q != NULL && sl_name_query_match(q, "my report.txt", 13),
               "a Latin-1 query with a run of letters longer than one "
               "conversion holds converts whole");
        sl_name_query_free(q);
        free(query);
        query = straddling_query(latin_value, 10);
        q = query != NULL ? sl_name_query_parse_in(query, 0, 8, cs) : NULL;
        report(q != NULL && sl_name_query_match(q, latin_value,
                                                strlen(latin_value)),
               "a Latin-1 value across the end of a piece converts byte for "
               "byte");
        sl_name_query_free(q);
    }

    free(query);

    /* 0x87 0xFB decodes to five units in Mac Japanese, U+F862 and 有限会社 */
    if ((cs = add_charset("MAC_JAPANESE")) == (charset_t) -1) {
        printf("ok %d - a Mac Japanese query # SKIP no MAC_JAPANESE\n",
               ++tests_run);
    } else if ((query = malloc(sizeof(mac_either) + 2 * mac_run + 2))
               != NULL) {
        memcpy(query, mac_either, sizeof(mac_either) - 1);

        for (size_t i = 0; i < mac_run; i++) {
            memcpy(query + sizeof(mac_either) - 1 + 2 * i, "\x87\xfb", 2);
        }

        memcpy(query + sizeof(mac_either) - 1 + 2 * mac_run, "\"", 2);
        q = sl_name_query_parse_in(query, 0, 8, cs);
        report(q != NULL && sl_name_query_match(q, "report.txt", 10),
               "a Mac Japanese query whose characters decode to more units "
               "than one conversion holds converts whole");
        sl_name_query_free(q);
        free(query);
    } else {
        report(false, "a long Mac Japanese query takes its memory");
    }

    /* 表 is 0x95 0x5C in Shift_JIS */
    if ((cs = add_charset("SHIFT_JIS")) == (charset_t) -1) {
        printf("ok %d - a Shift_JIS query # SKIP no Shift_JIS\n", ++tests_run);
        return;
    }

    q = sl_name_query_parse_in("kMDItemFSName==\"\x95\x5c\"c", 0, 8, cs);
    report(q != NULL && sl_name_query_match(q, "\x95\x5c", 2),
           "a Shift_JIS character whose second byte is a backslash escapes "
           "nothing");
    sl_name_query_free(q);

    /* the 41 bytes before the run put a first byte of 表 where the first
     * piece would end */
    if ((query = malloc(sizeof(sjis_either) + 2 * run + 2)) != NULL) {
        memcpy(query, sjis_either, sizeof(sjis_either) - 1);

        for (size_t i = 0; i < run; i++) {
            memcpy(query + sizeof(sjis_either) - 1 + 2 * i, "\x95\x5c", 2);
        }

        memcpy(query + sizeof(sjis_either) - 1 + 2 * run, "\"", 2);
        q = sl_name_query_parse_in(query, 0, 8, cs);
        report(q != NULL && sl_name_query_match(q, "\x95\x5c.txt", 6),
               "a Shift_JIS query longer than one conversion holds converts "
               "whole, a character never cut");
        sl_name_query_free(q);
        free(query);
    } else {
        report(false, "a long Shift_JIS query takes its memory");
    }

    /* the piece ends inside the sixth 表 */
    query = straddling_query(sjis_value, 11);
    q = query != NULL ? sl_name_query_parse_in(query, 0, 8, cs) : NULL;
    report(q != NULL && sl_name_query_match(q, sjis_value, strlen(sjis_value)),
           "a Shift_JIS value across the end of a piece converts byte for "
           "byte");
    sl_name_query_free(q);
    free(query);
}

/*!
 * @brief Run the checks and print the TAP plan
 *
 * @returns 0 when every check passed, 1 otherwise
 */
int main(void)
{
    printf("TAP version 13\n");
    test_values();
    test_like();
    test_queries();
    test_long_name();
    test_longer_name();
    test_dotted_i();
    test_policy();
    test_seed_choice();
    test_refused();
    test_term_query();
    test_value_query();
    test_like_holds();
    test_index_patterns();
    test_latin1();
    test_converted_query();
    test_unconverted_name();
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
