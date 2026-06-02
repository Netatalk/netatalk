#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <atalk/adouble.h>
#include <atalk/logger.h>

/*!
 * @brief Convert an AFP date-time value between local time and UTC.
 * @param aint_p AFP date-time to be converted.
 * @param offset_dir Which direction we want to convert the timestamp.
 */
int set_utc_offset(uint32_t *aint_p, ad_time_offset_dir_t offset_dir)
{
    /* If we somehow pass the earliest AFP date-time available, just exit. */
    if (*aint_p == AD_DATE_START) {
        return 0;
    }

    struct timeval tv;

    tv.tv_sec = AD_DATE_TO_UNIX(*aint_p);
    tv.tv_usec = 0;
    struct tm tm;

    if (offset_dir == TO_LOCALTIME) {
        if (localtime_r(&tv.tv_sec, &tm) == NULL) {
            LOG(log_error, logtype_default, "set_utc_offset: localtime_r: %s",
                strerror(errno));
            return -1;
        }

        tv.tv_sec = timegm(&tm);
        *aint_p = AD_DATE_FROM_UNIX(tv.tv_sec);
    } else {
        /*
         * This won't work reliably during the transition to/from DST.
         * The timestamp may be a hour off during the transistion back
         * to standard time at 0200 hours.
         */
        if (gmtime_r(&tv.tv_sec, &tm) == NULL) {
            LOG(log_error, logtype_default, "set_utc_offset: gmtime_r: %s",
                strerror(errno));
            return -1;
        }

        tm.tm_isdst = -1;
        tv.tv_sec = mktime(&tm);
        *aint_p = AD_DATE_FROM_UNIX(tv.tv_sec);
    }

    return 0;
}

int ad_setdate(struct adouble *ad,
               unsigned int dateoff, uint32_t date)
{
    int xlate = (dateoff & AD_DATE_UNIX);
    char *ade = NULL;
    dateoff &= AD_DATE_MASK;

    if (xlate) {
        date = AD_DATE_FROM_UNIX(date);
    }

    if (!ad_getentryoff(ad, ADEID_FILEDATESI) || !ad_entry(ad, ADEID_FILEDATESI)) {
        return -1;
    }

    if (dateoff > AD_DATE_ACCESS) {
        return -1;
    }

    ade = ad_entry(ad, ADEID_FILEDATESI);

    if (ade == NULL) {
        return -1;
    }

    memcpy(ade + dateoff, &date, sizeof(date));
    return 0;
}

int ad_getdate(const struct adouble *ad,
               unsigned int dateoff, uint32_t *date)
{
    int xlate = (dateoff & AD_DATE_UNIX);
    char *ade = NULL;
    dateoff &= AD_DATE_MASK;

    if (!ad_getentryoff(ad, ADEID_FILEDATESI) || !ad_entry(ad, ADEID_FILEDATESI)) {
        return -1;
    }

    if (dateoff > AD_DATE_ACCESS) {
        return -1;
    }

    ade = ad_entry(ad, ADEID_FILEDATESI);

    if (ade == NULL) {
        return -1;
    }

    memcpy(date, ade + dateoff, sizeof(uint32_t));

    if (xlate) {
        *date = AD_DATE_TO_UNIX(*date);
    }

    return 0;
}
