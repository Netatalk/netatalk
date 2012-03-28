
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

int        iniparser_getnsec(const dictionary * d);
const char *iniparser_getsecname(const dictionary * d, int n);
void       iniparser_dump_ini(const dictionary * d, FILE * f);
void       iniparser_dump(const dictionary * d, FILE * f);
const char *iniparser_getstring(const dictionary * d, const char *section, const char * key, const char * def);
char       *iniparser_getstrdup(const dictionary * d, const char *section, const char * key, const char * def);
int        iniparser_getint(const dictionary * d, const char *section, const char * key, int notfound);
double     iniparser_getdouble(const dictionary * d, const char *section, const char * key, double notfound);
int        iniparser_getboolean(const dictionary * d, const char *section, const char * key, int notfound);
int        iniparser_set(dictionary * ini, char *section, char * key, char * val);
void       iniparser_unset(dictionary * ini, char *section, char * key);
int        iniparser_find_entry(const dictionary * ini, const char * entry) ;
dictionary *iniparser_load(const char * ininame);
void       iniparser_freedict(dictionary * d);

#endif
