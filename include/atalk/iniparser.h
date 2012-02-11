
/*-------------------------------------------------------------------------*/
/**
   @file    iniparser.h
   @author  N. Devillard
   @date    Sep 2007
   @version 3.0
   @brief   Parser for ini files.
*/
/*--------------------------------------------------------------------------*/

/*
	$Id: iniparser.h,v 1.26 2011-03-02 20:15:13 ndevilla Exp $
	$Revision: 1.26 $
*/

#ifndef _INIPARSER_H_
#define _INIPARSER_H_

/*---------------------------------------------------------------------------
   								Includes
 ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The following #include is necessary on many Unixes but not Linux.
 * It is not needed for Windows platforms.
 * Uncomment it if needed.
 */
/* #include <unistd.h> */

#include "dictionary.h"

int        iniparser_getnsec(dictionary * d);
char       *iniparser_getsecname(dictionary * d, int n);
void       iniparser_dump_ini(dictionary * d, FILE * f);
void       iniparser_dump(dictionary * d, FILE * f);
char       *iniparser_getstring(dictionary * d, char *section, char * key, char * def);
char       *iniparser_getstringdup(dictionary * d, char *section, char * key, char * def);
int        iniparser_getint(dictionary * d, char *section, char * key, int notfound);
double     iniparser_getdouble(dictionary * d, char *section, char * key, double notfound);
int        iniparser_getboolean(dictionary * d, char *section, char * key, int notfound);
int        iniparser_set(dictionary * ini, char *section, char * key, char * val);
void       iniparser_unset(dictionary * ini, char *section, char * key);
int        iniparser_find_entry(dictionary * ini, char * entry) ;
dictionary *iniparser_load(char * ininame);
void       iniparser_freedict(dictionary * d);

#endif
