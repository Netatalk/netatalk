/* ----------------------------------------------
*/
#include "specs.h"
#include "adoublehelper.h"

static char temp[MAXPATHLEN];
static char temp1[MAXPATHLEN];

/* --------------------
   delete metadata
*/
int delete_unix_md(char *path, char *name, char *file)
{
    if (adouble == AD_V2) {
        if (!*file) {
            sprintf(temp, "%s/%s/.AppleDouble/.Parent", path, name);
        }
        else {
            sprintf(temp, "%s/%s/.AppleDouble/%s", path, name, file);
        }
        if (!Quiet) {
            fprintf(stdout,"unlink(%s)\n", temp);
        }
        if (unlink(temp) <0) {
			if (!Quiet) {
                fprintf(stdout,"\tFAILED unlink(%s) %s\n", temp, strerror(errno));
            }
            return -1;
        }
    } else {
        sprintf(temp, "%s/%s/%s", path, name, file);
        if (sys_lremovexattr(temp, AD_EA_META) != 0) {
			if (!Quiet) {
                fprintf(stdout,"\tFAILED sys_lremovexattr(%s, %s) %s\n", temp, AD_EA_META, strerror(errno));
            }
            return -1;
        }
    }

	return 0;
}

/* --------------------
   delete a resource fork
*/
int delete_unix_rf(char *path, char *name, char *file)
{
    if (adouble == AD_V2) {
        if (!*file) {
            sprintf(temp, "%s/%s/.AppleDouble/.Parent", path, name);
        }
        else {
            sprintf(temp, "%s/%s/.AppleDouble/%s", path, name, file);
        }
        if (!Quiet) {
            fprintf(stdout,"unlink(%s)\n", temp);
        }
        if (unlink(temp) <0) {
			if (!Quiet) {
                fprintf(stdout,"\tFAILED unlink(%s) %s\n", temp, strerror(errno));
            }
            return -1;
        }
    } else {
#ifdef HAVE_EAFD
        if (file) {
            sprintf(temp, "%s/%s/%s", path, name, file);
            if (sys_lremovexattr(temp, AD_EA_RESO) != 0) {
                if (!Quiet) {
                    fprintf(stdout,"\tFAILED sys_lremovexattr(%s, %s) %s\n", temp, AD_EA_RESO, strerror(errno));
                }
                return -1;
            }
        }
#else
        if (file) {
            sprintf(temp, "%s/%s/._%s", path, name, file);
			if (!Quiet) {
                fprintf(stdout,"unlink(%s)\n", temp);
            }
            if (unlink(temp) <0) {
                if (!Quiet) {
                    fprintf(stdout,"\tFAILED unlink(%s) %s\n", temp, strerror(errno));
                }
                return -1;
            }
        }
#endif
    }

	return 0;
}

/* ----------------------
 * delete a file
*/
int delete_unix_file(char *path, char *name, char *file)
{
    int rc = 0;

    if (delete_unix_rf(path, name, file))
		rc = -1;

    sprintf(temp, "%s/%s/%s", path, name, file);
    if (!Quiet) {
        fprintf(stdout,"unlink(%s)\n", temp);
    }
	if (unlink(temp) <0) {
        if (!Quiet) {
            fprintf(stdout,"\tFAILED unlink(%s) %s\n", temp, strerror(errno));
        }
        rc = -1;
	}
	return rc;
}

