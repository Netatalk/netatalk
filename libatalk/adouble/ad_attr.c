#include <string.h>
#include <atalk/adouble.h>

#define FILEIOFF_ATTR 14
#define AFPFILEIOFF_ATTR 2

int ad_getattr(const struct adouble *ad, u_int16_t *attr)
{
  if (ad->ad_version == AD_VERSION1)
    memcpy(attr, ad_entry(ad, ADEID_FILEI) + FILEIOFF_ATTR,
	   sizeof(u_int16_t));
  else if (ad->ad_version == AD_VERSION2)
    memcpy(attr, ad_entry(ad, ADEID_AFPFILEI) + AFPFILEIOFF_ATTR,
	   sizeof(u_int16_t));
  else 
    return -1;

  return 0;
}

int ad_setattr(const struct adouble *ad, const u_int16_t attr)
{
  if (ad->ad_version == AD_VERSION1)
    memcpy(ad_entry(ad, ADEID_FILEI) + FILEIOFF_ATTR, &attr,
	   sizeof(attr));
  else if (ad->ad_version == AD_VERSION2)
    memcpy(ad_entry(ad, ADEID_AFPFILEI) + AFPFILEIOFF_ATTR, &attr,
	   sizeof(attr));
  else 
    return -1;

  return 0;
}
