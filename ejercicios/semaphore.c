/*
Un semaforo binario puede ser declarado como sigue

	static SEM semBinary;
	rt_typed_sem_init(&semBinary, 1, BIN_SEM | FIFO_Q );

Es tomado por la siguiente sentencia
	rt_sem_wait(&semBinary);

Un semáforo binario puede ser visto como una bandera que está disponible o no disponible . Cuando una tarea tarda un semáforo binario , usando rt_sem_wait (), el resultado depende de si el semáforo está disponible o no disponible en el momento de la llamada. Si el semáforo está disponible, entonces el semáforo deja de estar disponible y entonces la tarea continúa ejecutándose inmediatamente. Si el semáforo no está disponible , la tarea se pone en una cola de tareas bloqueados y entra en un estado de espera de la disponibilidad del semáforo . Cuando una tarea da un semáforo binario , usando rt_sem_signal () El resultado depende de si el semáforo está disponible o no disponible en el momento de la call.If el semáforo ya está disponible , dando el semáforo tiene ningún efecto en absoluto. Si el semáforo no está disponible y no es una tarea está esperando a tomar , entonces el semáforo esté disponible. En caso el semáforo no está disponible y una o más tareas están pendientes de su disponibilidad, a continuación, la primera tarea en la cola de tareas pendientes se desbloquea y el semáforo se deja disponible.
*/

/*
   file global.c
   update global variable by two tasks
*/

/* includes */
#include <linux/kernel.h>	/*  kernel modules */
#include <linux/module.h>	/* kernel modules */
#include <linux/version.h>	/* LINUX_VERSION_CODE, KERNEL_VERSION() */
#include <linux/errno.h>	/* EINVAL, ENOMEM */

/*
  Specific header files for RTAI, our flavor of RT Linux
 */
#include "rtai.h"		/* RTAI configuration switches */
#include "rtai_sched.h"	/* RTAI scheduling */
#include <rtai_sem.h>   /* RTAI semaphores */

MODULE_LICENSE("GPL");

/* globals */
#define ITER 10

static RT_TASK  t1;
static RT_TASK  t2;
/* function prototypes */
void taskOne(long arg);
void taskTwo(long arg);

int global = 0;

void tasks(void)
{
    int retval;

    /* init the two tasks */
    retval = rt_task_init(&t1,taskOne, 0, 1024, 0, 0, 0);
    retval = rt_task_init(&t2,taskTwo, 0, 1024, 0, 0, 0);

    /* start the two tasks */
    retval = rt_task_resume(&t1);
    retval = rt_task_resume(&t2);
}


void taskOne(long arg)
{
    int i;
    for (i=0; i < ITER; i++)
    {
        rt_printk("I am taskOne and global = %d................\n", ++global);
    }
}

void taskTwo(long arg)
{
    int i;
    for (i=0; i < ITER; i++)
    {
        rt_printk("I am taskTwo and global = %d----------------\n", --global);
    }
}



int init_module(void)
{
    printk("start of init_module\n");

    rt_set_oneshot_mode();
    start_rt_timer(1);

    tasks();

    printk("end of init_module\n");
    return 0;
}


void cleanup_module(void)
{
    // task end themselves -> not necessary to delete them
    return;
}

