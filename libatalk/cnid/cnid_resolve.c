/*
 * $Id: cnid_resolve.c,v 1.4 2001-08-16 14:30:29 uhees Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

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

/* return the did/name pair corresponding to a CNID. */
char *cnid_resolve(void *CNID, cnid_t *id)
{
  CNID_private *db;
  DBT key, data;

  if (!(db = CNID) || !id || !(*id))
    return NULL;

  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));

  key.data = id;
  key.size = sizeof(*id);
  while ((errno = db->db_cnid->get(db->db_cnid, NULL, &key, &data, 0))) {
    if (errno == DB_LOCK_DEADLOCK)
      continue;

    if (errno != DB_NOTFOUND) 
      syslog(LOG_ERR, "cnid_resolve: can't get did/name");

    *id = 0;
    return NULL;
  }
  
  memcpy(id, (char *) data.data + CNID_DEVINO_LEN, sizeof(*id));
  return (char *) data.data + CNID_HEADER_LEN;
}
