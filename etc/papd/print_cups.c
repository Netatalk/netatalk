/*
 * $Id: print_cups.c,v 1.6 2010-01-26 20:43:11 didg Exp $
 *
 * Copyright 2004 Bjoern Fernhomberg.
 *
 * Some code copied or adapted from print_cups.c for samba
 * Copyright 1999-2003 by Michael R Sweet.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>


#ifdef HAVE_CUPS

#include <cups/ipp.h>
#include <cups/cups.h>
#include <cups/language.h>
#include <atalk/unicode.h>
#include <atalk/logger.h>
#include <atalk/atp.h>
#include <atalk/pap.h>
#include <atalk/util.h>

#include "printer.h"
#include "print_cups.h"

#define MAXCHOOSERLEN 31
#define HTTP_MAX_URI 1024

static const char* cups_status_msg[] = {
        "status: busy; info: \"%s\" is rejecting jobs; ",
        "status: idle; info: \"%s\" is stopped, accepting jobs ;",
        "status: idle; info: \"%s\" is ready ; ",
};

/* Local functions */
static int 	convert_to_mac_name ( const char *encoding, char * inptr, char * outptr, size_t outlen);
static size_t	to_ascii ( char *inbuf, char **outbuf);
static int 	cups_mangle_printer_name ( struct printer *pr, struct printer *printers);
static void     cups_free_printer ( struct printer *pr);


const char * cups_get_language (void)
{
        cups_lang_t *language;

        language = cupsLangDefault();           /* needed for conversion */
        return cupsLangEncoding(language);
}

/*
 * 'cups_passwd_cb()' - The CUPS password callback...
 */

static const char *                            /* O - Password or NULL */
cups_passwd_cb(const char *prompt _U_)      /* I - Prompt */
{
 /*
  * Always return NULL to indicate that no password is available...
  */
  return (NULL);
}


/*
 * 'cups_printername_ok()' - Verify supplied printer name is a valid cups printer
 */

int                                     /* O - 1 if printer name OK */
cups_printername_ok(char *name)         /* I - Name of printer */
{
        http_t          *http;          /* HTTP connection to server */
	cups_dest_t	*dest = NULL;	/* Destination */
        ipp_t           *request,       /* IPP Request */
                        *response;      /* IPP Response */
        cups_lang_t     *language;      /* Default language */
        char            uri[HTTP_MAX_URI]; /* printer-uri attribute */

       /*
        * Make sure we don't ask for passwords...
        */

        cupsSetPasswordCB(cups_passwd_cb);

	/*
	 * Try to connect to the requested printer...
	 */

	dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, name, NULL);
	
	if (!dest)
	{
		LOG(log_error, logtype_papd,
		    "Unable to get destination \"%s\": %s", name, cupsLastErrorString());
		return (0);
	}
	if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, NULL, NULL, 0, NULL, NULL)) == NULL)
	{
		LOG(log_error, logtype_papd,
		    "Unable to connect to destination \"%s\": %s", dest->name, cupsLastErrorString());
		return (0);
	}
	cupsFreeDests(1, dest);

       /*
        * Build an IPP_GET_PRINTER_ATTRS request, which requires the following
        * attributes:
        *
        *    attributes-charset
        *    attributes-natural-language
        *    requested-attributes
        *    printer-uri
        */
        request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

        language = cupsLangDefault();

        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
                     "attributes-charset", NULL, cupsLangEncoding(language));

        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
                     "attributes-natural-language", NULL, language->language);

        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
                     "requested-attributes", NULL, "printer-uri");

        sprintf(uri, "ipp://localhost/printers/%s", name);

        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                     "printer-uri", NULL, uri);

       /*
        * Do the request and get back a response...
        */

        if ((response = cupsDoRequest(http, request, "/")) == NULL)
        {
      		LOG(log_error, logtype_papd, "Unable to get printer status for %s - %s", name,
                         ippErrorString(cupsLastError()));
                httpClose(http);
                return (0);
        }

        httpClose(http);

        if (cupsLastError() >= IPP_OK_CONFLICT)
        {
      		LOG(log_error, logtype_papd, "Unable to get printer status for %s - %s", name,
                         ippErrorString(cupsLastError()));
                ippDelete(response);
                return (0);
        }
        else
        {
                ippDelete(response);
                return (1);
        }

	return (0);
}

