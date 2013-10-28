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
  @file
  Typesafe, dynamic object store based on talloc
 
  Usage:

  //
  // Define some terminal types:
  //

  // A key/value store aka dictionary that supports retrieving elements by key
  typedef dict_t DALLOC_CTX;

  // An ordered set that can store different objects which can be retrieved by number
  typedef set_t DALLOC_CTX;

  //
  // Create an dalloc object and add elementes of different type
  //

  // Allocate a new talloc context
  TALLOC_CTX *mem_ctx = talloc_new(NULL);
  // Create a new dalloc object
  DALLOC_CTX *d = talloc_zero(mem_ctx, DALLOC_CTX);
 
  // Store an int value in the object
  uint64_t i = 1;
  dalloc_add_copy(d, &i, uint64_t);
 
  // Store a string
  char *str = dalloc_strdup(d, "hello world");
  dalloc_add(d, str, char *);
 
  // Add a nested object, you later can't fetch this directly
  DALLOC_CTX *nested = talloc_zero(d, DALLOC_CTX);
  dalloc_add(d, nested, DALLOC_CTX);

  // Add an int value to the nested object, this can be fetched
  i = 2;
  dalloc_add_copy(nested, &i, uint64_t);

  // Add a nested set
  set_t *set = talloc_zero(nested, set_t);
  dalloc_add(nested, set, set_t);
 
  // Add an int value to the set
  i = 3;
  dalloc_add_copy(set, &i, uint64_t);

  // Add a dictionary (key/value store)
  dict_t *dict = talloc_zero(nested, dict_t);
  dalloc_add(nested, dict, dict_t);

  // Store a string as key in the dict
  str = dalloc_strdup(d, "key");
  dalloc_add(dict, str, char *);

  // Add a value for the key
  i = 4;
  dalloc_add_copy(dict, &i, uint64_t);

  //
  // Fetching value references
  // You can fetch anything that is not a DALLOC_CTXs, because passing
  // "DALLOC_CTXs" as type to the functions dalloc_get() and dalloc_value_for_key()
  // tells the function to step into that object and expect more arguments that specify
  // which element to fetch.
  //

  // Get reference to an objects element by position
  uint64_t *p = dalloc_get(d, "uint64_t", 0);
  // p now points to the first int with a value of 1

  // Get reference to the "hello world" string
  str = dalloc_get(d, "char *", 1);

  // You can't fetch a pure DALLOC_CTX
  nested = dalloc_get(d, "DALLOC_CTX", 2);
  // But you can do this
  p = dalloc_get(d, "DALLOC_CTX", 2, "uint64_t", 0);
  // p now points to the value 2

  // You can fetch types that are typedefd DALLOC_CTXs
  set = dalloc_get(d, "DALLOC_CTX", 2, "set_t", 1);

  // Fetch int from set, note that you must use DALLOC_CTX as type for the set
  p = dalloc_get(d, "DALLOC_CTX", 2, "DALLOC_CTX", 1, "uint64_t", 0);
  // p points to 3

  // Fetch value by key from dictionary
  p = dalloc_value_for_key(d, "DALLOC_CTX", 2, "DALLOC_CTX", 2, "key");
  // p now point to 4
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>

#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/talloc.h>
#include <atalk/bstrlib.h>
#include <atalk/dalloc.h>

/* Use dalloc_add_copy() macro, not this function */
int dalloc_add_talloc_chunk(DALLOC_CTX *dd, void *talloc_chunk, void *obj, size_t size)
{
    if (talloc_chunk) {
        /* Called from dalloc_add_copy() macro */
        dd->dd_talloc_array = talloc_realloc(dd,
                                             dd->dd_talloc_array,
                                             void *,
                                             talloc_array_length(dd->dd_talloc_array) + 1);
        memcpy(talloc_chunk, obj, size);
        dd->dd_talloc_array[talloc_array_length(dd->dd_talloc_array) - 1] = talloc_chunk;
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

/* Get number of elements, returns 0 if the structure is empty or not initialized */
int dalloc_size(DALLOC_CTX *d)
{
    if (!d || !d->dd_talloc_array)
        return 0;
    return talloc_array_length(d->dd_talloc_array);
}

/*
 * Get pointer to value from a DALLOC object
 *
 * Returns pointer to object from a DALLOC object. Nested object interation
 * is supported by using the type string "DALLOC_CTX". Any other type string
 * designates the requested objects type.
 */
void *dalloc_get(const DALLOC_CTX *d, ...)
{
    EC_INIT;
    void *p = NULL;
    va_list args;
    const char *type;
    int elem;
    const char *elemtype;

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
    if (ret != 0)
        p = NULL;
    return p;
}

void *dalloc_value_for_key(const DALLOC_CTX *d, ...)
{
    EC_INIT;
    void *p = NULL;
    va_list args;
    const char *type;
    int elem;
    const char *elemtype;
    char *s;

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
    va_end(args);

EC_CLEANUP:
    if (ret != 0)
        p = NULL;
    return p;
}

char *dalloc_strdup(const void *ctx, const char *string)
{
    EC_INIT;
    char *p;

    EC_NULL( p = talloc_strdup(ctx, string) );
    talloc_set_name(p, "char *");

EC_CLEANUP:
    if (ret != 0) {
        if (p)
            talloc_free(p);
        p = NULL;
    }
    return p;
}

char *dalloc_strndup(const void *ctx, const char *string, size_t n)
{
    EC_INIT;
    char *p;

    EC_NULL( p = talloc_strndup(ctx, string, n) );
    talloc_set_name(p, "char *");

EC_CLEANUP:
    if (ret != 0) {
        if (p)
            talloc_free(p);
        p = NULL;
    }
    return p;
}
