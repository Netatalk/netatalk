/*
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
#include <unistd.h>
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

static const char *cups_status_msg[] = {
    "status: busy; info: \"%s\" is rejecting jobs; ",
    "status: idle; info: \"%s\" is stopped, accepting jobs ; ",
    "status: idle; info: \"%s\" is ready ; ",
    "status: busy; info: \"%s\" is processing a job ; ",
};

/* Local functions */
static int 	convert_to_mac_name(const char *encoding, char *inptr,
                                char *outptr, size_t outlen);
static size_t	to_ascii(char *inbuf, char **outbuf);
static int 	cups_mangle_printer_name(struct printer *pr,
                                     struct printer *printers);
static void     cups_free_printer(struct printer *pr);


const char *cups_get_language(void)
{
    cups_lang_t *language;
    /* needed for conversion */
    language = cupsLangDefault();
    const char *curr_encoding = cupsLangEncoding(language);
    return curr_encoding;
}

/*
 * 'cups_passwd_cb()' - The CUPS password callback...
 * O - Password or NULL
 * I - Prompt
 */
static const char *cups_passwd_cb(const char *prompt _U_, http_t *http,
                                  const char *method,
                                  const char *resource, void *user_data)
{
    /*
     * Always return NULL to indicate that no password is available...
     */
    return NULL;
}


/*
 * 'cups_printername_ok()' - Verify supplied printer name is a valid cups printer
 * O - 1 if printer name OK
 * I - Name of printer
 */

int cups_printername_ok(char *name)
{
    /* HTTP connection to server */
    http_t *http;
    /* Destination */
    cups_dest_t	*dest = NULL;
    /* IPP Request */
    ipp_t *request;
    /* IPP Response */
    ipp_t *response;
    /*
     * Make sure we don't ask for passwords...
     */
    cupsSetPasswordCB2(cups_passwd_cb, NULL);
    /*
     * Try to connect to the requested printer...
     */
    dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, name, NULL);

    if (!dest) {
        LOG(log_error, logtype_papd,
            "Unable to get destination \"%s\": %s", name, cupsLastErrorString());
        return 0;
    }

    if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, NULL, NULL, 0,
                                NULL, NULL)) == NULL) {
        LOG(log_error, logtype_papd,
            "Unable to connect to destination \"%s\": %s", dest->name,
            cupsLastErrorString());
        return 0;
    }

    return 1;
}

