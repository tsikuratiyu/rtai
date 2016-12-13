#include <linux/kernel.h>   /* decls needed for kernel modules */
#include <linux/module.h>   /* decls needed for kernel modules */
#include <linux/version.h>  /* LINUX_VERSION_CODE, KERNEL_VERSION() */
#include <linux/errno.h>    /* EINVAL, ENOMEM */

/*
  Specific header files for RTAI, our flavor of RT Linux
 */
#include "rtai.h"       /* RTAI configuration switches */
#include "rtai_sched.h"     /* rt_set_periodic_mode(), start_rt_timer(),
                   nano2count(), RT_SCHED_LOWEST_PRIORITY,
                   rt_task_init(), rt_task_make_periodic() */
#include <rtai_sem.h>

MODULE_LICENSE("GPL");

static RT_TASK print_task;  /* we'll fill this in with our task */

void print_function(long arg)     /* Subroutine to be spawned */
{
   rt_printk("Hello world!\n"); /* Print task Id */
   return;
}

int init_module(void)
{
    int retval;         /* we look at our return values */

    /*
      Set up the timer to expire in one-shot mode by calling

      void rt_set_oneshot_mode(void);

      This sets the one-shot mode for the timer. The 8254 timer chip
      will be reprogrammed each task cycle, at a cost of about 2 microseconds.
    */
    rt_set_oneshot_mode();

    /*
      Start the one-shot timer by calling

      RTIME start_rt_timer(RTIME count)

      with 'count' set to 1 as a dummy nonzero value provided for the period since
      we don't have one. We habitually use a nonzero value since 0
      could mean disable the timer to some.
    */
    start_rt_timer(1);

    /*
      Create the RT task with rt_task_init(...)
     */
    retval = 
    rt_task_init(&print_task,   /* pointer to our task structure */
       print_function, /* the actual  function */
       0,       /* initial task parameter; we ignore */
       1024,        /* 1-K stack is enough for us */
       RT_SCHED_LOWEST_PRIORITY, /* lowest is fine for our 1 task */
       0,       /* uses floating point; we don't */
       0);      /* signal handler; we don't use signals */
    if (0 != retval) {
        if (-EINVAL == retval) {
            /* task structure is already in use */
            printk("task: task structure is invalid\n");
        } else {
            /* unknown error */
            printk("task: error starting task\n");
        }
        return retval;
    }

    /*
      Start the RT task with rt_task_resume()
     */
    retval = rt_task_resume(&print_task); /* pointer to our task structure */
    if (0 != retval) {
        if (-EINVAL == retval) {
            /* task structure is already in use */
            printk("task: task structure is invalid\n");
        } else {
            /* unknown error */
            printk("task: error starting task\n");
        }
        return retval;
    }

    return 0;
}

void cleanup_module(void)
{
  // task end themselves -> not necessary to delete them
  return;
}
