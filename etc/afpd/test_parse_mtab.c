/*
 * $Id: test_parse_mtab.c,v 1.1 2001-05-25 16:18:09 rufustfirefly Exp $
 * test driver for the afpd_mtab_parse fn.  -- rgr, 9-Apr-01.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#include "directory.h"
#include "parse_mtab.h"

/* hack.  etc/afpd/directory.h would need to be patched accordingly.  this
   version ignores the filep stuff.  based on the comment starting around
   line 200 in etc/afpd/file.c (search for "fucking mess"), this may be OK.
   */
#ifdef CNID
#undef CNID
#endif /* ! CNID */

/* keep-filep version.  must also decrement AFPD_MTAB_DEV_AND_INODE_BITS.  */
/* #define CNID(pst, filep)	(afpd_st_cnid(pst) | CNID_FILE(filep)) */
#define CNID(pst, filep)	(afpd_st_cnid(pst))

int
main(int argc, char **argv)
{
  char *file_name = "afpd.mtab";
  struct afpd_mount_table *table = NULL;
  int arg = 1;

  if (argc >= 3 && strcmp(argv[1], "-f") == 0) {
    file_name = argv[2];
    arg += 2;
  }
  if (arg < argc && argv[arg][0] == '-') {
    /* either they don't understand, or they must be asking for help anyway. */
    fprintf(stderr, "Usage:  %s [-f file-name] [file . . . ]\n",
	    argv[0]);
    exit(1);
  }
  table = afpd_mtab_parse(file_name);
  if (table != NULL) {
    struct stat st;
    int id, count = 0;

    fprintf(stderr, "File %s:  Field width %d, shift %d.\n",
	    file_name, table->bits, table->shift);
    for (id = 0; id < table->size; id++) {
      struct afpd_mtab_entry *entry = table->table[id];

      if (entry != NULL) {
	fprintf(stderr, "  Id %d -> device %s (%d,%d) on %s -> bits 0x%08x.\n",
		id, entry->device, entry->dev_major, entry->dev_minor,
		entry->mount_point, entry->bit_value);
	count++;
      }
    }
    fprintf(stderr, "%d entries used out of a maximum of %d (1..%d).\n",
	    count, table->size-1, table->size-1);
    for (; arg < argc; arg++) {
      if (lstat(argv[arg], &st) != 0) {
	fprintf(stderr, "Can't lstat '%s':  code %d\n",
		argv[arg], errno);
      }
      else {
	fprintf(stderr, "File %s, device (%d,%d), inode %d, CNID 0x%08x.\n",
		argv[arg], major(st.st_dev), minor(st.st_dev), st.st_ino,
		CNID(&st, S_ISDIR(st.st_mode) ? 1 : 0));
      }
    }
  }
  return(0);
}