const char * 
cups_get_printer_ppd ( char * name)
{
	http_t		*http;		/* Connection to destination */
	cups_dest_t	*dest = NULL;	/* Destination */
	cups_dest_t 	*dests;		/* Destination List */
	ipp_t           *request,       /* IPP Request */
			*response;      /* IPP Response */
	cups_lang_t	*language;      /* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */
        const char	*pattrs[] =   /* Requested printer attributes */
                        {
                          "printer-make-and-model",
                          "color-supported"
                        };
	ipp_attribute_t	*attr;
  	cups_file_t	*fp;		/* PPD file */
    	static char	buffer[1024];	/* I - Filename buffer */
    	size_t		bufsize;	/* I - Size of filename buffer */
  	char		make[256],	/* Make and model */
			*model;


	cupsSetPasswordCB(cups_passwd_cb);

	/*
	 *We have to go this roundabout way to correctly get the make and 
	 *model of DNS-SD discovered printers. Otherwise we get the name of 
	 *the CUPS temporary queue, which we don't want.
	 */

	int num_dests = cupsGetDests2(CUPS_HTTP_DEFAULT, &dests);
	dest = cupsGetDest(name, NULL, num_dests, dests);
	const char *make_model = cupsGetOption("printer-make-and-model", dest->num_options, dest->options);

	if (!dest)
	{
		LOG(log_error, logtype_papd, "testdest: Unable to get destination \"%s\": %s\n", name, cupsLastErrorString());
		cupsFreeDests(num_dests,dests);
		return (0);
	}

	if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, NULL, NULL, 0, NULL, NULL)) == NULL)
	{
		LOG(log_error, logtype_papd, "testdest: Unable to connect to destination \"%s\": %s\n", dest->name, cupsLastErrorString());
		cupsFreeDests(num_dests,dests);
		return (0);
	}
	
	/*
	 * Generate the printer URI...
	 */

	sprintf(uri, "ipp://localhost/printers/%s", name);

	/*
	 * Build an IPP_GET_PRINTER_ATTRIBUTES request, which requires the
	 * following attributes:
	 *
	 *    attributes-charset
	 *    attributes-natural-language
	 *    requested-attributes
	 *    printer-uri
	 */

        request = ippNew();

        ippSetOperation(request, IPP_GET_PRINTER_ATTRIBUTES);
        ippSetRequestId(request, 1);

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
		     "attributes-charset", NULL,
		     cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
		     "attributes-natural-language", NULL,
		     language->language);

	ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
		      "requested-attributes",
		      (sizeof(pattrs) / sizeof(pattrs[0])), NULL, pattrs);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
		     "printer-uri", NULL, uri);

	/*
	 * Do the request and get back a response...
	 */

        if ((response = cupsDoRequest(http, request, "/")) == NULL)
        {
                 LOG(log_error, logtype_papd,  "Unable to get printer attribs for %s - %s", name,
                         ippErrorString(cupsLastError()));
                httpClose(http);
		cupsFreeDests(num_dests,dests);
                return (0);
        }

        if (ippGetStatusCode(response) >= IPP_OK_CONFLICT)
        {
      		 LOG(log_error, logtype_papd,  "Unable to get printer attribs for %s - %s", name,
                         ippErrorString(ippGetStatusCode(response)));
                ippDelete(response);
                httpClose(http);
		cupsFreeDests(num_dests,dests);
                return (0);
        }

	/*
	 * Range check input...
	 */
	
	bufsize = sizeof(buffer);
	if (buffer)
		*buffer = '\0';

	if (!buffer || bufsize < 1)
	{
		LOG(log_error, logtype_papd, "Check buffer failed!\n");
		LOG(log_error, logtype_papd, strerror(EINVAL));
                ippDelete(response);
                httpClose(http);
		cupsFreeDests(num_dests,dests);
		return (0);
	}

	if (!response)
	{
		LOG(log_error, logtype_papd, "No IPP attributes.\n");
                ippDelete(response);
                httpClose(http);
		cupsFreeDests(num_dests,dests);
		return (0);
	}

	/*
	 * Open a temporary file for the PPD...
	 */

	if ((fp = cupsTempFile2(buffer, (int)bufsize)) == NULL)
	{
		LOG(log_error, logtype_papd, strerror(errno));
                ippDelete(response);
                httpClose(http);
		cupsFreeDests(num_dests,dests);
		return (0);
	}

	/*
	 * Write the header and some hard coded defaults for the PPD file...
	 */

	cupsFilePuts(fp, "*PPD-Adobe: \"4.3\"\n");
	cupsFilePuts(fp, "*FormatVersion: \"4.3\"\n");
	cupsFilePrintf(fp, "*FileVersion: \"%d.%d\"\n", CUPS_VERSION_MAJOR, CUPS_VERSION_MINOR);
	cupsFilePuts(fp, "*LanguageVersion: English\n");
	cupsFilePuts(fp, "*LanguageEncoding: ISOLatin1\n");
	cupsFilePuts(fp, "*PSVersion: \"(3010.000) 0\"\n");
	cupsFilePuts(fp, "*LanguageLevel: \"3\"\n");
	cupsFilePuts(fp, "*FileSystem: False\n");
	cupsFilePuts(fp, "*PCFileName: \"ippeve.ppd\"\n");

	if ((dests != NULL))
		strcpy(make, make_model);
	else
		strcpy(make, "Unknown Printer");

	if (!strncasecmp(make, "Hewlett Packard ", 16) || !strncasecmp(make, "Hewlett-Packard ", 16))
	{
		model = make + 16;
		strcpy(make, "HP");
  	}
	else if ((model = strchr(make, ' ')) != NULL)
		*model++ = '\0';
  	else
		model = "Unknown";

	cupsFilePrintf(fp, "*Manufacturer: \"%s\"\n", make);
	cupsFilePrintf(fp, "*ModelName: \"%s\"\n", model);
	cupsFilePrintf(fp, "*Product: \"(%s %s)\"\n", make, model);
	cupsFilePrintf(fp, "*NickName: \"%s %s\"\n", make, model);
	cupsFilePrintf(fp, "*ShortNickName: \"%s %s\"\n", make, model);

 	if ((attr = ippFindAttribute(response, "color-supported", IPP_TAG_BOOLEAN)) != NULL && ippGetBoolean(attr, 0))
  		cupsFilePuts(fp, "*ColorDevice: True\n");
	else
		cupsFilePuts(fp, "*ColorDevice: False\n");
	cupsFilePuts(fp, "*TTRasterizer: Type42\n");
	cupsFilePuts(fp, "*Resolution: 600dpi\n");
	cupsFilePuts(fp, "*FaxSupport: None\n");

	/*
	 *Clean up.
	 */
	
	httpClose(http);
	ippDelete(response);
	cupsFileClose(fp);
	cupsFreeDests(num_dests,dests);
	return (buffer);
}

