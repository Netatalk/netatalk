#!/usr/sbin/dtrace -s
/*
 * What one afpd session's path lookups cost ZFS on FreeBSD: name-cache
 * misses, reverse lookups from a vnode to its name, and the directory scans
 * those take.
 *
 * usage: dtrace -q -s zfs_lookups.d -o zfs.txt PID
 *        PID is the session's afpd; stop with ^C or SIGTERM, and the summary
 *        prints on exit
 *
 * zfs_freebsd_cachedlookup  a forward lookup the name cache could not answer
 * zfs_lookup                every forward lookup ZFS serves: those misses,
 *                           and all of them on a case-insensitive or
 *                           normalising dataset, which keeps no name cache
 * zfs_vptocnp               a reverse lookup getcwd() could not take from
 *                           the name cache
 * zap_value_search          the scan of a parent folder's entries that a
 *                           reverse lookup takes, its time the cost
 * vn_fullpath_dir           getcwd()'s locked walk up the name cache, taken
 *                           only when its lockless walk fails
 */

#pragma D option quiet
/* a function this kernel inlines or names otherwise has no probe */
#pragma D option zdefs

fbt::zfs_freebsd_cachedlookup:entry
/pid == $1/
{
	@n["zfs_freebsd_cachedlookup"] = count();
}

fbt::zfs_lookup:entry
/pid == $1/
{
	@n["zfs_lookup"] = count();
}

fbt::zfs_vptocnp:entry
/pid == $1/
{
	@n["zfs_vptocnp"] = count();
}

fbt::zap_value_search:entry
/pid == $1/
{
	self->z0 = timestamp;
}

fbt::zap_value_search:return
/self->z0/
{
	@n["zap_value_search"] = count();
	@ns["zap_value_search"] = sum(timestamp - self->z0);
	@scan = quantize((timestamp - self->z0) / 1000);
	self->z0 = 0;
}

fbt::vn_fullpath_dir:entry
/pid == $1/
{
	self->f0 = timestamp;
}

fbt::vn_fullpath_dir:return
/self->f0/
{
	@n["vn_fullpath_dir"] = count();
	@ns["vn_fullpath_dir"] = sum(timestamp - self->f0);
	self->f0 = 0;
}

END
{
	printf("== kernel functions called for afpd: count\n");
	printa("ZFS %-28s %@10d\n", @n);
	printf("\n== their time, ns\n");
	printa("ZFSNS %-28s %@16d\n", @ns);
	printf("\n== zap_value_search scans, usec\n");
	printa(@scan);
}
