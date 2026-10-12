/*
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief UAM module loading and the server services that papd offers UAMs
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <atalk/logger.h>
#include <sys/param.h>
#include <sys/time.h>

#include <atalk/asp.h>
#include <atalk/dsi.h>
#include <atalk/afp.h>
#include <atalk/util.h>

#include "uam_auth.h"

/* --- server uam functions -- */

/*!
 * @brief Load a UAM module for papd and run its setup
 *
 * The module has to export a server UAM under its file name without the
 * extension, such as uams_pam for uams_pam.so. Its uam_setup runs without a
 * server object, since papd has none. Each failure except a missing UAM
 * symbol logs.
 *
 * @param[in] path  module path
 * @param[in] name  module file name
 * @returns the module record, or NULL when the module does not open, lacks the
 *          UAM, or fails its setup
 */
struct uam_mod *uam_load(const char *path, const char *name)
{
    char buf[MAXPATHLEN + 1], *p;
    struct uam_mod *mod;
    void *module;

    if ((module = mod_open(path)) == NULL) {
        LOG(log_error, logtype_papd, "uam_load(%s): failed to load: %s", name,
            mod_error());
        return NULL;
    }

    if ((mod = (struct uam_mod *) malloc(sizeof(struct uam_mod))) == NULL) {
        LOG(log_error, logtype_papd, "uam_load(%s): malloc failed", name);
        goto uam_load_fail;
    }

    strlcpy(buf, name, sizeof(buf));

    if ((p = strchr(buf, '.'))) {
        *p = '\0';
    }

    if ((mod->uam_fcn = mod_symbol(module, buf)) == NULL) {
        goto uam_load_err;
    }

    if (mod->uam_fcn->uam_type != UAM_MODULE_SERVER) {
        LOG(log_error, logtype_papd,
            "uam_load(%s): attempted to load a non-server module",
            name);
        goto uam_load_err;
    }

    /* version check would go here */

    /* TODO: Temporary hack below to get this to compile with 3.x, we really should be passing an AFPObj as the first parameter */

    if (!mod->uam_fcn->uam_setup ||
            ((*mod->uam_fcn->uam_setup)(NULL, name) < 0)) {
        LOG(log_error, logtype_papd, "uam_load(%s): uam_setup failed", name);
        goto uam_load_err;
    }

    mod->uam_module = module;
    return mod;
uam_load_err:
    free(mod);
uam_load_fail:
    mod_close(module);
    return NULL;
}

/*!
 * @brief Run the cleanup function of a UAM module, close it, and free its entry
 */
void uam_unload(struct uam_mod *mod)
{
    if (mod->uam_fcn->uam_cleanup) {
        (*mod->uam_fcn->uam_cleanup)();
    }

    mod_close(mod->uam_module);
    free(mod);
}

/* -- client-side uam functions -- */

/*!
 * @brief Register a UAM of a module under a name and type
 *
 * In papd UAM_SERVER_PRINTAUTH passes one handler, the uam_printer function,
 * and a UAM_SERVER_LOGIN_EXT registration returns 1.
 */
int uam_register(const int type, const char *path, const char *name, ...)
{
    va_list ap;
    struct uam_obj *uam;
    int ret;

    if (!name) {
        return -1;
    }

    /* see if it already exists. */
    if ((uam = auth_uamfind(type, name, strlen(name)))) {
        if (strcmp(uam->uam_path, path)) {
            /* it exists, but it's not the same module. */
            LOG(log_error, logtype_papd, "uam_register: \"%s\" already loaded by %s",
                name, path);
            return -1;
        }

        uam->uam_count++;
        return 0;
    }

    /* allocate space for uam */
    if ((uam = calloc(1, sizeof(struct uam_obj))) == NULL) {
        return -1;
    }

    uam->uam_name = name;
    uam->uam_path = strdup(path);
    uam->uam_count++;
    va_start(ap, name);

    switch (type) {
    case UAM_SERVER_LOGIN:
        /* expect three arguments */
        uam->u.uam_login.login = va_arg(ap, void *);
        uam->u.uam_login.logincont = va_arg(ap, void *);
        uam->u.uam_login.logout = va_arg(ap, void *);
        break;

    case UAM_SERVER_CHANGEPW:
        /* one argument */
        uam->u.uam_changepw = va_arg(ap, void *);
        break;

    case UAM_SERVER_PRINTAUTH:
        /* x arguments */
        uam->u.uam_printer = va_arg(ap, void *);
        break;

    default:
        break;
    }

    va_end(ap);
    /* attach to other uams */
    ret = auth_register(type, uam);

    if (ret) {
        free(uam->uam_path);
        free(uam);
    }

    return ret;
}

/*!
 * @brief Drop a registration of a UAM and free the UAM after its last one
 */
void uam_unregister(const int type, const char *name)
{
    struct uam_obj *uam;

    if (!name) {
        return;
    }

    uam = auth_uamfind(type, name, strlen(name));

    if (!uam || --uam->uam_count > 0) {
        return;
    }

    auth_unregister(uam);
    free(uam->uam_path);
    free(uam);
}

/*!
 * @brief Answer a UAM request for a server or session value with nothing, for
 * UAMs loaded by papd
 *
 * In papd the call is a stub that sets nothing and returns 0 for every code,
 * since papd keeps no session values.
 */
int uam_afpserver_option(void *private _U_, const int what _U_,
                         void *option _U_, size_t *len _U_)
{
    return 0;
}

/* --- helper functions for plugin uams --- */

/*!
 * @brief Find the password entry of a login name for a UAM
 *
 * In papd the call skips the domain forms of afpd. After getpwnam fails, it
 * lowers the case of name in place. It then compares the name with the login
 * name and the first gecos field of every account.
 */
struct passwd *uam_getname(void *private _U_, char *name, const int len)
{
    struct passwd *pwent;
    char *user;
    int i;

    if ((pwent = getpwnam(name))) {
        return pwent;
    }

    for (i = 0; i < len; i++) {
        name[i] = tolower(name[i]);
    }

    setpwent();

    while ((pwent = getpwent())) {
        if ((user = strchr(pwent->pw_gecos, ','))) {
            *user = '\0';
        }

        user = pwent->pw_gecos;

        /* check against both the gecos and the name fields. the user
         * might have just used a different capitalization. */
        if ((strncasecmp(user, name, len) == 0) ||
                (strncasecmp(pwent->pw_name, name, len) == 0)) {
            strncpy(name, pwent->pw_name, len);
            break;
        }
    }

    endpwent();
    /* os x server doesn't keep anything useful if we do getpwent */
    return pwent ? getpwnam(name) : NULL;
}

/*!
 * @brief Check that a user may log in through a UAM
 *
 * In papd the shell check always runs, since papd.conf has no option to turn
 * it off, and an empty shell fails as well.
 */
int uam_checkuser(void *private _U_, const struct passwd *pwd)
{
    char *p;

    if (!pwd || !pwd->pw_shell || (*pwd->pw_shell == '\0')) {
        return -1;
    }

    while ((p = getusershell())) {
        if (strcmp(p, pwd->pw_shell) == 0) {
            break;
        }
    }

    endusershell();

    if (!p) {
        LOG(log_info, logtype_papd, "illegal shell %s for %s", pwd->pw_shell,
            pwd->pw_name);
        return -1;
    }

    return 0;
}
