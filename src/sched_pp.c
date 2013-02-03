/*
 * Name        : sched_pp.c
 * Author      : Matthias Behr
 * Version     :
 * Copyright   : Copyright (c) 2013, Matthias Behr, mbehr@mcbehr.de
 * Description : cmd line interface for sched_set/getscheduler
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

void print_params(pid_t pid, int *cur_policy)
{
	int ret, policy, pmin, pmax;
	struct sched_param sched_p;
	errno=0;
	printf("\nCurrent scheduling parameters for process %d:\n", pid);
	policy=sched_getscheduler(pid);
	if (errno){
		fprintf(stderr, "sched_getscheduler return errno %d (%s)\n", errno, strerror(errno));
	}else{
		if (cur_policy)
			*cur_policy = policy;

		printf("sched_getscheduler returned %d: ", policy);
		switch (policy){
		case SCHED_OTHER:
			printf("SCHED_OTHER\n");
			break;
			/*			case SCHED_BATCH:
			printf("SCHED_BATCH\n");
			break;
		case SCHED_IDLE:
			printf("SCHED_IDLE\n");
			break; */
		case SCHED_FIFO:
			printf("SCHED_FIFO\n");
			break;
		case SCHED_RR:
			printf("SCHED_RR\n");
			break;
		default:
			printf(" unknown policy (%d)!\n", policy);
			break;
		}
	}
	errno=0;
	pmin = sched_get_priority_min(policy);
	if (errno){
		fprintf(stderr, "sched_get_priority_min returned errno %d (%s)\n", errno, strerror(errno));
	}else{
		printf("priority min=%d\n", pmin);
	}
	errno=0;
	pmax = sched_get_priority_max(policy);
	if (errno){
		fprintf(stderr, "sched_get_priority_max returned errno %d (%s)\n", errno, strerror(errno));
	}else{
		printf("priority max=%d\n", pmax);
	}

	if (policy==SCHED_OTHER){
		int priority;
		errno=0;
		priority = getpriority(PRIO_PROCESS, pid);
		if (errno){
			fprintf(stderr, "getpriority returned errno %d (%s)\n", errno ,strerror(errno));
		}else{
			printf("getpriority: priority=%d\n", priority);
		}
	}else{
		ret = sched_getparam(pid, &sched_p);
		if (!ret){
			printf("sched_getparam: sched_priority=%d\n", sched_p.__sched_priority);
		}else{
			fprintf(stderr, "sched_getparam returned errno %d (%s)\n", errno ,strerror(errno));
		}
	}
	printf("\n");
}

int set_policy(pid_t pid, int policy, int priority)
{
	int ret;
	struct sched_param sched_p;
	sched_p.__sched_priority = priority;
	ret = sched_setscheduler(pid, policy, &sched_p);
	if (ret)
		fprintf(stderr, "sched_setscheduler returned %d errno=%d (%s).\n", ret, errno, strerror(errno));
	return ret;
}

int set_priority(pid_t pid, int policy, int priority)
{
	int ret;
	if (policy != SCHED_OTHER){
		struct sched_param sched_p;
		sched_p.__sched_priority = priority;
		ret = sched_setparam(pid, &sched_p);
		if (ret)
			fprintf(stderr, "sched_setparam returned %d errno=%d (%s).\n", ret, errno, strerror(errno));
	}else{
		/* for SCHED_OTHER we just change the "nice" value: */
		ret = setpriority(PRIO_PROCESS, pid, priority);
		if (ret)
			fprintf(stderr, "setpriority returned %d errno=%d (%s).\n", ret, errno, strerror(errno));

	}
	return -1;
}

int main (int argc, char **argv)
{
	int do_get = 0;
	int do_set = 0;
	char *pvalue = NULL;
	pid_t pid=0;
	int priority=0;
	int policy = -1;
	int index;
	int c;

	opterr = 0;

	while ((c = getopt (argc, argv, "gs:p:P:")) != -1)
		switch (c)
		{
		case 'g':
			do_get = 1;
			break;
		case 's':
			do_set = 1;
			priority = atol(optarg);
			break;
		case 'p':
			pvalue = optarg;
			pid=atol(pvalue);
			break;
		case 'P':
			if (!strcmp(optarg, "SCHED_FIFO")) policy = SCHED_FIFO;
			else if (!strcmp(optarg, "SCHED_RR")) policy = SCHED_RR;
			else if (!strcmp(optarg, "SCHED_OTHER")) policy = SCHED_OTHER;
			else{
				fprintf(stderr, "Unknown policy <%s>.\n", optarg);
				return 2;
			}
			break;
		case '?':
			if (optopt == 'p' || optopt == 's' || optopt == 'P')
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf (stderr,
						"Unknown option character `\\x%x'.\n",
						optopt);
			return 1;
		default:
			abort ();
		}

	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);

	if (do_get){
		print_params(pid, 0);
	}

	if (do_set){
		int cur_policy;
		/* print before */
		print_params(pid, &cur_policy);

		if (policy != -1){
			printf("trying to change pid %d to policy %d and priority %d:\n", pid, policy, priority);
			if (policy != SCHED_OTHER)
				set_policy(pid, policy, priority);
			else{
				set_policy(pid, policy, 0); /* SCHED_OTHER only allows 0 for that priority */
				set_priority(pid, policy, priority);
			}

		}else{
			printf("trying to change pid %d to priority %d:\n Params now:\n", pid, priority);
			set_priority(pid, cur_policy, priority);
		}
		/* print afterwards */
		print_params(pid, 0);
	}

	return 0;
}

