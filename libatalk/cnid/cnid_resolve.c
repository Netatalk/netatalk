/*
 * $Id: cnid_resolve.c,v 1.8 2001-10-21 08:33:33 jmarcus Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_DB
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>

#include <db.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "cnid_private.h"

/* Return the did/name pair corresponding to a CNID. */
char *cnid_resolve(void *CNID, cnid_t *id) {
	CNID_private *db;
	DBT key, data;
	int rc;

	if (!(db = CNID) || !id || !(*id)) {
		return NULL;
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.data = id;
	key.size = sizeof(cnid_t);
	while ((rc = db->db_cnid->get(db->db_cnid, NULL, &key, &data, 0))) {
		if (rc == DB_LOCK_DEADLOCK) {
			continue;
		}

		if (rc != DB_NOTFOUND) {
			syslog(LOG_ERR, "cnid_resolve: Unable to get did/name: %s",
			       db_strerror(rc));
		}

		*id = 0;
		return NULL;
	}

	memcpy(id, (char *)data.data + CNID_DEVINO_LEN, sizeof(cnid_t));
#ifdef DEBUG
	syslog(LOG_INFO, "cnid_resolve: Returning id = %u, did/name = %s",
	       ntohl(*id), (char *)data.data + CNID_HEADER_LEN);
#endif
	return (char *)data.data + CNID_HEADER_LEN;
}
#endif /* CNID_DB */
