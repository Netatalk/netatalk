/*
   $Id: extattrs.h,v 1.2 2009-10-02 09:32:40 franklahm Exp $
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#ifndef AFPD_EXT_ATTRS_H 
#define AFPD_EXT_ATTRS_H

/* AFP funcs */
extern int afp_listextattr(AFPObj *obj _U_, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen);
extern int afp_getextattr(AFPObj *obj _U_ , char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen);
extern int afp_setextattr(AFPObj *obj _U_, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen);
extern int afp_remextattr(AFPObj *obj _U_, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen);

#endif /* AFPD_EXT_ATTRS_H */
