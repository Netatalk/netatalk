#!/usr/sbin/dtrace -s
/*
 * Per-request view of one afpd session process, through netatalk's own
 * DTrace provider (built when meson's with-dtrace option is on, the default).
 *
 * usage: dtrace -q -s afpd_requests.d -p <afpd session pid> -o requests.txt
 *        stop with ^C or SIGTERM; the summary prints on exit
 *
 * Lines "REQ <usec since start> <command> <usec in afpd>" give the timeline.
 * The summary gives, per AFP command, its count and time, and the CNID calls
 * it made and their time. A probe on every system call would add its own
 * cost to each request in step with the calls it makes, so the system calls
 * come from afpd_syscalls.d, in a run of their own (collect.sh -s).
 */

#pragma D option quiet
#pragma D option dynvarsize=32m
#pragma D option bufsize=16m

BEGIN
{
	t_begin = timestamp;
}

afp$target:::afpfunc-start
{
	self->name = copyinstr(arg1);
	self->t0 = timestamp;
}

afp$target:::cnid-start
/self->t0/
{
	self->cnid = copyinstr(arg0);
	self->c0 = timestamp;
}

afp$target:::cnid-done
/self->c0/
{
	@cnid[self->name, self->cnid] = count();
	@cnidns[self->name, self->cnid] = sum(timestamp - self->c0);
	self->c0 = 0;
}

afp$target:::afpfunc-done
/self->t0/
{
	printf("REQ %d %s %d\n", (timestamp - t_begin) / 1000, self->name,
	    (timestamp - self->t0) / 1000);
	@n[self->name] = count();
	@ns[self->name] = sum(timestamp - self->t0);
	@lat[self->name] = quantize((timestamp - self->t0) / 1000);
	self->t0 = 0;
}

END
{
	printf("\n== AFP commands: count, total ns\n");
	printa("CMD %-24s %@10d %@16d\n", @n, @ns);
	printf("\n== CNID calls per AFP command: count, total ns\n");
	printa("CNID %-24s %-24s %@10d %@16d\n", @cnid, @cnidns);
	printf("\n== AFP command latency in afpd, usec\n");
	printa(@lat);
}
