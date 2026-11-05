#!/usr/sbin/dtrace -s
/*
 * The system calls of one afpd session process: in a run of their own, as
 * these probes add to each request's time, or alone for an afpd built
 * without netatalk's DTrace provider.
 *
 * usage: dtrace -q -s afpd_syscalls.d -p <afpd session pid> -o syscalls.txt
 *        stop with ^C or SIGTERM; the summary prints on exit
 *
 * Per system call: count, total time, and a latency histogram for the calls
 * that walk paths (chdir, __getcwd, openat, fstatat and the extended
 * attribute and ACL calls).
 */

#pragma D option quiet
#pragma D option dynvarsize=16m
/* a system call this FreeBSD release lacks has no probe */
#pragma D option zdefs

syscall:::entry
/pid == $target/
{
	self->s0 = timestamp;
}

syscall:::return
/pid == $target && self->s0/
{
	@n[probefunc] = count();
	@ns[probefunc] = sum(timestamp - self->s0);
	self->s0 = 0;
}

syscall::chdir:entry, syscall::fchdir:entry, syscall::__getcwd:entry,
syscall::openat:entry, syscall::fstatat:entry,
syscall::extattr_get_file:entry, syscall::extattr_get_link:entry,
syscall::extattr_get_fd:entry, syscall::extattr_list_file:entry,
syscall::extattr_list_link:entry, syscall::extattr_list_fd:entry,
syscall::__acl_get_file:entry, syscall::__acl_get_link:entry,
syscall::__acl_get_fd:entry
/pid == $target/
{
	self->w0 = timestamp;
}

syscall::chdir:return, syscall::fchdir:return, syscall::__getcwd:return,
syscall::openat:return, syscall::fstatat:return,
syscall::extattr_get_file:return, syscall::extattr_get_link:return,
syscall::extattr_get_fd:return, syscall::extattr_list_file:return,
syscall::extattr_list_link:return, syscall::extattr_list_fd:return,
syscall::__acl_get_file:return, syscall::__acl_get_link:return,
syscall::__acl_get_fd:return
/self->w0/
{
	@lat[probefunc] = quantize((timestamp - self->w0) / 1000);
	self->w0 = 0;
}

END
{
	printf("== system calls: count, total ns\n");
	printa("SYS %-24s %@10d %@16d\n", @n, @ns);
	printf("\n== path-walking calls, usec\n");
	printa(@lat);
}
