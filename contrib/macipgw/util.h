/*
 *
 * (c) 1997 Stefan Bethke. All rights reserved.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

extern int gDebug;

#if defined(DEBUG)
#define DEBUG_MACIP		(0x0001)
#define DEBUG_ROUTE		(0x0010)
#define DEBUG_TUNNEL	(0x0100)
#define DEBUG_TUNDEV	(0x0200)
#else
#define DEBUG_MACIP		(0)
#define DEBUG_ROUTE		(0)
#define DEBUG_TUNNEL	(0)
#define DEBUG_TUNDEV	(0)
#endif

extern char *iptoa (u_long ip);
extern u_long atoip (char *s);