const char *cups_get_printer_ppd(char *name)
{
    /* Connection to destination */
    http_t *http;
    /* Destination */
    cups_dest_t *dest = NULL;
    /* Destination List */
    cups_dest_t *dests;
    /* IPP Request */
    ipp_t *request;
    /* IPP Response */
    ipp_t *response;
    /* Requested printer attributes */
    const char *pattrs[] = {
        "printer-make-and-model",
        "color-supported"
    };
    ipp_attribute_t	*attr;
    /* PPD file */
    cups_file_t	*fp;
    /* I - Filename buffer */
    static char buffer[1024];
    /* I - Size of filename buffer */
    size_t bufsize;
    /* Make and model */
    char make[256];
    char *model;
    /* Destination flag */
    unsigned flags = 0;
    cupsSetPasswordCB2(cups_passwd_cb, NULL);
    int num_dests = cupsGetDests2(CUPS_HTTP_DEFAULT, &dests);
    dest = cupsGetDest(name, NULL, num_dests, dests);

    if (!dest) {
        LOG(log_error, logtype_papd, "Unable to get destination \"%s\": %s\n", name,
            cupsLastErrorString());
        cupsFreeDests(num_dests, dests);
        return 0;
    }

    const char *make_model = cupsGetOption("printer-make-and-model",
                                           dest->num_options, dest->options);
    const char *printer_uri_supported = cupsGetOption("printer-uri-supported",
                                        dest->num_options, dest->options);
    const char *printer_is_temporary = cupsGetOption("printer-is-temporary",
                                       dest->num_options, dest->options);

    /* Is this a DNS-SD discovered printer? Connect directly to it. */
    if (!printer_uri_supported || !strcmp(printer_is_temporary, "true")) {
        flags = CUPS_DEST_FLAGS_DEVICE;
    } else {
        flags = CUPS_DEST_FLAGS_NONE;
    }

    if ((http = cupsConnectDest(dest, flags, 30000, NULL, NULL, 0, NULL,
                                NULL)) == NULL) {
        LOG(log_error, logtype_papd, "Unable to connect to destination \"%s\": %s\n",
            dest->name, cupsLastErrorString());
        cupsFreeDests(num_dests, dests);
        return 0;
    }

    const char *uri = cupsGetOption("device-uri", dest->num_options, dest->options);
    /*
     * Build an IPP_OP_GET_PRINTER_ATTRIBUTES request, which requires the
     * following attributes:
     *
     *    requested-attributes
     *    printer-uri
     */
    request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

    /* Use the correct URI if DNS-SD printer. */
    if (flags == CUPS_DEST_FLAGS_DEVICE) {
        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                     "printer-uri", NULL, uri);
    } else {
        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                     "printer-uri", NULL, printer_uri_supported);
    }

    ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                  "requested-attributes",
                  (sizeof(pattrs) / sizeof(pattrs[0])), NULL, pattrs);

    /*
     * Do the request and get back a response...
     */

    if ((response = cupsDoRequest(http, request, "/")) == NULL) {
        LOG(log_error, logtype_papd,  "Unable to get printer attribs for %s - %s", name,
            ippErrorString(cupsLastError()));
        httpClose(http);
        cupsFreeDests(num_dests, dests);
        return 0;
    }

    if (ippGetStatusCode(response) >= IPP_STATUS_OK_CONFLICTING) {
        LOG(log_error, logtype_papd,  "Unable to get printer attribs for %s - %s", name,
            ippErrorString(ippGetStatusCode(response)));
        ippDelete(response);
        httpClose(http);
        cupsFreeDests(num_dests, dests);
        return 0;
    }

    /*
     * Range check input...
     */
    bufsize = sizeof(buffer);
    buffer[0] = '\0';

    if (bufsize < 1) {
        LOG(log_error, logtype_papd, "Check buffer failed!\n");
        LOG(log_error, logtype_papd, strerror(EINVAL));
        ippDelete(response);
        httpClose(http);
        cupsFreeDests(num_dests, dests);
        return 0;
    }

    if (!response) {
        LOG(log_error, logtype_papd, "No IPP attributes.\n");
        ippDelete(response);
        httpClose(http);
        cupsFreeDests(num_dests, dests);
        return 0;
    }

    /*
     * Open a temporary file for the PPD...
     */

    if ((fp = cupsTempFile2(buffer, (int)bufsize)) == NULL) {
        LOG(log_error, logtype_papd, strerror(errno));
        ippDelete(response);
        httpClose(http);
        cupsFreeDests(num_dests, dests);
        return 0;
    }

    /*
     * Write the header and some hard coded defaults for the PPD file...
     */
    cupsFilePuts(fp, "*PPD-Adobe: \"4.3\"\n");
    cupsFilePuts(fp, "*FormatVersion: \"4.3\"\n");
    cupsFilePrintf(fp, "*FileVersion: \"%d.%d\"\n", CUPS_VERSION_MAJOR,
                   CUPS_VERSION_MINOR);
    cupsFilePuts(fp, "*LanguageVersion: English\n");
    cupsFilePuts(fp, "*LanguageEncoding: ISOLatin1\n");
    cupsFilePuts(fp, "*PSVersion: \"(3010.000) 0\"\n");
    cupsFilePuts(fp, "*LanguageLevel: \"3\"\n");
    cupsFilePuts(fp, "*FileSystem: False\n");
    cupsFilePuts(fp, "*PCFileName: \"ippeve.ppd\"\n");

    if (dests != NULL) {
        strcpy(make, make_model);
    } else {
        strcpy(make, "Unknown Printer");
    }

    if (!strncasecmp(make, "Hewlett Packard ", 16)
            || !strncasecmp(make, "Hewlett-Packard ", 16)) {
        model = make + 16;
        strcpy(make, "HP");
    } else if ((model = strchr(make, ' ')) != NULL) {
        *model++ = '\0';
    } else {
        model = "Unknown";
    }

    cupsFilePrintf(fp, "*Manufacturer: \"%s\"\n", make);
    cupsFilePrintf(fp, "*ModelName: \"%s\"\n", model);
    cupsFilePrintf(fp, "*Product: \"(%s %s)\"\n", make, model);
    cupsFilePrintf(fp, "*NickName: \"%s %s\"\n", make, model);
    cupsFilePrintf(fp, "*ShortNickName: \"%s %s\"\n", make, model);

    if ((attr = ippFindAttribute(response, "color-supported",
                                 IPP_TAG_BOOLEAN)) != NULL && ippGetBoolean(attr, 0)) {
        cupsFilePuts(fp, "*ColorDevice: True\n");
    } else {
        cupsFilePuts(fp, "*ColorDevice: False\n");
    }

    cupsFilePuts(fp, "*TTRasterizer: Type42\n");
    cupsFilePuts(fp, "*Resolution: 600dpi\n");
    cupsFilePuts(fp, "*FaxSupport: None\n");
    /*
     *Clean up.
     */
    httpClose(http);
    ippDelete(response);
    cupsFileClose(fp);
    cupsFreeDests(num_dests, dests);
    return buffer;
}