int
cups_get_printer_status (struct printer *pr)
{

        http_t          *http;          /* HTTP connection to server */
	cups_dest_t	*dest = NULL;	/* Destination */
        ipp_t           *request,       /* IPP Request */
                        *response;      /* IPP Response */
        ipp_attribute_t *attr;          /* Current attribute */
        cups_lang_t     *language;      /* Default language */
        char            uri[HTTP_MAX_URI]; /* printer-uri attribute */
	int 		status = -1;

        static const char *pattrs[] =   /* Requested printer attributes */
                        {
                          "printer-state",
                          "printer-state-message",
			  "printer-is-accepting-jobs"
                        };

       /*
        * Make sure we don't ask for passwords...
        */

        cupsSetPasswordCB(cups_passwd_cb);

	/*
	 * Try to connect to the requested printer...
	 */

	dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, pr->p_printer, NULL);
	
	if (!dest)
	{
		LOG(log_error, logtype_papd,
		    "Unable to get destination \"%s\": %s", pr->p_printer, cupsLastErrorString());
		return (0);
	}
	if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, NULL, NULL, 0, NULL, NULL)) == NULL)
	{
		LOG(log_error, logtype_papd,
		    "Unable to connect to destination \"%s\": %s", dest->name, cupsLastErrorString());
		return (0);
	}
	cupsFreeDests(1, dest);

       /*
        * Generate the printer URI...
        */

        sprintf(uri, "ipp://localhost/printers/%s", pr->p_printer);



       /*
        * Build an IPP_GET_PRINTER_ATTRIBUTES request, which requires the
        * following attributes:
        *
        *    attributes-charset
        *    attributes-natural-language
        *    requested-attributes
        *    printer-uri
        */

        request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

        language = cupsLangDefault();

        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
                     "attributes-charset", NULL, cupsLangEncoding(language));

        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
                     "attributes-natural-language", NULL, language->language);

        ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
                      "requested-attributes",
                      (sizeof(pattrs) / sizeof(pattrs[0])),
                      NULL, pattrs);

        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                     "printer-uri", NULL, uri);

       /*
        * Do the request and get back a response...
        */

        if ((response = cupsDoRequest(http, request, "/")) == NULL)
        {
      		LOG(log_error, logtype_papd, "Unable to get printer status for %s - %s", pr->p_printer,
                         ippErrorString(cupsLastError()));
                httpClose(http);
                return (0);
        }

        if (cupsLastError() >= IPP_OK_CONFLICT)
        {
      		LOG(log_error, logtype_papd, "Unable to get printer status for %s - %s", pr->p_printer,
                         ippErrorString(cupsLastError()));
                ippDelete(response);
                httpClose(http);
                return (0);
        }

       /*
        * Get the current printer status and convert it to the status values.
        */

	memset ( pr->p_status, 0 ,sizeof(pr->p_status));

        if ((attr = ippFindAttribute(response, "printer-state", IPP_TAG_ENUM)) != NULL)
        {
                if (ippGetInteger(attr, 0) == IPP_PRINTER_STOPPED)
			status = 1;
                else if (ippGetInteger(attr,0) == IPP_NOT_ACCEPTING)
			status = 0;
		else
			status = 2;
        }

	if ((attr = ippFindAttribute(response, "printer-is-accepting-jobs", IPP_TAG_BOOLEAN)) != NULL)
	{
		if ( ippGetBoolean(attr, 0) == 0 )
			status = 0;
	}
		
	snprintf ( pr->p_status, 255, cups_status_msg[status], pr->p_printer );

        if ((attr = ippFindAttribute(response, "printer-state-message", IPP_TAG_TEXT)) != NULL)
		strncat ( pr->p_status, ippGetString(attr, 0, NULL), 255-strlen(pr->p_status));

        ippDelete(response);

       /*
        * Return the print status ...
        */

        httpClose(http);

	return (status);
}


