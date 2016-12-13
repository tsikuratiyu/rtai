#include <linux/kernel.h>
#include <linux/module.h>
#include <rtai.h>
MODULE_LICENSE("GPL");
int init_module(void) // entry point
{
printk("Hello world!\n");
return 0;
}
void cleanup_module(void) // exit point
{
printk("Goodbye world!\n");
return;
}