int
cups_get_printer_status(struct printer *pr)
{
    /* HTTP connection to server */
    http_t 	*http;
    /* Destination */
    cups_dest_t *dest = NULL;
    int status = -1;
    char printer_reason[150];
    memset(printer_reason, 0, sizeof(printer_reason));
    int printer_avail = 0;
    int printer_state = 3;
    unsigned flags;
    /*
     * Make sure we don't ask for passwords...
     */
    cupsSetPasswordCB2(cups_passwd_cb, NULL);
    /*
     * Try to connect to the requested printer...
     */
    dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, pr->p_printer, NULL);

    if (!dest) {
        LOG(log_error, logtype_papd,
            "Unable to get destination \"%s\": %s", pr->p_printer, cupsLastErrorString());
        snprintf(pr->p_status, 255, "status: busy; info: \"%s\" appears to be offline.",
                 pr->p_printer);
        return 0;
    }

    /*
     * Collect the needed attributes...
     */
    const char *printer_uri_supported = cupsGetOption("printer-uri-supported",
                                        dest->num_options, dest->options);
    const char *printer_is_temporary = cupsGetOption("printer-is-temporary",
                                       dest->num_options, dest->options);
    memset(pr->p_status, 0, sizeof(pr->p_status));

    /* DNS-SD discovered IPP printer: Get status directly from printer */
    if (!printer_uri_supported || !strcmp(printer_is_temporary, "true")) {
        flags = CUPS_DEST_FLAGS_DEVICE;
    } else {
        flags = CUPS_DEST_FLAGS_NONE;
    }

    if ((http = cupsConnectDest(dest, flags, 30000, NULL, NULL, 0, NULL,
                                NULL)) == NULL) {
        LOG(log_error, logtype_papd,
            "Unable to connect to destination \"%s\": %s", dest->name,
            cupsLastErrorString());
        snprintf(pr->p_status, 255, "status: busy; info: \"%s\" appears to be offline.",
                 pr->p_printer);
        cupsFreeDests(1, dest);
        return 0;
    }

    /* IPP Request */
    ipp_t *request;
    /* IPP Response */
    ipp_t *response;
    /* Requested printer attributes */
    const char *pattrs[] = {
        "printer-state",
        "printer-is-accepting-jobs",
        "printer-state-reasons"
    };
    ipp_attribute_t *attr;
    /*
     * Build an IPP_OP_GET_PRINTER_ATTRIBUTES request, which requires the
     * following attributes:
     *
     *    requested-attributes
     *    printer-uri
     */
    request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);
    const char *uri = cupsGetOption("device-uri", dest->num_options, dest->options);

    if (flags == CUPS_DEST_FLAGS_DEVICE)
        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                     "printer-uri", NULL, uri);
    else
        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                     "printer-uri", NULL, printer_uri_supported);

    ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                  "requested-attributes",
                  (sizeof(pattrs) / sizeof(pattrs[0])), NULL, pattrs);

    /*
     * Do the request and get back a response...
     */

    if ((response = cupsDoRequest(http, request, "/")) == NULL) {
        /* Printer didn't respond to status request. Don't block print jobs as the scheduler will handle them */
        LOG(log_error, logtype_papd, "Unable to get printer attribs for %s - %s",
            pr->p_printer,
            ippErrorString(cupsLastError()));
        snprintf(pr->p_status, 255,
                 "status: busy; info: \"%s\" not responding to queries.", pr->p_printer);
        httpClose(http);
        cupsFreeDests(1, dest);
        return 1;
    }

    if ((attr = ippFindAttribute(response, "printer-state",
                                 IPP_TAG_ENUM)) != NULL) {
        printer_state = ippGetInteger(attr, 0);
    }

    if ((attr = ippFindAttribute(response, "printer-is-accepting-jobs",
                                 IPP_TAG_BOOLEAN)) != NULL) {
        printer_avail = ippGetBoolean(attr, 0);
    }

    if ((attr = ippFindAttribute(response, "printer-state-reasons",
                                 IPP_TAG_KEYWORD)) != NULL) {
        int i, count = ippGetCount(attr);

        for (i = 0; i < count; i++) {
            strlcat(printer_reason, ippGetString(attr, i, NULL), sizeof(printer_reason));

            if (i != (count - 1)) {
                strlcat(printer_reason, ", ", sizeof(printer_reason));
            }
        }
    }

    ippDelete(response);
    httpClose(http);

    /*
     * Get the current printer status and convert it to the status values.
     */

    if (printer_state == 5) {
        /* printer is stopped */
        status = 1;
    } else if (printer_state == 4) {
        /* printer is processing a job */
        status = 3;
    } else {
        /* ready */
        status = 2;
    }

    if (!printer_avail && printer_state != 4) {
        /* printer is rejecting jobs */
        status = 0;
    }

    snprintf(pr->p_status, 255, cups_status_msg[status], pr->p_printer);

    /* printer state */
    if (!strcmp(printer_reason, "none\n")) {
        strncat(pr->p_status, printer_reason, 255 - strlen(pr->p_status));
    }

    /*
     * Return the print status ...
     */
    cupsFreeDests(1, dest);
    return status;
}


