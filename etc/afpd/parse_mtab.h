/* header for afpd_mtab_parse, afpd_st_cnid */

#ifndef _parse_mtab_h
#define _parse_mtab_h

/* set this to 31 to reserve the high-order bit for distinguishing files and
   directories.  */
#define AFPD_MTAB_DEV_AND_INODE_BITS 32
/* this gives us some stability when adding partitions. */
#define AFPD_MTAB_MIN_DEV_BITS 3

struct afpd_mtab_entry {
  /* Description of one partition.  */
  int id;		/* unique index (a small integer) */
  int dev_major, dev_minor;	/* device numbers */
  int bit_value;	/* preshifted device index field */
  char *device;		/* device name string (for debugging) */
  char *mount_point;	/* mounted directory (for debugging) */
};

struct afpd_mount_table {
  /* Description of all partitions.  */
  int size;		/* length of entries array, a power of 2 */
  int bits;		/* number of bits, log2(size) */
  unsigned int shift;	/* amount by which to shift into CNID device field */
  struct afpd_mtab_entry **table;	/* index -> entry map vector, some
					   entries may be null */
};

extern
unsigned int
afpd_st_cnid __P((struct stat *st));

extern
struct afpd_mount_table *
afpd_mtab_parse __P((char *file_name));

#endif /* not _parse_mtab_h */
