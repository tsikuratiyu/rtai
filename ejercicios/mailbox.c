/*
  Example Mailbox
*/

/* includes */
#include <linux/kernel.h>       /* decls needed for kernel modules */
#include <linux/module.h>       /* decls needed for kernel modules */
#include <linux/version.h>      /* LINUX_VERSION_CODE, KERNEL_VERSION() */
#include <linux/errno.h>        /* EINVAL, ENOMEM */

/*
  Specific header files for RTAI, our flavor of RT Linux
 */

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_mbx.h>

MODULE_LICENSE("GPL");

static RT_TASK  t1;
static RT_TASK  t2;
/* function prototypes */
void taskOne(long arg);
void taskTwo(long arg);


/* defines */
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 50

/* globals */
static MBX mailboxId;

void message(void) /* function to create the message queue and two tasks */
{
    int retval;

    /* create FIFO mailbox */
    retval = rt_typed_mbx_init (&mailboxId, MAX_MESSAGES,  FIFO_Q);
    if (0 != retval) {
        if (-ENOMEM == retval) {
          printk("ENOMEM: Space could not be allocated for the mailbox buffer.");
        } else {
          /* unknown error */
          printk("Unknown error creating message queue\n");
        }
    }

    /* spawn the two tasks that will use the mailbox */

    /* init the two tasks */
    retval = rt_task_init(&t1,taskOne, 0, 1024, 0, 0, 0);
    retval = rt_task_init(&t2,taskTwo, 0, 1024, 0, 0, 0);

    /* start the two tasks */
    retval = rt_task_resume(&t1);
    retval = rt_task_resume(&t2);

}

void taskOne(long arg) /* task that writes to the mailbox */
{
    int retval;
    char message[] = "Received message from taskOne";

    /* send message */
    retval = rt_mbx_send(&mailboxId, message, sizeof(message));
    if (0 != retval) {
        if (-EINVAL == retval) {
          rt_printk("mailbox is invalid\n");
        } else {
          /* unknown error */
          rt_printk("Unknown mailbox error\n");
        }
    } else
    {
        rt_printk("taskOne sent message to mailbox\n");}

}

void taskTwo(long arg) /* tasks that reads from the mailbox */
{
    int retval;
    char msgBuf[MAX_MESSAGE_LENGTH];

    /* receive message */
    retval = rt_mbx_receive_wp(&mailboxId, msgBuf, 50);
    if (-EINVAL == retval) {
          rt_printk("mailbox is invalid\n");
    } else {
        rt_printk("taskTwo received message : %s\n",msgBuf);
        /* retval gives number of not received bytes from 50 */
        rt_printk("with length %d\n",50-retval);
    }


    /* delete mailbox */
    rt_mbx_delete(&mailboxId);
}

int init_module(void)
{

    printk("start of init_module\n");

    rt_set_oneshot_mode();
    start_rt_timer(1);

    message();

    printk("end of init_module\n");
    return 0;
}

void cleanup_module(void)
{
     stop_rt_timer();
     rt_task_delete(&t1);
     rt_task_delete(&t2);

    return;
}