/*------------------------------------------------------------------------*/

/* pass the job to cups */

int cups_print_job(char *name, const char *filename, char *job, char *username,
                   char *cupsoptions)
{
    /* HTTP connection to server */
    http_t *http;
    /* Destination */
    cups_dest_t	*dest = NULL;
    cups_dinfo_t *info = NULL;
    int jobid;
    char filepath[MAXPATHLEN];
    int num_options;
    cups_option_t *options;
    /* Initialize the options array */
    num_options = 0;
    options = (cups_option_t *) 0;
    /*
     * Make sure we don't ask for passwords...
     */
    cupsSetPasswordCB2(cups_passwd_cb, NULL);
    /*
     * Try to connect to the requested printer...
     */
    dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, name, NULL);

    if (!dest) {
        LOG(log_error, logtype_papd,
            "Unable to get destination \"%s\": %s", name, cupsLastErrorString());
        cupsFreeDests(1, dest);
        return 0;
    }

    info = cupsCopyDestInfo(CUPS_HTTP_DEFAULT, dest);

    if (username != NULL) {
        /* Add options using cupsAddOption() */
        num_options = cupsAddOption("job-originating-user-name", username, num_options,
                                    &options);
        num_options = cupsAddOption("originating-user-name", username, num_options,
                                    &options);
        cupsSetUser(username);
    }

    if (cupsoptions != NULL) {
        num_options = cupsParseOptions(cupsoptions, num_options, &options);
    }

    strlcpy(filepath, SPOOLDIR, sizeof(filepath));
    strlcat(filepath, "/", sizeof(filepath));
    strlcat(filepath, filename, sizeof(filepath));

    /*
     * Create a new print job.
     */

    if (cupsCreateDestJob(CUPS_HTTP_DEFAULT, dest, info, &jobid, job, num_options,
                          options) == IPP_STATUS_OK) {
        LOG(log_info, logtype_papd, "Job '%s' queued to printer '%s' with id '%d'", job,
            name, jobid);
    } else {
        LOG(log_error, logtype_papd,
            "Unable to print job '%s' to printer '%s' for user '%s' - CUPS error : '%s'",
            job, name, username, cupsLastErrorString());
    }

    /*
     * Send the job off to the printer.
     */
    FILE *fp = fopen(filepath, "rb");

    if (fp == NULL) {
        LOG(log_error, logtype_papd, "Unable to open file '%s' for printing: %s",
            filepath, strerror(errno));
        cupsFreeOptions(num_options, options);
        cupsFreeDests(1, dest);
        return 0;
    }

    size_t bytes;
    char buffer[65536];

    if (cupsStartDestDocument(CUPS_HTTP_DEFAULT, dest, info, jobid, job,
                              CUPS_FORMAT_AUTO, 0, NULL, true) == HTTP_STATUS_CONTINUE) {
        while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0)
            if (cupsWriteRequestData(CUPS_HTTP_DEFAULT, buffer,
                                     bytes) != HTTP_STATUS_CONTINUE) {
                break;
            }

        if (cupsFinishDestDocument(CUPS_HTTP_DEFAULT, dest, info) == IPP_STATUS_OK) {
            LOG(log_info, logtype_papd, "Document send succeeded.");
        } else {
            LOG(log_error, logtype_papd, "Document send failed: %s\n",
                cupsLastErrorString());
        }
    }

    fclose(fp);
    cupsFreeOptions(num_options, options);
    cupsFreeDests(1, dest);
    return jobid;
}


