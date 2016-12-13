#include <linux/module.h>
#include <linux/version.h>
#include <linux/errno.h>
#include "rtai.h"
#include "rtai_sched.h"
#include <rtai_sem.h>
MODULE_LICENSE("GPL");
static RT_TASK print_task;
/* EINVAL, ENOMEM */
void print_function(long arg){
rt_printk("Hello world!\n");
return;
}
int init_module(void){
int retval;
rt_set_oneshot_mode(); start_rt_timer(1); // the execution starts
retval = rt_task_init(&print_task, print_function, 0, 1024, RT_SCHED_LOWEST_PRIORITY,
0, 0);
if(retval != 0){
if(-EINVAL == retval)
printk("task: task structure is invalid\n");
else
printk("task: error starting task\n");
return retval;
}
retval = rt_task_resume(&print_task);
/* error checking here */
return 0;
}
void cleanup_module(void){
printk("closing... bye\n");
return;
}