/* Rename a file and its resource fork */
int rename_unix_file(char *path, char *dir, char *src, char *dst)
{
    sprintf(temp, "%s/%s/%s", Path, dir, src);
    sprintf(temp1, "%s/%s/%s", Path, dir, dst);
    if (!Quiet) {
        fprintf(stdout,"rename %s %s\n", temp, temp1);
    }
    if (rename(temp, temp1) < 0) {
        if (!Quiet) {
            fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
        }
        test_failed();
        return -1;
    }

    if (adouble == AD_V2) {
        sprintf(temp, "%s/%s/.AppleDouble/%s", Path, dir, src);
        sprintf(temp1,"%s/%s/.AppleDouble/%s", Path, dir, dst);
        if (!Quiet) {
            fprintf(stdout,"rename %s %s\n", temp, temp1);
        }
        if (rename(temp, temp1) < 0) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
            }
            test_failed();
            return -1;
        }
    } else {
#ifndef HAVE_EAFD
        sprintf(temp, "%s/%s/._%s", Path, dir, src);
        sprintf(temp1,"%s/%s/._%s", Path, dir, dst);
        if (!Quiet) {
            fprintf(stdout,"rename %s %s\n", temp, temp1);
        }
        if (rename(temp, temp1) < 0) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
            }
            test_failed();
            return -1;
        }

#endif
    }

    return 0;
}

/* unlink file only, dont care about adouble file */
int unlink_unix_file(char *path, char *name, char *file)
{
	sprintf(temp, "%s/%s/%s", path, name, file);
    if (!Quiet) {
        fprintf(stdout,"unlink(%s)\n", temp);
    }
	if (unlink(temp) <0) {
        if (!Quiet) {
            fprintf(stdout,"\tFAILED unlink(%s) %s\n", temp, strerror(errno));
        }
		test_failed();
		return -1;
	}
	return 0;
}

/* ----------------------------- */
int symlink_unix_file(char *target, char *path, char *source)
{
	sprintf(temp, "%s/%s", path, source);
    if (!Quiet) {
        fprintf(stdout,"symlink(%s -> %s)\n", temp, target);
    }
	if (symlink(target, temp) <0) {
        if (!Quiet) {
            fprintf(stdout,"\tFAILED symlink(%s -> %s) %s\n", temp, target, strerror(errno));
        }
		test_failed();
		return -1;
	}
	return 0;
}

/* Delete metadata of directory */
int delete_unix_adouble(char *path, char *name)
{
    if (adouble == AD_EA) {
        sprintf(temp, "%s/%s", path, name);
        sys_lremovexattr(temp, AD_EA_META);
        sys_lremovexattr(temp, AD_EA_RESO);
    } else {
        if (!Quiet) {
            fprintf(stdout,"rmdir(%s/.AppleDouble) \n", name);
        }
        sprintf(temp, "%s/%s/.AppleDouble/.Parent", path, name);
        if (unlink(temp) <0) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED unlink(%s) %s\n", temp, strerror(errno));
            }
            test_failed();
            return -1;
        }
        sprintf(temp, "%s/%s/.AppleDouble", path, name);
        if (rmdir(temp) <0) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED rmdir(%s) %s\n", temp, strerror(errno));
            }
            test_failed();
            return -1;
        }
    }
    return 0;
}

/* chmod .AppleDouble directory */
static int chmod_unix_adouble(char *path,char *name, int mode)
{
    if (adouble == AD_EA)
        return 0;

	sprintf(temp, "%s/%s/.AppleDouble", path, name);
    if (!Quiet) {
        fprintf(stdout, "chmod (%s, %o)\n", temp, mode);
    }
	if (chmod(temp, mode)) {
        if (!Quiet) {
            fprintf(stdout,"\tFAILED %s\n", strerror(errno));
        }
		test_failed();
		return -1;
	}
	return 0;
}

/* --------------------
*/
int chmod_unix_meta(char *path, char *name, char *file, mode_t mode)
{
    if (adouble == AD_EA) {
#if defined (HAVE_EAFD) && defined (SOLARIS)
        sprintf(temp, "runat '%s/%s/%s' chmod 0%o %s", path, name, file, mode, AD_EA_META);
        if (!Quiet) {
            fprintf(stdout, "%s\n", temp);
        }
        if (system(temp) != 0) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED %s\n", strerror(errno));
            }
            test_failed();
            return -1;
        }
        return 0;
#else
        return 0;
