/*
 * Author:   Lee Essen <lee.essen@nowonline.co.uk>
 * Based on: avahi support from Daniel S. Haischt <me@daniel.stefan.haischt.name>
 * Purpose:  mdns based Zeroconf support
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MDNS

#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <poll.h>

#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/unicode.h>
#include <atalk/netatalk_conf.h>

#include "afp_zeroconf.h"
#include "afp_mdns.h"

/*
 * We'll store all the DNSServiceRef's here so that we can
 * deallocate them later
 */
static DNSServiceRef   *svc_refs = NULL;
static int             svc_ref_count = 0;
static pthread_t       poller;

/*
 * Its easier to use asprintf to set the TXT record values
 */

int TXTRecordPrintf(TXTRecordRef * rec, const char * key, const char * fmt, ... ) 
{
    int ret = 0;
    char *str;
    va_list ap;
    va_start( ap, fmt );

    if( 0 > vasprintf(&str, fmt, ap ) ) {
        va_end(ap);
        return -1;    
    }
    va_end(ap);

    if( kDNSServiceErr_NoError != TXTRecordSetValue(rec, key, strlen(str), str) ) {
        ret = -1;
    }

    free(str);
    return ret;
}

int TXTRecordKeyPrintf(TXTRecordRef * rec, const char * key_fmt, int key_var, const char * fmt, ...) 
{
    int ret = 0;
    char *key = NULL, *str = NULL;
    va_list ap;

    if( 0 > asprintf(&key, key_fmt, key_var))
        return -1;

    va_start( ap, fmt );
    if( 0 > vasprintf(&str, fmt, ap )) {
        va_end(ap);
        ret = -1;
        goto exit;
    }
    va_end(ap);

    if( kDNSServiceErr_NoError != TXTRecordSetValue(rec, key, strlen(str), str) ) {
        ret = -1;
        goto exit;
    }

exit:
    if (str)
        free(str);
    if (key)
        free(key);
    return ret;
}

static struct pollfd *fds;

/*
 * This is the thread that polls the filehandles
 */
static void *polling_thread(void *arg) {
    // First we loop through getting the filehandles and adding them to our poll, we
    // need to allocate our pollfd's
    DNSServiceErrorType error;
    fds = calloc(svc_ref_count, sizeof(struct pollfd));
    assert(fds);

    for(int i=0; i < svc_ref_count; i++) {
        int fd = DNSServiceRefSockFD(svc_refs[i]);
        fds[i].fd = fd;
        fds[i].events = POLLIN;
    }

    // Now we can poll and process the results...
    while(poll(fds, svc_ref_count, -1) > 0) {
        for(int i=0; i < svc_ref_count; i++) {
            if(fds[i].revents & POLLIN) {
                error = DNSServiceProcessResult(svc_refs[i]);
            }
        }
    }
    return(NULL);
}

/*
 * This is the callback for the service register function ... actually there isn't a lot
 * we can do if we get problems, so we don't really need to do anything other than report
 * the issue.
 */
static void RegisterReply(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode,
                          const char *name, const char *regtype, const char *domain, void *context)
{
    if (errorCode != kDNSServiceErr_NoError) {
        LOG(log_error, logtype_afpd, "Failed to register mDNS service: %s%s%s: code=%d",
            name, regtype, domain, errorCode);
    }
}

/*
 * This function unregisters anything we have already
 * registered and frees associated memory
 */
static void unregister_stuff() {
    pthread_cancel(poller);    

    for (int i = 0; i < svc_ref_count; i++)
        close(fds[i].fd);
    free(fds);
    fds = NULL;

    if(svc_refs) {
        for(int i=0; i < svc_ref_count; i++) {
            DNSServiceRefDeallocate(svc_refs[i]);
        }
        free(svc_refs);
        svc_refs = NULL;
        svc_ref_count = 0;
    }
}

/*
 * This function tries to register the AFP DNS
 * SRV service type.
 */
