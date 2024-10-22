/* ---------------------------------------
*/
extern int delete_unix_adouble(char *path, char *name);
extern int delete_unix_dir(char *path, char *name);
extern int folder_with_ro_adouble(uint16_t vol, int did, char *name, char *file);
extern int delete_ro_adouble(uint16_t vol, int did, char *file);

extern int delete_unix_md(char *path, char *name, char *file);
extern int delete_unix_rf(char *path, char *name, char *file);
extern int delete_unix_file(char *path, char *name, char *file);
extern int rename_unix_file(char *path, char *dir, char *src, char *dst);

extern int unlink_unix_file(char *path, char *name, char *file);
extern int symlink_unix_file(char *target, char *path, char *source);

extern int chmod_unix_meta(char *path, char *name, char *file, mode_t mode);
extern int chmod_unix_rfork(char *path, char *name, char *file, mode_t mode);
/* -------------------
*/
