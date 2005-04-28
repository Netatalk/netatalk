/*
 * $Id: timeout.c,v 1.5 2005-04-28 20:49:36 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

char **av;

struct slist {
	char *name;
	int num;
} sigs[] = {
#ifdef SIGHUP
	{ "HUP",	SIGHUP },
#endif
#ifdef SIGINT
	{ "INT",	SIGINT },
#endif
#ifdef SIGQUIT
	{ "QUIT",	SIGQUIT },
#endif
#ifdef SIGILL
	{ "ILL",	SIGILL },
#endif
#ifdef SIGTRAP
	{ "TRAP",	SIGTRAP },
#endif
#ifdef SIGABRT
	{ "ABRT",	SIGABRT },
#endif
#ifdef SIGIOT
	{ "IOT",	SIGIOT },
#endif
#ifdef SIGEMT
	{ "EMT",	SIGEMT },
#endif
#ifdef SIGFPE
	{ "FPE",	SIGFPE },
#endif
#ifdef SIGKILL
	{ "KILL",	SIGKILL },
#endif
#ifdef SIGBUS
	{ "BUS",	SIGBUS },
#endif
#ifdef SIGSEGV
	{ "SEGV",	SIGSEGV },
#endif
#ifdef SIGSYS
	{ "SYS",	SIGSYS },
#endif
#ifdef SIGPIPE
	{ "PIPE",	SIGPIPE },
#endif
#ifdef SIGALRM
	{ "ALRM",	SIGALRM },
#endif
#ifdef SIGTERM
	{ "TERM",	SIGTERM },
#endif
#ifdef SIGURG
	{ "URG",	SIGURG },
#endif
#ifdef SIGSTOP
	{ "STOP",	SIGSTOP },
#endif
#ifdef SIGTSTP
	{ "TSTP",	SIGTSTP },
#endif
#ifdef SIGCONT
	{ "CONT",	SIGCONT },
#endif
#ifdef SIGCHLD
	{ "CHLD",	SIGCHLD },
#endif
#ifdef SIGCLD
	{ "CLD",	SIGCLD },
#endif
#ifdef SIGTTIN
	{ "TTIN",	SIGTTIN },
#endif
#ifdef SIGTTOU
	{ "TTOU",	SIGTTOU },
#endif
#ifdef SIGIO
	{ "IO",		SIGIO },
#endif
#ifdef SIGXCPU
	{ "XCPU",	SIGXCPU },
#endif
#ifdef SIGXFSZ
	{ "XFSZ",	SIGXFSZ },
#endif
#ifdef SIGVTALRM
	{ "VTALRM",	SIGVTALRM },
#endif
#ifdef SIGPROF
	{ "PROF",	SIGPROF },
#endif
#ifdef SIGWINCH
	{ "WINCH",	SIGWINCH },
#endif
#ifdef SIGINFO
	{ "INFO",	SIGINFO },
#endif
#ifdef SIGUSR1
	{ "USR1",	SIGUSR1 },
#endif
#ifdef SIGUSR2
	{ "USR2",	SIGUSR2 },
#endif
#ifdef SIGPWR
	{ "PWR",	SIGPWR },
#endif
	{ 0,		0 }
};

void
usage()
{
	int i;

	fprintf (stderr, "Usage: %s [-s signal] seconds program [args]\n\n",
		 *av);
	fprintf (stderr, "You can use a numerical signal, or one of these:\n");

#define COLS 8

	for (i = 0; sigs[i].name; i++) {
		if (i % COLS == 0)
			fprintf (stderr, "\n\t");

		fprintf (stderr, "%s", sigs[i].name);

		if ((i + 1) % COLS != 0)
			fprintf (stderr, "\t");
	}

	fprintf (stderr, "\n\n");
}

int
main (argc, argv)
	int argc;
	char **argv;
{
	int i;
	int whatsig = SIGTERM;
	extern char *optarg;
	extern int optind;
	int pid;

	av = argv;

	while ((i = getopt (argc, argv, "+s:")) != -1) {

		switch (i) {
		case 's':
			if (isdigit (*optarg)) {
				whatsig = atoi (optarg);
			} else {
				for (i = 0; sigs[i].name; i++) {
					if (!strcmp (sigs[i].name, optarg)) {
						whatsig = sigs[i].num;
						break;
					}
				}

				if (!sigs[i].name) {
					fprintf (stderr, 
						 "%s: Unknown signal %s\n",
						 *av, optarg);
					usage();
					exit (1);
				}
			}

			break;

		default:
			usage();
			exit (1);
		}
	}

	if (optind > argc - 2) {
		usage();
		exit (1);
	}

	if (!isdigit (*argv[optind])) {
		fprintf (stderr, "%s: \"seconds\" must be numeric, not %s\n",
			 *av, argv[optind]);
		usage();
		exit (1);
	}

	pid = fork();

	if (pid < 0) {
		fprintf (stderr, "%s: fork failed: ", *av);
		perror ("fork");
		exit (1);
	} else if (pid == 0) {
		int seconds = atoi (argv[optind]);

		pid = getppid();

		fclose (stdin);
		fclose (stdout);
		fclose (stderr);

		while (seconds-- > 0) {
			/*
			 * too bad there's no SIGPARENT so we have to keep
			 * polling to find out if it's still there
			 */

			if (kill (pid, 0) != 0)
				exit (0);

			sleep (1);
		}

		kill (pid, whatsig);
		exit (0);
	} else {
		execvp (argv[optind + 1], argv + optind + 1);

		fprintf (stderr, "%s: can't execute ", *av);
		perror (argv[optind + 1]);
		exit (1);
	}
}
