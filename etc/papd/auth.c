/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief UAM registration and loading for papd printer authentication
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/util.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <atalk/logger.h>

#include "uam_auth.h"

static struct uam_mod uam_modules = {NULL, NULL, &uam_modules, &uam_modules};
static struct uam_obj uam_login = {"", "", 0, {{NULL, NULL, NULL}}, &uam_login,
    &uam_login
};
static struct uam_obj uam_changepw = {"", "", 0, {{NULL, NULL, NULL}}, &uam_changepw,
    &uam_changepw
};
static struct uam_obj uam_printer = {"", "", 0, {{NULL, NULL, NULL}}, &uam_printer,
    &uam_printer
};


/*!
 * @brief Write the names of the UAMs of a type into a buffer, one per line
 *
 * The names come in reverse order of registration, and a final line with `*`
 * ends the list. The call appends to uamnames with strcat and has no bound,
 * so the buffer has to hold every name.
 *
 * @param[in]     type      UAM_SERVER_LOGIN, UAM_SERVER_CHANGEPW, or
 *                          UAM_SERVER_PRINTAUTH
 * @param[in,out] uamnames  NUL-terminated buffer that receives the names
 * @returns 0, or -1 for an unknown type
 */
int getuamnames(const int type, char *uamnames)
{
    struct uam_obj *prev, *start;

    if (!(start = UAM_LIST(type))) {
        return -1;
    }

    prev = start;

    while ((prev = prev->uam_prev) != start) {
        strcat(uamnames, prev->uam_name);
        strcat(uamnames, "\n");
    }

    strcat(uamnames, "*\n");
    return 0;
}


/*!
 * @brief Find a registered UAM by name
 *
 * In papd each UAM type, UAM_SERVER_PRINTAUTH included, has a separate list.
 */
struct uam_obj *auth_uamfind(const int type, const char *name,
                             const int len)
{
    struct uam_obj *prev, *start;

    if (!name || !(start = UAM_LIST(type))) {
        return NULL;
    }

    prev = start;

    while ((prev = prev->uam_prev) != start)
        if (strndiacasecmp(prev->uam_name, name, len) == 0) {
            return prev;
        }

    return NULL;
}

/*!
 * @brief Add a UAM to the list of its type
 *
 * In papd UAM_SERVER_PRINTAUTH has a list as well.
 */
int auth_register(const int type, struct uam_obj *uam)
{
    struct uam_obj *start;

    if (!uam || !uam->uam_name || (*uam->uam_name == '\0')) {
        return -1;
    }

    if (!(start = UAM_LIST(type))) {
        return 1;
    }

    uam_attach(start, uam);
    return 0;
}

/*!
 * @brief Load the UAM modules that the am option of papd.conf names
 *
 * Each name in list, separated by commas, loads from the UAM directory. The
 * call skips a missing module without a log line. A module that fails to
 * open, is no server module, or fails its setup gets a log line from
 * uam_load.
 *
 * @param[in] path  UAM directory
 * @param[in] list  comma-separated module names, or NULL when no printer sets
 *                  am
 * @returns 0, or -1 for a NULL path or list, a path that is too long, or a list
 *          without names
 */
int auth_load(const char *path, const char *list)
{
    char name[MAXPATHLEN + 1], buf[MAXPATHLEN + 1], *p;
    struct uam_mod *mod;
    struct stat st;
    size_t len;

    if (!path || !list || (len = strlen(path)) > sizeof(name) - 2) {
        return -1;
    }

    strlcpy(buf, list, sizeof(buf));

    if ((p = strtok(buf, ",")) == NULL) {
        return -1;
    }

    strcpy(name, path);

    if (name[len - 1] != '/') {
        strcat(name, "/");
        len++;
    }

    while (p) {
        strlcpy(name + len, p, sizeof(name) - len);

        if ((stat(name, &st) == 0) && (mod = uam_load(name, p))) {
            uam_attach(&uam_modules, mod);
            LOG(log_info, logtype_papd, "uam: %s loaded", p);
        }

        p = strtok(NULL, ",");
    }

    return 0;
}

/*! get rid of all of the uams */
void auth_unload(void)
{
    struct uam_mod *mod, *prev, *start = &uam_modules;
    prev = start->uam_prev;

    while ((mod = prev) != start) {
        prev = prev->uam_prev;
        uam_detach(mod);
        uam_unload(mod);
    }
}
