/*
  Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/*!
 * @file
 * @brief Typesafe, dynamic object store based on talloc
 *
 * Usage:
 *
 *
 * Define some terminal types:
 *
 * A key/value store aka dictionary that supports retrieving elements by key
 * @code
 *   typedef dict_t DALLOC_CTX;
 * @endcode
 *
 * An ordered set that can store different objects which can be retrieved by number
 * @code
 *   typedef set_t DALLOC_CTX;
 * @endcode
 *
 *
 * Create an dalloc object and add elementes of different type
 *
 * Allocate a new talloc context
 * @code
 *   TALLOC_CTX *mem_ctx = talloc_new(NULL);
 * @endcode
 *
 * Create a new dalloc object
 * @code
 *   DALLOC_CTX *d = talloc_zero(mem_ctx, DALLOC_CTX);
 * @endcode
 *
 * Store an int value in the object
 * @code
 *   uint64_t i = 1;
 *   dalloc_add_copy(d, &i, uint64_t);
 * @endcode
 *
 * Store a string
 * @code
 *   char *str = dalloc_strdup(d, "hello world");
 *   dalloc_add(d, str, char *);
 * @endcode
 *
 * Add a nested object, you later can't fetch this directly
 * @code
 *   DALLOC_CTX *nested = talloc_zero(d, DALLOC_CTX);
 *   dalloc_add(d, nested, DALLOC_CTX);
 * @endcode
 *
 * Add an int value to the nested object, this can be fetched
 * @code
 *   i = 2;
 *   dalloc_add_copy(nested, &i, uint64_t);
 * @endcode
 *
 * Add a nested set
 * @code
 *   set_t *set = talloc_zero(nested, set_t);
 *   dalloc_add(nested, set, set_t);
 * @endcode
 *
 * Add an int value to the set
 * @code
 *   i = 3;
 *   dalloc_add_copy(set, &i, uint64_t);
 * @endcode
 *
 * Add a dictionary (key/value store)
 * @code
 *   dict_t *dict = talloc_zero(nested, dict_t);
 *   dalloc_add(nested, dict, dict_t);
 * @endcode
 *
 * Store a string as key in the dict
 * @code
 *   str = dalloc_strdup(d, "key");
 *   dalloc_add(dict, str, char *);
 * @endcode
 *
 * Add a value for the key
 * @code
 *   i = 4;
 *   dalloc_add_copy(dict, &i, uint64_t);
 * @endcode
 *
 *
 * Fetching value references
 *
 * You can fetch anything that is not a DALLOC_CTXs, because passing
 * "DALLOC_CTXs" as type to the functions dalloc_get() and dalloc_value_for_key()
 * tells the function to step into that object and expect more arguments that specify
 * which element to fetch.
 *
 * Get reference to an objects element by position
 * @code
 *   uint64_t *p = dalloc_get(d, "uint64_t", 0);
 * @endcode
 * p now points to the first int with a value of 1
 *
 * Get reference to the "hello world" string
 * @code
 *   str = dalloc_get(d, "char *", 1);
 * @endcode
 *
 * You can't fetch a pure DALLOC_CTX
 * @code
 *   nested = dalloc_get(d, "DALLOC_CTX", 2);
 * @endcode
 * But you can do this
 * @code
 *   p = dalloc_get(d, "DALLOC_CTX", 2, "uint64_t", 0);
 * @endcode
 * p now points to the value 2
 *
 * You can fetch types that are typedefd DALLOC_CTXs
 * @code
 *   set = dalloc_get(d, "DALLOC_CTX", 2, "set_t", 1);
 * @endcode
 *
 * Fetch int from set, note that you must use DALLOC_CTX as type for the set
 * @code
 *   p = dalloc_get(d, "DALLOC_CTX", 2, "DALLOC_CTX", 1, "uint64_t", 0);
 * @endcode
 * p points to 3
 *
 * Fetch value by key from dictionary
 * @code
 *   p = dalloc_value_for_key(d, "DALLOC_CTX", 2, "DALLOC_CTX", 2, "key");
 * @endcode
 * p now point to 4
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <talloc.h>

#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/dalloc.h>

/*!
 * @brief Append an element to a dalloc array, the worker of dalloc_add and
 * dalloc_add_copy
 *
 * With a talloc chunk the call stores the bytes of obj in that chunk, as
 * dalloc_add_copy needs. Without one it stores obj itself, as dalloc_add
 * needs. The dalloc_add and dalloc_add_copy macros call it.
 *
 * @param[in,out] dd            dalloc array
 * @param[in]     talloc_chunk  chunk that receives the bytes of obj, or NULL
 * @param[in]     obj           object to store
 * @param[in]     size          size of obj
 * @returns 0
 */
int dalloc_add_talloc_chunk(DALLOC_CTX *dd, void *talloc_chunk, void *obj,
                            size_t size)
{
    if (talloc_chunk) {
        /* Called from dalloc_add_copy() macro */
        dd->dd_talloc_array = talloc_realloc(dd,
                                             dd->dd_talloc_array,
                                             void *,
                                             talloc_array_length(dd->dd_talloc_array) + 1);
        memcpy(talloc_chunk, obj, size);
        dd->dd_talloc_array[talloc_array_length(dd->dd_talloc_array) - 1] =
            talloc_chunk;
    } else {
        /* Called from dalloc_add() macro */
        dd->dd_talloc_array = talloc_realloc(dd,
                                             dd->dd_talloc_array,
                                             void *,
                                             talloc_array_length(dd->dd_talloc_array) + 1);
        dd->dd_talloc_array[talloc_array_length(dd->dd_talloc_array) - 1] = obj;
    }

    return 0;
}