/*------------------------------------------------------------------------*/

struct printer	*
cups_autoadd_printers(struct printer	*defprinter, struct printer *printers)
{
    struct printer 	*pr;
    int         	num_dests, i;
    int 	    	ret;
    cups_dest_t 	*dests;
    char 	    	name[MAXCHOOSERLEN + 1], *p;
    /* get the available destination from CUPS */
    num_dests = cupsGetDests2(CUPS_HTTP_DEFAULT, &dests);

    for (i = 0; i < num_dests; i++) {
        if ((pr = (struct printer *)malloc(sizeof(struct printer)))	== NULL) {
            LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
            exit(1);
        }

        memcpy(pr, defprinter, sizeof(struct printer));
        /* convert from CUPS to local encoding */
        convert_string_allocate(add_charset(cups_get_language()), CH_UNIX,
                                dests[i].name, -1, &pr->p_u_name);

        /* convert CUPS name to Mac charset */
        if (convert_to_mac_name(cups_get_language(), dests[i].name, name,
                                sizeof(name)) <= 0) {
            LOG(log_error, logtype_papd, "Conversion from CUPS to MAC name failed for %s",
                dests[i].name);
            free(pr);
            continue;
        }

        if ((pr->p_name = (char *)malloc(strlen(name) + 1)) == NULL) {
            LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
            exit(1);
        }

        strcpy(pr->p_name, name);
        /* set printer flags */
        pr->p_flags &= ~P_PIPED;
        pr->p_flags |= P_SPOOLED;
        pr->p_flags |= P_CUPS;
        pr->p_flags |= P_CUPS_AUTOADDED;

        if ((pr->p_printer = (char *)malloc(strlen(dests[i].name) + 1)) == NULL) {
            LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
            exit(1);
        }

        strcpy(pr->p_printer, dests[i].name);

        if ((p = (char *) cups_get_printer_ppd(pr->p_printer)) != NULL) {
            if ((pr->p_ppdfile = (char *)malloc(strlen(p) + 1)) == NULL) {
                LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
                exit(1);
            }

            strcpy(pr->p_ppdfile, p);
            pr->p_flags |= P_CUPS_PPD;
        }

        if ((ret = cups_check_printer(pr, printers, 0)) == -1) {
            ret = cups_mangle_printer_name(pr, printers);
        }

        if (ret) {
            cups_free_printer(pr);
            LOG(log_info, logtype_papd, "Printer %s not added: reason %d", name, ret);
        } else {
            pr->p_next = printers;
            printers = pr;
        }
    }

    cupsFreeDests(num_dests, dests);
    return printers;
}


/*------------------------------------------------------------------------*/

/* cups_mangle_printer_name
 *    Mangles the printer name if two CUPS printer provide the same Chooser Name
 *    Append '#nn' to the chooser name, if it is longer than 28 char we overwrite the last three chars
 * Returns: 0 on Success, 2 on Error
 */

static int cups_mangle_printer_name(struct printer *pr,
                                    struct printer *printers)
{
    size_t 	count, name_len;
    char	name[MAXCHOOSERLEN];
    count = 1;
    name_len = strlen(pr->p_name);
    strncpy(name, pr->p_name, MAXCHOOSERLEN - 3);
    /* Reallocate necessary space */
    (name_len >= MAXCHOOSERLEN - 3) ? (name_len = MAXCHOOSERLEN + 1) :
    (name_len = name_len + 4);
    pr->p_name = (char *) realloc(pr->p_name, name_len);

    while ((cups_check_printer(pr, printers, 0)) && count < 100) {
        memset(pr->p_name, 0, name_len);
        sprintf(pr->p_name, "%s#%2.2lu", name, (unsigned long) count++);
    }

    if (count > 99) {
        return 2;
    }

    return 0;
}

/*------------------------------------------------------------------------*/

/* fallback ASCII conversion */