static void register_stuff(const AFPObj *obj) {
    uint                                        port;
    const struct vol                *volume;
    char                                        name[MAXINSTANCENAMELEN+1];
    DNSServiceErrorType         error;
    TXTRecordRef                        txt_adisk;
    TXTRecordRef                        txt_devinfo;
    char                                        tmpname[256];

    // If we had already registered, then we will unregister and re-register
    if(svc_refs) unregister_stuff();

    /* Register our service, prepare the TXT record */
    TXTRecordCreate(&txt_adisk, 0, NULL);
    if( 0 > TXTRecordPrintf(&txt_adisk, "sys", "waMa=0,adVF=0x100") ) {
        LOG ( log_error, logtype_afpd, "Could not create Zeroconf TXTRecord for sys");
        goto fail;
    }

    /* Build AFP volumes list */
    int i = 0;

    for (volume = getvolumes(); volume; volume = volume->v_next) {

        if (convert_string(CH_UCS2, CH_UTF8_MAC, volume->v_u8mname, -1, tmpname, 255) <= 0) {
            LOG ( log_error, logtype_afpd, "Could not set Zeroconf volume name for TimeMachine");
            goto fail;
        }

        if (volume->v_flags & AFPVOL_TM) {
            if (volume->v_uuid) {
                LOG(log_info, logtype_afpd, "Registering volume '%s' with UUID: '%s' for TimeMachine",
                    volume->v_localname, volume->v_uuid);
                if( 0 > TXTRecordKeyPrintf(&txt_adisk, "dk%u", i++, "adVN=%s,adVF=0xa1,adVU=%s",
                                   tmpname, volume->v_uuid) ) {
                    LOG ( log_error, logtype_afpd, "Could not set Zeroconf TXTRecord for dk%u", i);
                    goto fail;
                }
            } else {
                LOG(log_warning, logtype_afpd, "Registering volume '%s' for TimeMachine. But UUID is invalid.",
                    volume->v_localname);
                if( 0 > TXTRecordKeyPrintf(&txt_adisk, "dk%u", i++, "adVN=%s,adVF=0xa1", tmpname) ) {
                    LOG ( log_error, logtype_afpd, "Could not set Zeroconf TXTRecord for dk%u", i);
                    goto fail;
                }
            }
        }
    }

    /* AFP_DNS_SERVICE_TYPE */
    svc_ref_count = 1;
    if (i) {
        /* ADISK_SERVICE_TYPE */
        svc_ref_count++;
    }
    if (obj->options.mimicmodel) {
        /* DEV_INFO_SERVICE_TYPE */
        svc_ref_count++;
    }

    // Allocate the memory to store our service refs
    svc_refs = calloc(svc_ref_count, sizeof(DNSServiceRef));
    assert(svc_refs);
    svc_ref_count = 0;

    port = atoi(obj->options.port);

    if (convert_string(obj->options.unixcharset,
                       CH_UTF8,
                       obj->options.hostname,
                       -1,
                       name,
                       MAXINSTANCENAMELEN) <= 0) {
        LOG(log_error, logtype_afpd, "Could not set Zeroconf instance name");
        goto fail;
    }

    error = DNSServiceRegister(&svc_refs[svc_ref_count++],
                               0,               // no flags
                               0,               // all network interfaces
                               name,
                               AFP_DNS_SERVICE_TYPE,
                               "",            // default domains
                               NULL,            // default host name
                               htons(port),
                               0,               // length of TXT
                               NULL,            // no TXT
                               RegisterReply,           // callback
                               NULL);       // no context
    if (error != kDNSServiceErr_NoError) {
        LOG(log_error, logtype_afpd, "Failed to add service: %s, error=%d",
            AFP_DNS_SERVICE_TYPE, error);
        goto fail;
    }

    if (i) {
        error = DNSServiceRegister(&svc_refs[svc_ref_count++],
                                   0,               // no flags
                                   0,               // all network interfaces
                                   name,
                                   ADISK_SERVICE_TYPE,
                                   "",            // default domains
                                   NULL,            // default host name
                                   htons(port),
                                   TXTRecordGetLength(&txt_adisk),
                                   TXTRecordGetBytesPtr(&txt_adisk),
                                   RegisterReply,           // callback
                                   NULL);       // no context
        if (error != kDNSServiceErr_NoError) {
            LOG(log_error, logtype_afpd, "Failed to add service: %s, error=%d",
                ADISK_SERVICE_TYPE, error);
            goto fail;
        }
    }

    if (obj->options.mimicmodel) {
        LOG(log_info, logtype_afpd, "Registering server as model '%s'",
            obj->options.mimicmodel);
        TXTRecordCreate(&txt_devinfo, 0, NULL);
        if ( 0 > TXTRecordPrintf(&txt_devinfo, "model", obj->options.mimicmodel) ) {
            LOG ( log_error, logtype_afpd, "Could not create Zeroconf TXTRecord for model");
            goto fail;
        }

        error = DNSServiceRegister(&svc_refs[svc_ref_count++],
                                   0,               // no flags
                                   0,               // all network interfaces
                                   name,
                                   DEV_INFO_SERVICE_TYPE,
                                   "",            // default domains
                                   NULL,            // default host name
                                   /*
                                    * We would probably use port 0 zero, but we can't, from man DNSServiceRegister:
                                    *   "A value of 0 for a port is passed to register placeholder services.
                                    *    Place holder services are not found  when browsing, but other
                                    *    clients cannot register with the same name as the placeholder service."
                                    * We therefor use port 9 which is used by the adisk service type.
                                    */
                                   htons(9),
                                   TXTRecordGetLength(&txt_devinfo),
                                   TXTRecordGetBytesPtr(&txt_devinfo),
                                   RegisterReply,           // callback
                                   NULL);       // no context
        TXTRecordDeallocate(&txt_devinfo);
        if (error != kDNSServiceErr_NoError) {
            LOG(log_error, logtype_afpd, "Failed to add service: %s, error=%d",
                DEV_INFO_SERVICE_TYPE, error);
            goto fail;
        }
    } /* if (config->obj.options.mimicmodel) */

    /*
     * Now we can create the thread that will poll for the results
     * and handle the calling of the callbacks
     */
    if(pthread_create(&poller, NULL, polling_thread, NULL) != 0) {
        LOG(log_error, logtype_afpd, "Unable to start mDNS polling thread");
        goto fail;
    }

fail:
    TXTRecordDeallocate(&txt_adisk);
    return;
}

/************************************************************************
 * Public funcions
 ************************************************************************/

/*
 * Tries to setup the Zeroconf thread and any
 * neccessary config setting.
 */
void md_zeroconf_register(const AFPObj *obj) {
    int error;

    register_stuff(obj);
    return;
}

/*
 * Tries to shutdown this loop impl.
 * Call this function from inside this thread.
 */
int md_zeroconf_unregister() {
    unregister_stuff();
    return 0;
}

#endif /* USE_MDNS */

