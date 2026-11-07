#!/usr/sbin/dtrace -s
/*
 * Per-request view of one afpd session process, through netatalk's own
 * DTrace provider (built when meson's with-dtrace option is on, the default).
 *
 * usage: dtrace -q -s afpd_requests.d -p <afpd session pid> -o requests.txt
 *        stop with ^C or SIGTERM; the summary prints on exit
 *
 * Lines "REQ <since start> <command> <in afpd> <on CPU> <asleep> <runnable>",
 * times in usec, give the timeline; a request of 50 ms or more adds a "SLOW"
 * line with its longest sleep. The summary gives, per AFP command, its count
 * and times, the CNID calls it made, why afpd left the CPU (the kernel's wait
 * message or the lock it blocked on), the stacks of sleeps of 1 ms or more,
 * and where it ran, from samples at 997 Hz.
 *
 * A probe on every system call would add its own cost to each request in
 * step with the calls it makes, so the system calls come from
 * afpd_syscalls.d, in a run of their own (collect.sh -s). The scheduler
 * probes act only when afpd sleeps and wakes.
 */

#pragma D option quiet
#pragma D option dynvarsize=32m
#pragma D option bufsize=16m
#pragma D option aggsize=16m
#pragma D option stackframes=32

inline int SLOW_NS = 50000000;
inline int STACK_NS = 1000000;

BEGIN
{
	t_begin = timestamp;
}

afp$target:::afpfunc-start
{
	self->name = copyinstr(arg1);
	self->t0 = timestamp;
	self->v0 = vtimestamp;
	self->slept = 0;
	self->waited = 0;
	self->sleeps = 0;
	self->longest = 0;
	self->longwhy = "-";
	self->longcnid = "-";
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

/*
 * A thread leaves the CPU with a wait message when it sleeps, with a lock
 * name when it blocks on a kernel lock, and with neither when it is
 * preempted and stays runnable.
 */
sched:::off-cpu
/pid == $target && self->t0/
{
	self->off = timestamp;
	self->why = curthread->td_wmesg != NULL ?
	    stringof(curthread->td_wmesg) :
	    curthread->td_lockname != NULL ?
	    stringof(curthread->td_lockname) :
	    (curthread->td_inhibitors & 0x1) != 0 ? "suspended" : "runnable";
}

/* a woken thread joins a run queue; a preempted one rejoins it unreported */
sched:::enqueue
/args[1]->p_pid == $target/
{
	woken[(int)args[0]->td_tid] = timestamp;
}

sched:::on-cpu
/pid == $target && self->off/
{
	this->away = timestamp - self->off;
	this->waited = self->why == "runnable" ? this->away :
	    woken[(int)tid] > self->off ? timestamp - woken[(int)tid] : 0;
	this->slept = this->away - this->waited;
	this->cnid = self->c0 ? self->cnid : "-";
	self->slept += this->slept;
	self->waited += this->waited;
	self->sleeps += this->slept > 0;
	@offn[self->name, this->cnid, self->why] = count();
	@offs[self->name, this->cnid, self->why] = sum(this->slept);
	@offw[self->name, this->cnid, self->why] = sum(this->waited);
}

sched:::on-cpu
/pid == $target && self->off && this->slept > self->longest/
{
	self->longest = this->slept;
	self->longwhy = self->why;
	self->longcnid = this->cnid;
}

sched:::on-cpu
/pid == $target && self->off && this->slept >= STACK_NS/
{
	@sleepstk[self->name, self->why, stack(), ustack()] = sum(this->slept);
}

sched:::on-cpu
/pid == $target && self->off/
{
	woken[(int)tid] = 0;
	self->off = 0;
}

profile-997
/pid == $target && self->t0 && arg1/
{
	@ucpu[self->name, ufunc(arg1)] = count();
}

profile-997
/pid == $target && self->t0 && arg0/
{
	@kcpu[self->name, stack(10)] = count();
}

afp$target:::afpfunc-done
/self->t0/
{
	this->wall = timestamp - self->t0;
	this->run = vtimestamp - self->v0;
	printf("REQ %d %s %d %d %d %d\n", (timestamp - t_begin) / 1000,
	    self->name, this->wall / 1000, this->run / 1000,
	    self->slept / 1000, self->waited / 1000);
	@n[self->name] = count();
	@ns[self->name] = sum(this->wall);
	@oncpu[self->name] = sum(this->run);
	@asleep[self->name] = sum(self->slept);
	@runnable[self->name] = sum(self->waited);
	@lat[self->name] = quantize(this->wall / 1000);
}

afp$target:::afpfunc-done
/self->t0 && this->wall >= SLOW_NS/
{
	printf("SLOW %d %s %d us: %d on CPU, %d asleep in %d sleeps, ",
	    (timestamp - t_begin) / 1000, self->name, this->wall / 1000,
	    this->run / 1000, self->slept / 1000, self->sleeps);
	printf("%d runnable; longest sleep %d us on %s, CNID call %s\n",
	    self->waited / 1000, self->longest / 1000, self->longwhy,
	    self->longcnid);
}

afp$target:::afpfunc-done
/self->t0/
{
	self->t0 = 0;
}

END
{
	printf("\n== AFP commands: count, total ns\n");
	printa("CMD %-24s %@10d %@16d\n", @n, @ns);
	printf("\n== their time in afpd: on CPU, asleep, runnable, ns\n");
	printa("TIME %-24s %@16d %@16d %@16d\n", @oncpu, @asleep, @runnable);
	printf("\n== CNID calls per AFP command: count, total ns\n");
	printa("CNID %-24s %-24s %@10d %@16d\n", @cnid, @cnidns);
	printf("\n== off CPU per AFP command, CNID call and reason: ");
	printf("count, ns asleep, ns runnable\n");
	printa("OFF %-24s %-16s %-24s %@10d %@16d %@16d\n", @offn, @offs,
	    @offw);
	printf("\n== sleeps of 1 ms or more per AFP command and reason, ");
	printf("by kernel and user stack: ns\n");
	trunc(@sleepstk, 12);
	printa(@sleepstk);
	printf("\n== on CPU in requests, samples at 997 Hz: user functions\n");
	trunc(@ucpu, 20);
	printa(@ucpu);
	printf("\n== on CPU in requests, samples at 997 Hz: kernel stacks\n");
	trunc(@kcpu, 12);
	printa(@kcpu);
	printf("\n== AFP command latency in afpd, usec\n");
	printa(@lat);
}
