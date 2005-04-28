/*
 * $Id: uid.c,v 1.14 2005-04-28 20:49:45 bfernhomberg Exp $
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
#include <errno.h>
#include <atalk/logger.h>

/* functions for username and group */
#include <pwd.h>
#include <grp.h>
#include "uid.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

extern uid_t    uuid; 

void save_uidgid ( pair )
uidgidset *pair;
{
    pair->uid = geteuid ();
    pair->gid = getegid ();
} 

void restore_uidgid ( pair )
uidgidset *pair;
{
    uid_t uid
    gid_t gid;   
    
    uid = geteuid ();
    gid = getegid ();

    if (uid == pair->uid && gid == pair->gid)
       return;

    if (seteuid(0) < 0) {
        LOG(log_error, logtype_afpd, "set_uidgid: Could not switch back to root: %s",
				strerror(errno));
    }

    if ( setegid ( pair->gid ) < 0 )
        LOG(log_error, logtype_afpd, "restore_uidgid: unable to setegid '%s': %s",
            pair->gid, strerror(errno) );

    if ( seteuid ( pair->uid ) < 0 )
        LOG(log_error, logtype_afpd, "restore_uidgid: unable to seteuid '%s': %s",
            pair->uid, strerror(errno) );
    else
    	uuid = pair->uid;	/* ugly hack for utommode */
}

void set_uidgid ( this_volume )
const struct vol	*this_volume;
{
    int		uid, gid;   /* derived ones go in here */

    /* check to see if we have to switch users */
    uid = user_to_uid ( (this_volume)->v_forceuid);
    gid = group_to_gid ( (this_volume)->v_forcegid);

    if ((!uid || uid == geteuid()) && (!gid || gid == getegid()))
       return;

    if ( seteuid(0) < 0) {
        LOG(log_error, logtype_afpd, "set_uidgid: Could not switch back to root: %s",
				strerror(errno));
	return;
    }

    /* check to see if we have to switch groups */
    if ( gid ) {
        if ( setegid ( gid ) < 0 )
            LOG(log_error, logtype_afpd, "set_uidgid: unable to setegid '%s': %s",
                (this_volume)->v_forcegid, strerror(errno) );
    } /* end of checking for (this_volume)->v_forcegid */

    if ( uid) {
        if ( seteuid ( uid ) < 0 )
            LOG(log_error, logtype_afpd, "set_uidgid: unable to seteuid '%s': %s",
                (this_volume)->v_forceuid, strerror(errno) );
    	else
    	    uuid = uid;	/* ugly hack for utommode */

    } /* end of checking for (this_volume)->v_forceuid */

} /* end function void set_uidgid ( username, group ) */

int user_to_uid ( username )
char	*username;
{
    struct passwd *this_passwd;

    /* check for anything */
    if ( !username || strlen ( username ) < 1 ) return 0;

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

    /* check for anything */
    if ( !group || strlen ( group ) < 1 ) return 0;

    /* grab the /etc/groups record relating to group */
    this_group = getgrnam ( group );

    /* return false if there is no structure returned */
    if (this_group == NULL) return 0;

    /* return proper gid */
    return this_group->gr_gid;

} /* end function int group_to_gid ( group ) */

#endif /* FORCE_UIDGID */
