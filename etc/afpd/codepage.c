/*
 * $Id: codepage.c,v 1.5 2001-06-20 18:33:04 rufustfirefly Exp $
 *
 * Copyright (c) 2000 Adrian Sun
 * All Rights Reserved. See COPYRIGHT.
 *
 * codepage support (based initially on some code from
 * julian@whistle.com)
 *
 * the strategy:
 * for single-byte maps, the codepage support devolves to a lookup
 * table for all possible 8-bit values. for double-byte maps, 
 * the first byte is used as a hash index followed by a linked list of
 * values.
 *
 * the badumap specifies illegal characters. these are 8-bit values
 * with an associated rule field. here are the rules:
 *   
 * illegal values: 0 is the only illegal value. no translation will
 * occur in those cases.  
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <syslog.h>

#include <netatalk/endian.h>

#include "globals.h"
#include "volume.h"
#include "codepage.h"

#define MAPSIZE 256

/* deal with linked lists */
#define CP_INIT(a) (a)->next = (a)->prev = (a)
#define CP_ADD(a, b) do { \
  (b)->next = (a)->next; \
  (b)->prev = (a); \
  (a)->next = (b); \
} while (0)
#define CP_REMOVE(a) do {  \
  (a)->prev->next = (a)->next; \
  (a)->next->prev = (a)->prev; \
} while (0)

/* search for stuff */
#if 0
static __inline__ unsigned char *codepage_find(struct codepage *page,
					       unsigned char *from)
{
}
#endif

static int add_code(struct codepage *page, unsigned char *from,
		    unsigned char *to)
{
#if 0
  union codepage_val *ptr;
#endif /* 0 */

  if (page->quantum < 1) /* no quantum given. don't do anything */
    return 1;

  if (page->quantum == 1) {
    page->map[*from].value = *to;
    return 0;
  }

#if 0
  if (ptr = codepage_find(page->map, from)) {
    
  } else {
    unsigned char *space;
    ptr = map[*from].hash;

    space = (unsigned char *) malloc(sizeof(unsigned char)*quantum);
    if (!ptr->from) {
    } else {
    }
    
    map[*from].hash
  }
#endif /* 0 */
  return 0;
}

static struct codepage *init_codepage(const int quantum)
{
    struct codepage *cp;

    cp = (struct codepage *) malloc(sizeof(struct codepage));
    if (!cp)
      return NULL;

    if ((cp->map = (union codepage_val *)
	 calloc(MAPSIZE, sizeof(union codepage_val))) == NULL) {
      free(cp);
      return NULL;
    }

    cp->quantum = quantum;
    return cp;
}


static void free_codepage(struct codepage *cp)
{
  int i;

  if (!cp)
    return;

  if (cp->map) {
    if (cp->quantum > 1) {
      /* deal with any linked lists that may exist */
      for (i = 0; i < MAPSIZE; i++) {
	struct codepage_hash *ptr, *h;
	
	h = &cp->map[i].hash; /* we don't free this one */
	while ((ptr = h->prev) != h) {
	  CP_REMOVE(ptr);
	  free(ptr);
	}
      }
    }
    free(cp->map);
  }
  free(cp);
}



/* this is used by both codepages and generic mapping utilities. we
 * allocate enough space to map all 8-bit characters if necessary.
 * for double-byte mappings, we just use the table as a hash lookup.
 * if we don't match, we don't convert.
 */
int codepage_init(struct vol *vol, const int rules, 
		  const int quantum)
{
  if ((rules & CODEPAGE_RULE_UTOM) && !vol->v_utompage) {
    vol->v_utompage = init_codepage(quantum);
    if (!vol->v_utompage)
      goto err_utompage;
  }

  if ((rules & CODEPAGE_RULE_MTOU) && !vol->v_mtoupage) {
    vol->v_mtoupage = init_codepage(quantum);
    if (!vol->v_mtoupage) {
      goto err_mtoupage;
    }
  }

  if ((rules & CODEPAGE_RULE_BADU)  && !vol->v_badumap) {
    vol->v_badumap = init_codepage(quantum);
    if (!vol->v_badumap)
      goto err_mtoupage;
  }
  return 0;

err_mtoupage:
    free_codepage(vol->v_mtoupage);
    vol->v_mtoupage = NULL;

err_utompage:
    free_codepage(vol->v_utompage);
    vol->v_utompage = NULL;
    return -1;
}

