/*
 * $Id: uid.c,v 1.10 2002-02-28 21:20:39 jmarcus Exp $
 * code: jeff@univrel.pr.uconn.edu
 *
 * These functions are abstracted here, so that all calls for resolving
 * user/group names can be centrally changed (good for OS dependant calls
 * across the package).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* don't compile this file at all unless FORCE_UIDGID is set */
#ifdef FORCE_UIDGID

#include <stdio.h>
#include <string.h>
#include <atalk/logger.h>

/* functions for username and group */
#include <pwd.h>
#include <grp.h>
#include "uid.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

void save_uidgid ( pair )
uidgidset **pair;
{
    /* allocate the memory */
    pair = malloc ( sizeof ( uidgidset ) );

    /* then assign the values */
    (*pair)->uid = geteuid ();
    (*pair)->gid = getegid ();
} /* end function void save_uidgid ( pair ) */

void restore_uidgid ( pair )
uidgidset **pair;
{
    if ( seteuid ( (*pair)->uid ) < 0 )
        LOG(log_error, logtype_default, "restore_uidgid: unable to seteuid '%s': %s",
            (*pair)->uid, strerror(errno) );
    if ( setegid ( (*pair)->gid ) < 0 )
        LOG(log_error, logtype_default, "restore_uidgid: unable to setegid '%s': %s",
            (*pair)->gid, strerror(errno) );
} /* end function void restore_uidgid ( pair ) */

void set_uidgid ( this_volume )
const struct vol	*this_volume;
{
    int		uid, gid;   /* derived ones go in here */

    /* check to see if we have to switch users */
    if ( uid = user_to_uid ( (this_volume)->v_forceuid ) ) {
        if ( seteuid ( uid ) < 0 )
            LOG(log_error, logtype_default, "set_uidgid: unable to seteuid '%s': %s",
                (this_volume)->v_forceuid, strerror(errno) );
    } /* end of checking for (this_volume)->v_forceuid */

    /* check to see if we have to switch groups */
    if ( gid = group_to_gid ( (this_volume)->v_forcegid ) ) {
        if ( seteuid ( gid ) < 0 )
            LOG(log_error, logtype_default, "set_uidgid: unable to setegid '%s': %s",
                (this_volume)->v_forcegid, strerror(errno) );
    } /* end of checking for (this_volume)->v_forcegid */

} /* end function void set_uidgid ( username, group ) */

int user_to_uid ( username )
char	*username;
{
    struct passwd *this_passwd;

    /* free memory for pointer */
    this_passwd = malloc ( sizeof ( struct passwd ) );

    /* check for anything */
    if ( strlen ( username ) < 1 ) return 0;

    /* grab the /etc/passwd record relating to username */
    this_passwd = getpwnam ( username );

    /* return false if there is no structure returned */
    if (this_passwd == NULL) return 0;

    /* return proper uid */
    return this_passwd->pw_uid;

} /* end function int user_to_uid ( username ) */

int group_to_gid ( group )
char	*group;
{
    struct group *this_group;

    /* free memory for pointer */
    this_group = malloc ( sizeof ( struct group ) );

    /* check for anything */
    if ( strlen ( group ) < 1 ) return 0;

    /* grab the /etc/groups record relating to group */
    this_group = getgrnam ( group );

    /* return false if there is no structure returned */
    if (this_group == NULL) return 0;

    /* return proper gid */
    return this_group->gr_gid;

} /* end function int group_to_gid ( group ) */

#endif /* FORCE_UIDGID */