/*------------------------------------------------------------------------*/

/* pass the job to cups */

int cups_print_job ( char * name, char *filename, char *job, char *username, char * cupsoptions )
{
	int 		jobid;
	char 		filepath[MAXPATHLEN];
	int           	num_options;
	cups_option_t 	*options;

	/* Initialize the options array */
	num_options = 0;
	options     = (cups_option_t *)0;

        cupsSetPasswordCB(cups_passwd_cb);

	if ( username != NULL )
	{
		/* Add options using cupsAddOption() */
		num_options = cupsAddOption("job-originating-user-name", username, num_options, &options);
		num_options = cupsAddOption("originating-user-name", username, num_options, &options);
		cupsSetUser ( username );
	}

	if (cupsoptions != NULL)
	{
              num_options = cupsParseOptions(cupsoptions, num_options, &options);
	}
	
	strlcpy ( filepath, SPOOLDIR, sizeof(filepath));
	strlcat ( filepath , "/", sizeof(filepath));
	strlcat ( filepath , filename, sizeof(filepath));
	
	if ((jobid = cupsPrintFile( name, filepath, job, 0, options)) == 0)
		LOG(log_error, logtype_papd, "Unable to print job '%s' (%s) to printer '%s' for user '%s' - CUPS error : '%s'", job, filepath, name, username, ippErrorString(cupsLastError()));
	else 
		LOG(log_info, logtype_papd, "Job '%s' queued to printer '%s' with id '%d'", job, name, jobid);

	cupsFreeOptions(num_options, options);
	return (jobid);
}


/*------------------------------------------------------------------------*/

