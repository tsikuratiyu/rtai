#include <linux/kernel.h>
/*
   Example, exercise 6 (to add round robin)
   Three tasks with equal priority
*/
#include <linux/kernel.h>
#include <linux/module.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_sem.h>

MODULE_LICENSE("GPL");

#define STACK_SIZE 2000

#define EXECTIME   400000000
/* proposed time-slice, not yet used here */
#define RR_QUANTUM  10000000

#define NTASKS 3
#define PRIORITY 100

/* globals */

static SEM sync;
static RT_TASK tasks[NTASKS];
static int switchesCount[NTASKS];

static void fun(long indx)
/* function executed by all tasks */
{
	RTIME starttime;
    RTIME endtime;
	rt_printk("Resume task #%d (%p) on CPU %d.\n", indx, &tasks[indx], hard_cpu_id());
	rt_sem_wait(&sync);
	rt_printk("Task #%d (%p) starts executing on CPU %d.\n", indx, &tasks[indx], hard_cpu_id());

    /* execute until EXECTIME time units have elapsed */
	starttime = rt_get_cpu_time_ns();
	while(rt_get_cpu_time_ns() < (starttime + EXECTIME));

    /* ready, check time and top signalling context switches */
    endtime = rt_get_cpu_time_ns()-starttime;
	tasks[indx].signal = 0;
	rt_printk("Task #%d (%p) terminates after %d.\n", indx, &tasks[indx],endtime);
}

static void signal(void)
/* signal handler, executed when a context switch occurs */
{
	RT_TASK *task;
	int i;
	for (i = 0; i < NTASKS; i++) {
		if ((task = rt_whoami()) == &tasks[i]) {
			switchesCount[i]++;
			rt_printk("Switch to task #%d (%p) on CPU %d.\n", i, task, hard_cpu_id());
			break;
		}
	}
}

int init_module(void)
{
	int i;

	printk("INSMOD on CPU %d.\n", hard_cpu_id());
	rt_sem_init(&sync, 0);

	rt_set_oneshot_mode();
	start_rt_timer(1);

	for (i = 0; i < NTASKS; i++) {
		rt_task_init(&tasks[i], fun, i, STACK_SIZE, PRIORITY, 0, signal);
	}
	for (i = 0; i < NTASKS; i++) {
		rt_task_resume(&tasks[i]);
	}
	rt_sem_broadcast(&sync);
	return 0;
}

void cleanup_module(void)
{
	int i;

	stop_rt_timer();
	rt_sem_delete(&sync);
	for (i = 0; i < NTASKS; i++) {
		printk("number of context switches task # %d -> %d\n", i, switchesCount[i]);
		rt_task_delete(&tasks[i]);
	}
}
