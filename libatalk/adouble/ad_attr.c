/*
 * $Id: ad_attr.c,v 1.4 2002-09-29 17:39:59 didg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <atalk/adouble.h>

#define FILEIOFF_ATTR 14
#define AFPFILEIOFF_ATTR 2

int ad_getattr(const struct adouble *ad, u_int16_t *attr)
{
  if (ad->ad_version == AD_VERSION1)
    memcpy(attr, ad_entry(ad, ADEID_FILEI) + FILEIOFF_ATTR,
	   sizeof(u_int16_t));
#if AD_VERSION == AD_VERSION2
  else if (ad->ad_version == AD_VERSION2)
    memcpy(attr, ad_entry(ad, ADEID_AFPFILEI) + AFPFILEIOFF_ATTR,
	   sizeof(u_int16_t));
#endif
  else 
    return -1;

  return 0;
}

int ad_setattr(const struct adouble *ad, const u_int16_t attr)
{
  if (ad->ad_version == AD_VERSION1)
    memcpy(ad_entry(ad, ADEID_FILEI) + FILEIOFF_ATTR, &attr,
	   sizeof(attr));
#if AD_VERSION == AD_VERSION2
  else if (ad->ad_version == AD_VERSION2)
    memcpy(ad_entry(ad, ADEID_AFPFILEI) + AFPFILEIOFF_ATTR, &attr,
	   sizeof(attr));
#endif
  else 
    return -1;

  return 0;
}
