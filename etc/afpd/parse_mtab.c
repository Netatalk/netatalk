/*
 * $Id: parse_mtab.c,v 1.2 2001-06-10 20:15:00 srittau Exp $
 *
 * afpd_mtab_parse & support.  -- rgr, 9-Apr-01.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "directory.h"
#include "parse_mtab.h"

#define MAX_AFPD_MTAB_ENTRIES 256	/* unreasonably large number */
#define MTAB_DELIM_CHARS " \t\n"	/* delimiters for parsing */

#ifndef COPY_STRING
#define COPY_STRING(lval, str) \
	  (lval) = malloc(1+strlen(str)), strcpy((lval), (str))
#endif /* COPY_STRING */

/* global mount table; afpd_st_cnid uses this to lookup the right entry.  */
static struct afpd_mount_table *afpd_mount_table = NULL;

static int
ceil_log_2 __P((int n))
     /* Return the number of bits required to represent n.  Only works for
	positive n.  */
{
  int n_bits = 0;

  while (n) {
    n >>= 1;
    n_bits += 1;
  }
  return(n_bits);
}

unsigned int
afpd_st_cnid __P((struct stat *st))
     /* Given a stat structure, look up the device in afpd_mount_table, and
        compute and return a CNID.  */
{
  int id;
  struct afpd_mtab_entry *entry;

  if (afpd_mount_table != NULL) {
    for (id = 0; id < afpd_mount_table->size; id++) {
      entry = afpd_mount_table->table[id];
      if (entry != NULL
	  && entry->dev_major == major(st->st_dev)
	  && entry->dev_minor == minor(st->st_dev)) {
	return(entry->bit_value | st->st_ino);
      }
    }
  }

  /* Fallback. */
  return(st->st_ino);
}

struct afpd_mount_table *
afpd_mtab_parse __P((char *file_name))
     /* Parse the given mtab file, returning a new afpd_mount_table structure.
	Also saves it in the afpd_mount_table static variable for use by the
	afpd_st_cnid function.  Returns NULL if it encounters an error.

        [It would be great if this could generate a warning for any device that
        allows more inodes than we can allocate bits for.  -- rgr, 9-Apr-01.]
        */
{
  struct afpd_mtab_entry *entries[MAX_AFPD_MTAB_ENTRIES];	/* temp */
  int id, max_id = 0, n_errors = 0;
  struct stat st;
  char line[1000], *p, *index_tok, *dev_tok, *mount_tok;
  int line_number = 0;		/* one-based */
  FILE *f = fopen(file_name, "r");

  if (f == NULL) {
    fprintf(stderr, "Error:  Can't open %s:  code %d\n",
	    file_name, errno);
    return(NULL);
  }
  for (id = 0; id < MAX_AFPD_MTAB_ENTRIES; id++)
    entries[id] = NULL;
  while (fgets(line, sizeof(line), f) != NULL) {
    line_number++;
    /* flush comment */
    if ((p = strchr(line, '#')) != NULL)
      *p = '\0';
    /* get fields */
    index_tok = strtok(line, MTAB_DELIM_CHARS);
    dev_tok = strtok(NULL, MTAB_DELIM_CHARS);
    mount_tok = strtok(NULL, MTAB_DELIM_CHARS);
    if (mount_tok == NULL) {
      /* [warning here if index_tok nonempty?  -- rgr, 9-Apr-01.]  */
    }
    else if (id = strtol(index_tok, &p, 10),
	     *p != '\0') {
      fprintf(stderr, "afpd:%s:%d:  Non-integer device index value '%s'.\n",
	      file_name, line_number, index_tok);
      n_errors++;
    }
    else if (id < 1 || id > MAX_AFPD_MTAB_ENTRIES) {
      fprintf(stderr,
	      "afpd:%s:%d:  Expected an integer from 1 to %d, but got '%s'.\n",
	      file_name, line_number, MAX_AFPD_MTAB_ENTRIES-1, index_tok);
      n_errors++;
    }
    else if (entries[id] != NULL) {
      /* not unique. */
      fprintf(stderr,
	      "afpd:%s:%d:  Id %d is already taken for %s.\n",
	      file_name, line_number, id, entries[id]->device);
      n_errors++;
    }
    else if (stat(dev_tok, &st) != 0) {
      fprintf(stderr, "afpd:%s:%d:  Can't stat '%s':  code %d\n",
	      file_name, line_number, dev_tok, errno);
      n_errors++;
    }
    else if (! S_ISBLK(st.st_mode)) {
      fprintf(stderr, "afpd:%s:%d:  '%s' is not a block device.\n",
	      file_name, line_number, dev_tok);
      n_errors++;
    }
    else {
      /* make a new entry */
      struct afpd_mtab_entry *entry
	= (struct afpd_mtab_entry *) malloc(sizeof(struct afpd_mtab_entry));

      entry->id = id;
      entry->dev_major = major(st.st_rdev);
      entry->dev_minor = minor(st.st_rdev);
      COPY_STRING(entry->device, dev_tok);
      COPY_STRING(entry->mount_point, mount_tok);
      entries[id] = entry;
      if (id > max_id)
	max_id = id;
    }
  } /* next line */

  fclose(f);
  if (n_errors) {
    fprintf(stderr, "Got %d errors while reading %s; exiting.\n",
	    n_errors, file_name);
    return(NULL);
  }
  else {
    /* make the table. */
    struct afpd_mount_table *mount_table	/* return value */
      = (struct afpd_mount_table *) malloc(sizeof(struct afpd_mount_table));
    int n_bits = ceil_log_2(max_id);
    int table_size;
    struct afpd_mtab_entry **new_entries;

    if (n_bits < AFPD_MTAB_MIN_DEV_BITS)
      n_bits = AFPD_MTAB_MIN_DEV_BITS;
    table_size = 1 << n_bits;
    new_entries = (struct afpd_mtab_entry **)
      malloc(table_size*sizeof(struct afpd_mtab_entry *));
    mount_table->size = table_size;
    mount_table->bits = n_bits;
    mount_table->shift = AFPD_MTAB_DEV_AND_INODE_BITS-n_bits;
    for (id = 0; id < table_size; id++) {
      if (entries[id] != NULL)
	entries[id]->bit_value = entries[id]->id << mount_table->shift;
      new_entries[id] = entries[id];
    }
    mount_table->table = new_entries;

    afpd_mount_table = mount_table;
    return(mount_table);
  }
}
