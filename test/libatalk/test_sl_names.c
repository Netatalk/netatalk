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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <atalk/sl_names.h>

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
    /* comparisons of other modifiers fold the name apart */
    /* without the trailing '*' the escaped quotes are quotes */
    {"kMDItemFSName==\"\\\"two words\\\"\"cd", "\"two words\"", true},
    {"kMDItemFSName==\"\\\"two words\\\"\"cd", "two words", false},
    {"kMDItemFSName==\"*\\\"two words\\\"\"cd", "my \"two words\"", true},
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
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
