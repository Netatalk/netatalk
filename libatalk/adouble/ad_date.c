#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/adouble.h>

int ad_setdate(const struct adouble *ad, 
	       unsigned int dateoff, u_int32_t date)
{
  int xlate = (dateoff & AD_DATE_UNIX);

  dateoff &= AD_DATE_MASK;
  if (xlate)
    date = AD_DATE_FROM_UNIX(date);

  if (ad->ad_version == AD_VERSION1) {
    if (dateoff > AD_DATE_BACKUP)
      return -1;
    memcpy(ad_entry(ad, ADEID_FILEI) + dateoff, &date, sizeof(date));

  } else if (ad->ad_version == AD_VERSION2) {
    if (dateoff > AD_DATE_ACCESS)
      return -1;
    memcpy(ad_entry(ad, ADEID_FILEDATESI) + dateoff, &date, sizeof(date));

  } else 
    return -1;

  return 0;
}

int ad_getdate(const struct adouble *ad,
	       unsigned int dateoff, u_int32_t *date) 
{
  int xlate = (dateoff & AD_DATE_UNIX);

  dateoff &= AD_DATE_MASK;
  if (ad->ad_version == AD_VERSION1) {
    if (dateoff > AD_DATE_BACKUP)
      return -1;
    memcpy(date, ad_entry(ad, ADEID_FILEI) + dateoff, sizeof(u_int32_t));

  } else if (ad->ad_version == AD_VERSION2) {
    if (dateoff > AD_DATE_ACCESS)
      return -1;
    memcpy(date, ad_entry(ad, ADEID_FILEDATESI) + dateoff, sizeof(u_int32_t));

  } else 
    return -1;

  if (xlate)
    *date = AD_DATE_TO_UNIX(*date);
  return 0;
}