/*!
 * @brief Return an element of a dalloc tree by a path of indexes and a type
 * name
 *
 * Each "DALLOC_CTX" argument with an index steps into a nested array. The
 * last type name and index select the element, which must carry that talloc
 * name.
 *
 * @param[in] d    dalloc tree
 * @param[in] ...  "DALLOC_CTX" and index pairs, then the type name and the
 *                 index of the element
 * @returns the element, or NULL for an index out of range or a type mismatch
 */
void *dalloc_get(const DALLOC_CTX *d, ...)
{
    EC_INIT;
    void *p = NULL;
    va_list args;
    const char *type;
    int elem;
    va_start(args, d);
    type = va_arg(args, const char *);

    while (STRCMP(type, ==, "DALLOC_CTX")) {
        elem = va_arg(args, int);

        if (elem >= talloc_array_length(d->dd_talloc_array)) {
            LOG(log_error, logtype_sl, "dalloc_get(%s): bound check error: %d >= %d",
                type, elem >= talloc_array_length(d->dd_talloc_array));
            EC_FAIL;
        }

        d = d->dd_talloc_array[elem];
        type = va_arg(args, const char *);
    }

    elem = va_arg(args, int);

    if (elem >= talloc_array_length(d->dd_talloc_array)) {
        LOG(log_error, logtype_sl, "dalloc_get(%s): bound check error: %d >= %d",
            type, elem,  talloc_array_length(d->dd_talloc_array));
        EC_FAIL;
    }

    if (!(p = talloc_check_name(d->dd_talloc_array[elem], type))) {
        LOG(log_error, logtype_sl, "dalloc_get(%d/%s): type mismatch: %s",
            type, elem, talloc_get_name(d->dd_talloc_array[elem]));
    }

    va_end(args);
EC_CLEANUP:

    if (ret != 0) {
        p = NULL;
    }

    return p;
}

/*!
 * @brief Return the value for a name in a dalloc dictionary, reached by a path
 * of indexes
 *
 * Each "DALLOC_CTX" argument with an index steps into a nested array. The
 * dictionary alternates string names and values, and the value must carry
 * the final type name.
 *
 * @param[in] d    dalloc tree
 * @param[in] ...  "DALLOC_CTX" and index pairs, then the name and the type name
 *                 of the value
 * @returns the value, or NULL for a missing name, a name that is not a string,
 *          or a type mismatch
 */
void *dalloc_value_for_key(const DALLOC_CTX *d, ...)
{
    EC_INIT;
    void *p = NULL;
    va_list args;
    const char *type = NULL;
    int elem;
    va_start(args, d);
    type = va_arg(args, const char *);

    while (STRCMP(type, ==, "DALLOC_CTX")) {
        elem = va_arg(args, int);
        AFP_ASSERT(elem < talloc_array_length(d->dd_talloc_array));
        d = d->dd_talloc_array[elem];
        type = va_arg(args, const char *);
    }

    for (elem = 0; elem + 1 < talloc_array_length(d->dd_talloc_array); elem += 2) {
        if (STRCMP(talloc_get_name(d->dd_talloc_array[elem]), !=, "char *")) {
            LOG(log_error, logtype_default, "dalloc_value_for_key: key not a string: %s",
                talloc_get_name(d->dd_talloc_array[elem]));
            EC_FAIL;
        }

        if (STRCMP((char *)d->dd_talloc_array[elem], ==, type)) {
            p = d->dd_talloc_array[elem + 1];
            break;
        }
    }

    if (p == NULL) {
        EC_FAIL;
    }

    type = va_arg(args, const char *);

    if (STRCMP(talloc_get_name(p), !=, type)) {
        p = NULL;
    }

EC_CLEANUP:
    va_end(args);

    if (ret != 0) {
        p = NULL;
    }

    return p;
}

/*!
 * @brief Duplicate a string in talloc memory under the "char *" type name
 *
 * The type name lets dalloc_get and dalloc_value_for_key return the string.
 *
 * @param[in] ctx     talloc parent
 * @param[in] string  NUL-terminated string
 * @returns the new string, or NULL when talloc fails
 */
char *dalloc_strdup(const void *ctx, const char *string)
{
    EC_INIT;
    char *p;
    EC_NULL(p = talloc_strdup(ctx, string));
    talloc_set_name(p, "char *");
EC_CLEANUP:

    if (ret != 0) {
        if (p) {
            talloc_free(p);
        }

        p = NULL;
    }

    return p;
}

/*!
 * @brief Duplicate up to n bytes of a string in talloc memory under the
 * "char *" type name
 *
 * The result always ends with a NUL.
 *
 * @param[in] ctx     talloc parent
 * @param[in] string  string
 * @param[in] n       byte limit, without the NUL
 * @returns the new string, or NULL when talloc fails
 */
char *dalloc_strndup(const void *ctx, const char *string, size_t n)
{
    EC_INIT;
    char *p;
    EC_NULL(p = talloc_strndup(ctx, string, n));
    talloc_set_name(p, "char *");
EC_CLEANUP:

    if (ret != 0) {
        if (p) {
            talloc_free(p);
        }

        p = NULL;
    }

    return p;
}
