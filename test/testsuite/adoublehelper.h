/* ---------------------------------------
*/
extern int delete_unix_adouble(char *path, char *name);
extern int delete_unix_dir(char *path, char *name);
extern int folder_with_ro_adouble(uint16_t vol, int did, char *name,
                                  char *file);
extern int delete_ro_adouble(uint16_t vol, int did, char *file);

extern int delete_unix_md(char *path, char *name, char *file);
extern int delete_unix_rf(char *path, char *name, char *file);
extern int delete_unix_file(char *path, char *name, char *file);
extern int rename_unix_file(char *path, char *dir, char *src, char *dst);

extern int unlink_unix_file(char *path, char *name, char *file);
extern int symlink_unix_file(char *target, char *path, char *source);

extern int chmod_unix_meta(char *path, char *name, char *file, mode_t mode);
extern int chmod_unix_rfork(char *path, char *name, char *file, mode_t mode);

/* Names for our Extended Attributes adouble data
   Keep in sync with <atalk/ea.h> */
#define AD_EA_META "org.netatalk.Metadata"
#define AD_EA_META_LEN (sizeof(AD_EA_META) - 1)
#ifdef __APPLE__
#define AD_EA_RESO "com.apple.ResourceFork"
#define EA_FINFO "com.apple.FinderInfo"
#define NOT_NETATALK_EA(a) (strcmp((a), AD_EA_META) != 0) && (strcmp((a), AD_EA_RESO) != 0) && (strcmp((a), EA_FINFO) != 0)
#else
#define AD_EA_RESO "org.netatalk.ResourceFork"
#define NOT_NETATALK_EA(a) (strcmp((a), AD_EA_META) != 0) && (strcmp((a), AD_EA_RESO) != 0)
#endif

/****************************************************************************************
 * Wrappers for native EA functions taken from Samba
 ****************************************************************************************/
int sys_lremovexattr(const char *path, const char *name);

/* -------------------
*/
