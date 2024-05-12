#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <string.h>

#include <atalk/adouble.h>

int ad_setdate(struct adouble *ad,
               unsigned int dateoff, uint32_t date)
{
    int xlate = (dateoff & AD_DATE_UNIX);
    char *ade = NULL;

    dateoff &= AD_DATE_MASK;
    if (xlate)
        date = AD_DATE_FROM_UNIX(date);

    if (!ad_getentryoff(ad, ADEID_FILEDATESI) || !ad_entry(ad, ADEID_FILEDATESI))
        return -1;

    if (dateoff > AD_DATE_ACCESS)
        return -1;

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
    if (!ad_getentryoff(ad, ADEID_FILEDATESI) || !ad_entry(ad, ADEID_FILEDATESI))
        return -1;

    if (dateoff > AD_DATE_ACCESS)
        return -1;


    ade = ad_entry(ad, ADEID_FILEDATESI);
    if (ade == NULL) {
        return -1;
    }
    memcpy(date, ade + dateoff, sizeof(uint32_t));

    if (xlate)
        *date = AD_DATE_TO_UNIX(*date);
    return 0;
}