#endif
    } else {
        sprintf(temp, "%s/%s/.AppleDouble/%s", path, name, file);
        if (!Quiet) {
            fprintf(stdout, "chmod (%s, 0%o)\n", temp, mode);
        }
        if (chmod(temp, mode)) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED %s\n", strerror(errno));
            }
            test_failed();
            return -1;
        }
        return 0;
    }
}

/* --------------------
*/
int chmod_unix_rfork(char *path, char *name, char *file, mode_t mode)
{
    if (adouble == AD_EA) {
#if defined (HAVE_EAFD) && defined (SOLARIS)
        sprintf(temp, "runat '%s/%s/%s' chmod 0%o %s", path, name, file, mode, AD_EA_RESO);
        if (!Quiet) {
            fprintf(stdout, "%s\n", temp);
        }
        if (system(temp) != 0) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED %s\n", strerror(errno));
            }
            test_failed();
            return -1;
        }
        return 0;
#else
#ifndef __APPLE__
        sprintf(temp, "%s/%s/._%s", path, name, file);
        if (!Quiet) {
            fprintf(stdout, "chmod(%s, 0%o)\n", temp, mode);
        }
        if (chmod(temp, mode)) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED %s\n", strerror(errno));
            }
            test_failed();
            return -1;
        }
#endif
        return 0;
#endif
    } else {
        sprintf(temp, "%s/%s/.AppleDouble/%s", path, name, file);
        if (!Quiet) {
            fprintf(stdout, "chmod (%s, 0%o)\n", temp, mode);
        }
        if (chmod(temp, mode)) {
            if (!Quiet) {
                fprintf(stdout,"\tFAILED %s\n", strerror(errno));
            }
            test_failed();
            return -1;
        }
        return 0;
    }
}

/* --------------------
	delete an empty directory
*/
int delete_unix_dir(char *path, char *name)
{
    if (!Quiet) {
        fprintf(stdout,"rmdir(%s)\n", name);
    }

    if (adouble == AD_V2)
        if (delete_unix_adouble(path, name))
            return -1;
	sprintf(temp, "%s/%s", path, name);
	if (rmdir(temp) <0) {
        if (!Quiet) {
            fprintf(stdout,"\tFAILED rmdir %s %s\n", temp, strerror(errno));
        }
		return -1;
	}
	return 0;
}

/* ----------------------
 * create a folder with r-xr-xr-x .AppleDouble
*/
int folder_with_ro_adouble(uint16_t vol, int did, char *name, char *file)
{
int ret = 0;
int dir = 0;
uint16_t bitmap =  (1 << DIRPBIT_ACCESS);

    if (!Quiet) {
        fprintf(stdout,"\t>>>>>>>> Create folder with ro adouble <<<<<<<<<< \n");
    }

	if (!(dir = FPCreateDir(Conn,vol, did , name))) {
		test_nottested();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  dir , "", 0,bitmap )) {
		test_nottested();
		goto fin;
	}

	if (FPCreateFile(Conn, vol,  0, dir , file)) {
		test_nottested();
		goto fin;
	}
	if (!Mac && adouble == AD_V2) {
		if (delete_unix_rf(Path, name, "")) {
			test_nottested();
		}
		else if (delete_unix_rf(Path, name, file)) {
			test_nottested();
		}
		else if (chmod_unix_adouble(Path,name,0555)) {
			test_nottested();
		}
		ret = dir;
	}
	else {
		ret = dir;
	}
fin:
	if (!ret && dir) {
		FPDelete(Conn, vol,  dir, file);
		if (FPDelete(Conn, vol,  did, name)) {
			test_nottested();
		}
	}
    if (!Quiet) {
        fprintf(stdout,"\t>>>>>>>> done <<<<<<<<<< \n");
    }
	return ret;
}

/* -------------------------------- */
int delete_ro_adouble(uint16_t vol, int did, char *file)
{
    if (!Quiet) {
        fprintf(stdout,"\t>>>>>>>> delete folder with ro adouble <<<<<<<<<< \n");
    }
	FAIL (FPDelete(Conn, vol, did, file))
	FAIL (FPDelete(Conn, vol, did, ""))
	return 0;
}
