/* 

This example features NTASKS realtime tasks running periodically in EDF mode.
The task are given a priority ordered in such a way that low numbered tasks
have the lowest priority. However the tasks execute for a duration proportional 
to their number so that, under EDF, the lowest priority tasks run first.
So if they appear increasingly ordered on the screen EDF should be working.

*/

#include <linux/module.h>

#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>

#define ONE_SHOT

#define TICK_PERIOD 10000000

#define STACK_SIZE 2000

#define LOOPS 3
//#define LOOPS 1000000000

#define NTASKS 8

static RT_TASK thread[NTASKS];

static RTIME tick_period;

static int cpu_used[NR_RT_CPUS];

static void fun(long t)
{
	unsigned int loops = LOOPS;
	while(loops--) {
		cpu_used[hard_cpu_id()]++;
		rt_printk("TASK %d with priority %d in loop %d \n", t, thread[t].priority,loops);
		rt_task_set_resume_end_times(-NTASKS*tick_period, -(t + 1)*tick_period);
	}
        rt_printk("TASK %d with priority %d  ENDS\n", t, thread[t].priority);
}


int init_module(void)
{
	RTIME now;
	int i;

#ifdef ONE_SHOT
	rt_set_oneshot_mode();
#endif
	for (i = 0; i < NTASKS; i++) {
		rt_task_init(&thread[i], fun, i, STACK_SIZE, NTASKS - i - 1, 0, 0);
	}
	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	now = rt_get_time() + NTASKS*tick_period;
	for (i = 0; i < NTASKS; i++) {
		rt_task_make_periodic(&thread[NTASKS - i - 1], now, NTASKS*tick_period);
	}
	return 0;
}


void cleanup_module(void)
{
	int i, cpuid;

	stop_rt_timer();
	for (i = 0; i < NTASKS; i++) {
		rt_task_delete(&thread[i]);
	}
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