static size_t
to_ascii(char  *inptr, char **outptr)
{
    char *out, *osav;

    if (NULL == (out = (char *) malloc(strlen(inptr) + 1))) {
        LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
        exit(1);
    }

    osav = out;

    while (*inptr != '\0') {
        if (*inptr & 0x80) {
            *out = '_';
            out++;
            inptr++;
        } else {
            *out++ = *inptr++;
        }
    }

    *out = '\0';
    *outptr = osav;
    return strlen(osav);
}


/*------------------------------------------------------------------------*/

/* convert_to_mac_name
 *  1) Convert from encoding to MacRoman
 *  2) Shorten to MAXCHOOSERLEN (31)
 *  3) Replace @ and _ as they are illegal
 * Returns: -1 on failure, length of name on success; outpr contains name in MacRoman
 */

static int convert_to_mac_name(const char *encoding, char *inptr,
                               char *outptr, size_t outlen)
{
    char   	*outbuf;
    char	*soptr;
    size_t 	name_len = 0;
    size_t 	i;
    charset_t chCups;

    /* Change the encoding */
    if ((charset_t) -1 != (chCups = add_charset(encoding))) {
        name_len = convert_string_allocate(chCups, CH_MAC, inptr, -1, &outbuf);
    }

    if (name_len == 0 || name_len == (size_t) -1) {
        /* charset conversion failed, use ascii fallback */
        name_len = to_ascii(inptr, &outbuf);
    }

    soptr = outptr;

    for (i = 0; i < name_len && i < outlen - 1 ; i++) {
        if (outbuf[i] == '_') {
            /* Replace '_' with a space (just for the looks) */
            *soptr = ' ';
        } else if (outbuf[i] == '@' || outbuf[i] == ':') {
            /* Replace @ and : with '_' as they are illegal chars */
            *soptr = '_';
        } else {
            *soptr = outbuf[i];
        }

        soptr++;
    }

    *soptr = '\0';
    free(outbuf);
    return i;
}


/*------------------------------------------------------------------------*/

/*
 * cups_check_printer:
 * check if a printer with this name already exists.
 * if yes, and replace = 1 the existing printer is replaced with
 * the new one. This allows to overwrite printer settings
 * created by cupsautoadd. It also used by cups_mangle_printer.
 */

int cups_check_printer(struct printer *pr, struct printer *printers,
                       int replace)
{
    struct printer *listptr, *listprev;
    listptr = printers;
    listprev = NULL;

    while (listptr != NULL) {
        if (strcasecmp(pr->p_name, listptr->p_name) == 0) {
            /* Check if printer has been autoadded */
            if (pr->p_flags & P_CUPS_AUTOADDED) {
                if (listptr->p_flags & P_CUPS_AUTOADDED) {
                    /* Conflicting Cups Auto Printer (mangling issue?) */
                    return -1;
                } else {
                    /* Never replace a hand edited printer with auto one */
                    return 1;
                }
            }

            if (replace) {
                /* Replace printer */
                if (listprev != NULL) {
                    pr->p_next = listptr->p_next;
                    listprev->p_next = pr;
                    cups_free_printer(listptr);
                } else {
                    printers = pr;
                    printers->p_next = listptr->p_next;
                    cups_free_printer(listptr);
                }
            }

            /* Conflicting Printers */
            return 1;
        }

        listprev = listptr;
        listptr = listptr->p_next;
    }

    /* No conflict */
    return 0;
}


/*------------------------------------------------------------------------*/


void
cups_free_printer(struct printer *pr)
{
    if (pr->p_name != NULL) {
        free(pr->p_name);
    }

    if (pr->p_printer != NULL) {
        free(pr->p_printer);
    }

    if (pr->p_ppdfile != NULL) {
        free(pr->p_ppdfile);
    }

    /* CUPS autoadded printers share the other informations
     * so if the printer is autoadded we won't free them.
     * We might leak some bytes here though.
     */
    if (pr->p_flags & P_CUPS_AUTOADDED) {
        free(pr);
        return;
    }

    if (pr->p_operator != NULL) {
        free(pr->p_operator);
    }

    if (pr->p_zone != NULL) {
        free(pr->p_zone);
    }

    if (pr->p_type != NULL) {
        free(pr->p_type);
    }

    if (pr->p_authprintdir != NULL) {
        free(pr->p_authprintdir);
    }

    free(pr);
    return;
}

#endif /* HAVE_CUPS*/