struct printer	*
cups_autoadd_printers ( struct printer	*defprinter, struct printer *printers)
{
	struct printer 	*pr;
        int         	num_dests,i;
	int 	    	ret;
        cups_dest_t 	*dests;
        cups_lang_t 	*language;
	char 	    	name[MAXCHOOSERLEN+1], *p;

        language  = cupsLangDefault();		/* needed for conversion */
        num_dests = cupsGetDests(&dests);	/* get the available destination from CUPS */

        for  (i=0; i< num_dests; i++)
        {
		if (( pr = (struct printer *)malloc( sizeof( struct printer )))	== NULL ) {
			LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
			exit( 1 );
		}
	
		memcpy( pr, defprinter, sizeof( struct printer ) );

		/* convert from CUPS to local encoding */
                convert_string_allocate( add_charset(cupsLangEncoding(language)), CH_UNIX, 
                                         dests[i].name, -1, &pr->p_u_name);

		/* convert CUPS name to Mac charset */
		if ( convert_to_mac_name ( cupsLangEncoding(language), dests[i].name, name, sizeof(name)) <= 0)
		{
			LOG (log_error, logtype_papd, "Conversion from CUPS to MAC name failed for %s", dests[i].name);
			free (pr);
			continue;
		}

		if (( pr->p_name = (char *)malloc( strlen( name ) + 1 )) == NULL ) {
			LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
			exit( 1 );
		}
		strcpy( pr->p_name, name );

		/* set printer flags */
		pr->p_flags &= ~P_PIPED;
		pr->p_flags |= P_SPOOLED;
		pr->p_flags |= P_CUPS;
		pr->p_flags |= P_CUPS_AUTOADDED;
			
		if (( pr->p_printer = (char *)malloc( strlen( dests[i].name ) + 1 )) == NULL ) {
			LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
               		exit( 1 );
        	}
        	strcpy( pr->p_printer, dests[i].name );			

        	if ( (p = (char *) cups_get_printer_ppd ( pr->p_printer )) != NULL ) {
        		if (( pr->p_ppdfile = (char *)malloc( strlen( p ) + 1 )) == NULL ) {
				LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
               			exit( 1 );
        		}
        		strcpy( pr->p_ppdfile, p );
			pr->p_flags |= P_CUPS_PPD;
        	}

		if ( (ret = cups_check_printer ( pr, printers, 0)) == -1) 
			ret = cups_mangle_printer_name ( pr, printers );

		if (ret) {
			cups_free_printer (pr);
			LOG(log_info, logtype_papd, "Printer %s not added: reason %d", name, ret);
		}
		else {
			pr->p_next = printers;
			printers = pr;
		}
	}
 
        cupsFreeDests(num_dests, dests);
        cupsLangFree(language);

	return printers;
}


/*------------------------------------------------------------------------*/

/* cups_mangle_printer_name
 *    Mangles the printer name if two CUPS printer provide the same Chooser Name
 *    Append '#nn' to the chooser name, if it is longer than 28 char we overwrite the last three chars
 * Returns: 0 on Success, 2 on Error
 */

static int cups_mangle_printer_name ( struct printer *pr, struct printer *printers)
{
	size_t 	count, name_len;
	char	name[MAXCHOOSERLEN];

	count = 1;
	name_len = strlen (pr->p_name);
	strncpy ( name, pr->p_name, MAXCHOOSERLEN-3);
	
	/* Reallocate necessary space */
	(name_len >= MAXCHOOSERLEN-3) ? (name_len = MAXCHOOSERLEN+1) : (name_len = name_len + 4);
	pr->p_name = (char *) realloc (pr->p_name, name_len );
		
	while ( ( cups_check_printer ( pr, printers, 0 )) && count < 100)
	{
		memset ( pr->p_name, 0, name_len);
		strncpy ( pr->p_name, name, MAXCHOOSERLEN-3);
		sprintf ( pr->p_name, "%s#%2.2u", pr->p_name, count++);
	}

	if ( count > 99) 
		return (2);
	
	return (0);
}	

/*------------------------------------------------------------------------*/

/* fallback ASCII conversion */