void codepage_free(struct vol *vol)
{
  if (vol->v_utompage) {
    free_codepage(vol->v_utompage);
    vol->v_utompage = NULL;
  }

  if (vol->v_mtoupage) {
    free_codepage(vol->v_mtoupage);
    vol->v_mtoupage = NULL;
  }

  if (vol->v_badumap) {
    free_codepage(vol->v_badumap);
    vol->v_badumap = NULL;
  }
}


int codepage_read(struct vol *vol, const char *path)
{
  unsigned char buf[CODEPAGE_FILE_HEADER_SIZE], *cur;
  u_int16_t id;
  int fd, i, quantum, rules;
  
  if ((fd = open(path, O_RDONLY)) < 0) {
    syslog(LOG_ERR, "%s: failed to open codepage", path);
    return -1;
  }
  
  /* Read the codepage file header. */
  if(read(fd, buf, sizeof(buf)) != sizeof(buf)) {
    syslog( LOG_ERR, "%s: failed to read codepage header", path);
    goto codepage_fail;
  }

  /* Check the file id */
  cur = buf;
  memcpy(&id, cur, sizeof(id));
  cur += sizeof(id);
  id = ntohs(id);
  if (id != CODEPAGE_FILE_ID) {
      syslog( LOG_ERR, "%s: not a codepage", path);
      goto codepage_fail;
  } 

  /* check the version number */
  if (*cur++ != CODEPAGE_FILE_VERSION) {
      syslog( LOG_ERR, "%s: codepage version not supported", path);
      goto codepage_fail;
  } 

  /* ignore namelen */
  cur++;

  /* find out the data quantum size. default to 1 if nothing's given. */
  quantum = *cur ? *cur : 1;
  cur++;
  
  /* rules used in this file. */
  rules = *cur++;

  if (codepage_init(vol, rules, quantum) < 0) {
    syslog( LOG_ERR, "%s: Unable to allocate memory", path);
    goto codepage_fail;
  }

  /* offset to data */

  /* skip to the start of the data */
  memcpy(&id, cur , sizeof(id));
  id = ntohs(id);
  lseek(fd, id, SEEK_SET);

  /* mtoupage is the the equivalent of samba's unix2dos. utompage is
   * the equivalent of dos2unix. it's a little confusing due to a
   * desire to match up with mtoupath and utompath. 
   * NOTE: we allow codepages to specify 7-bit mappings if they want. 
   */
  i = 1 + 2*quantum;
  while (read(fd, buf, i) == i) {
    if (*buf & CODEPAGE_RULE_MTOU) {
      if (add_code(vol->v_mtoupage, buf + 1, buf + 1 + quantum) < 0) {
	syslog(LOG_ERR, "unable to allocate memory for mtoupage");
	break;
      }
    }

    if (*buf & CODEPAGE_RULE_UTOM) {
      if (add_code(vol->v_utompage, buf + 1 + quantum, buf + 1) < 0) {
	syslog(LOG_ERR, "unable to allocate memory for utompage");
	break;
      }
    }
      
    /* we only look at the first character here. if we need to 
     * do so, we can always use the quantum to expand the 
     * available flags. */
    if (*buf & CODEPAGE_RULE_BADU) 
      vol->v_badumap->map[*(buf + 1)].value = *(buf + 1 + quantum);
  }
  close(fd);
  return 0;

codepage_fail:
  close(fd);
  return -1;
}
