
/*
    messqueue.c
*/

/* ------------ headers ------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <linux/errno.h> /* error codes */
#include <signal.h>
#include <pthread.h>

#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include <rtai_sem.h>
#include <rtai_posix.h>

/* ------------ globals ------------------------------------------- */

/* turn on(1) or off(0) debugging */
const int DEBUG=1;

static int thread_watchdog;
RT_TASK *task_watchdog;
static int thread_main;
RT_TASK *task_main;


#define NTASKS 3
#define PRIORITY 100
#define STACK_SIZE 2000

#define EXECTIME    1e9 
#define RR_QUANTUM  1e7

// linux threads id's
static int thread[NTASKS];
// realtime task structures
static RT_TASK  *task[NTASKS];
static int taskarg[NTASKS];

// keep record of number of context switches of a task
static int contextswitches[NTASKS];

// semaphore used for synchronisation
static SEM *sem_sync;

// keep track running task
RT_TASK *runningtask;
static int countrunning=0;




// time after which when watchdog will make emergency stop 
static RTIME watchdog_sleep=2e9;

// Watchdog implements a hardstop after watchdog_sleep time
// IMPORTANT : only soft threads can be killed
static void *watchdog(void *arg)
{
    int i;
    task_watchdog = rt_task_init_schmod(nam2num("TASKWD"), 0, 0, 0, SCHED_FIFO, 0xF);
    mlockall(MCL_CURRENT | MCL_FUTURE);
    printf("STARTING WATCHDOG\n");
    rt_make_hard_real_time();

    // let watchdog sleep until it has to make a hard stop
    rt_sleep(nano2count(watchdog_sleep));
   
    // make a hardstop by killing all threads
    // IMPORTANT : only soft threads can be killed
    // reasons : 
    //      - phtread_kill_rt makes you go to soft realtime before
    //        calling the usermode function pthread_kill
    //      - rt_task_delete also goes to soft realtime and even doesn't
    //        work there, reason unknown
    //      - rt_task_suspend : doesn't work
	for (i = 0; i < NTASKS; i++) {
        pthread_kill_rt(thread[i],9);
	}
    pthread_kill_rt(thread_main,9);
    
    
    rt_printk("WATCHDOG KILLED TASKS\n");
    
    rt_make_soft_real_time();
    printf("=====> EMERGENCY STOP BY WATCHDOG\n");
    return 0;
}


//static void fun(long indx)
void fun(void *arg)
{
	RTIME starttime;
    int indx= *((int *) arg);
    
    /*  make this thread LXRT soft realtime */
    //  NOTE : under linux you cannot set the time quantum for round robin -> a
    //         default valus is taken
    task[indx] = rt_task_init_schmod(indx, 1, 0, 0, SCHED_RR, 0xF);
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // activate round robin and set quantum under the realtime schedular
    // quantum only applies in hard realtime !!!
    rt_set_sched_policy(task[indx], RT_SCHED_RR, RR_QUANTUM);
    
    // makes task hard real time (only when not developing/debugging)
    if ( !DEBUG )  rt_make_hard_real_time();

    // print intro
    rt_printk("RESUMED TASK #%d (%p).\n", indx, task[indx]);

    // wait to be started, main will never signal the semaphore before
    // this task is suspended on it because it has a lower priority 
    // (that is : the next two statements are atomic for main!)
    countrunning++;
	rt_sem_wait(sem_sync);
	rt_printk("TASK #%d (%p) STARTS WORKING RR.\n", indx, task[indx]);

    // do busy waiting till end of execution time
    starttime = rt_get_cpu_time_ns();
	while(rt_get_cpu_time_ns() < (starttime + EXECTIME)) {
        // check if this task is stored in runningtask variable
        if ( runningtask !=  task[indx] ) {
            contextswitches[indx]++;
            runningtask=task[indx];
            rt_printk("TASK #%d (%p).\n", indx, task[indx]);
        }    
    }
	
    // write your suicide letter
    countrunning--;
	rt_printk("TASK #%d (%p) SUICIDES.\n", indx, task[indx]);
}


void cleanup(void)
{
	int i;
	stop_rt_timer();
	for (i = 0; i < NTASKS; i++) {
		rt_printk("# %d -> %d\n", i, contextswitches[i]);
		rt_task_delete(task[i]);
	}
	rt_sem_delete(sem_sync);
}

int main(void)
{
	int i;
    printf("Start of main\n");
    
    // set realtime timer to run in oneshot mode    
    rt_set_oneshot_mode();
    // start realtime timer and scheduler
    start_rt_timer(1);
    // get main thread
    thread_main=pthread_self();
    
    /*  make this thread LXRT soft realtime */
    task_main = rt_task_init_schmod(nam2num("TASK0"), 100, 0, 0, SCHED_FIFO, 0xF);
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // Start watchdog 
    // NOTE : rt_sleep is necessary to get it really started, otherwise
    //        main will continue before the thread gets runtime. Because
    //        main is already FIFO scheduled with a higher priority than
    //        the newly started watchdog thread which at start
    //        temporarely is SCHED_OTHER
    //        Note: another solution instead of rt_sleep would be using
    //              semaphores
    thread_watchdog = rt_thread_create(watchdog, NULL, 10000);
    rt_sleep(nano2count(1e6));


	rt_printk("START.\n");
	
    /* create binary semaphore with semaphore available and queue tasks on FIFO basis */
    sem_sync=rt_typed_sem_init(nam2num("SYNC"), 0, BIN_SEM | FIFO_Q );

	for (i = 0; i < NTASKS; i++) {
        taskarg[i]=i;
        thread[i] = rt_thread_create(fun, &taskarg[i], 10000);
	}

    // wait until all tasks are running
	while(countrunning != NTASKS) {
	   rt_printk("countrunning %d\n", countrunning);
       rt_sleep(nano2count(1e6));
    }    
        
	rt_printk("2.\n");
	rt_sem_broadcast(sem_sync);
    
    // wait for end of program
	while(countrunning != 0) {
	   rt_printk("countrunning %d\n", countrunning);
       rt_sleep(nano2count(1e6));
    }    
    
    // cleanup
    cleanup();
    
    printf("Finished\n");    
	return 0;
}