static size_t
to_ascii ( char  *inptr, char **outptr)
{
	char *out, *osav;

	if ( NULL == (out = (char*) malloc ( strlen ( inptr) + 1 )) ) {
		LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
		exit (1);
	}

	osav = out;

	while ( *inptr != '\0' ) {
		if ( *inptr & 0x80 ) {
			*out = '_';
			out++;
			inptr++;
		}
		else
			*out++ = *inptr++;
	}
	*out = '\0';
	*outptr = osav;
	return ( strlen (osav) );
}


/*------------------------------------------------------------------------*/

/* convert_to_mac_name
 * 	1) Convert from encoding to MacRoman
 *	2) Shorten to MAXCHOOSERLEN (31)
 *      3) Replace @ and _ as they are illegal
 * Returns: -1 on failure, length of name on success; outpr contains name in MacRoman
 */

static int convert_to_mac_name ( const char * encoding, char * inptr, char * outptr, size_t outlen)
{
	char   	*outbuf;
	char	*soptr;
	size_t 	name_len = 0;
	size_t 	i;
	charset_t chCups;

	/* Change the encoding */
	if ((charset_t)-1 != (chCups = add_charset(encoding))) {
		name_len = convert_string_allocate( chCups, CH_MAC, inptr, -1, &outbuf);
	}

	if (name_len == 0 || name_len == (size_t)-1) {
		/* charset conversion failed, use ascii fallback */
		name_len = to_ascii ( inptr, &outbuf );
	}
	
	soptr = outptr;

	for ( i=0; i< name_len && i < outlen-1 ; i++)
	{
		if ( outbuf[i] == '_' )	
			*soptr = ' '; /* Replace '_' with a space (just for the looks) */
		else if ( outbuf[i] == '@' || outbuf[i] == ':' ) 
			*soptr = '_'; /* Replace @ and : with '_' as they are illegal chars */
		else
			*soptr = outbuf[i];
		soptr++;
	}
	*soptr = '\0';
	free (outbuf);
	return (i);
}


/*------------------------------------------------------------------------*/

/*
 * cups_check_printer:
 * check if a printer with this name already exists.
 * if yes, and replace = 1 the existing printer is replaced with
 * the new one. This allows to overwrite printer settings
 * created by cupsautoadd. It also used by cups_mangle_printer.
 */

int cups_check_printer ( struct printer *pr, struct printer *printers, int replace)
{
	struct printer *listptr, *listprev;

	listptr = printers;
	listprev = NULL;

	while ( listptr != NULL) {
		if ( strcasecmp (pr->p_name, listptr->p_name) == 0) {
			if ( pr->p_flags & P_CUPS_AUTOADDED ) {  /* Check if printer has been autoadded */
				if ( listptr->p_flags & P_CUPS_AUTOADDED )
					return (-1);		 /* Conflicting Cups Auto Printer (mangling issue?) */
				else
					return (1);		 /* Never replace a hand edited printer with auto one */
			}

			if ( replace ) {
				/* Replace printer */
				if ( listprev != NULL) {
					pr->p_next = listptr->p_next;
					listprev->p_next = pr;
					cups_free_printer (listptr);
				}
				else {
					printers = pr;
					printers->p_next = listptr->p_next;
					cups_free_printer (listptr);
				}
			}
			return (1);  /* Conflicting Printers */
		}
		listprev = listptr;
		listptr = listptr->p_next;
	}	
	return (0);	/* No conflict */
}


/*------------------------------------------------------------------------*/


void
cups_free_printer ( struct printer *pr)
{
	if ( pr->p_name != NULL)
		free (pr->p_name);
	if ( pr->p_printer != NULL)
		free (pr->p_printer);
	if ( pr->p_ppdfile != NULL)
		free (pr->p_ppdfile);
	
	/* CUPS autoadded printers share the other informations
	 * so if the printer is autoadded we won't free them.
	 * We might leak some bytes here though.
	 */
	if ( pr->p_flags & P_CUPS_AUTOADDED ) {
		free (pr);
		return;
	}

	if ( pr->p_operator != NULL )
		free (pr->p_operator);
	if ( pr->p_zone != NULL )
		free (pr->p_zone);
	if ( pr->p_type != NULL )
		free (pr->p_type);
	if ( pr->p_authprintdir != NULL )
		free (pr->p_authprintdir);
	
	free ( pr );
	return;
	
}
	
#endif /* HAVE_CUPS*/
