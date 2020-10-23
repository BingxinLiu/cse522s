/* simple_module.c - a simple template for a loadable kernel module in Linux,
   based on the hello world kernel module example on pages 338-339 of Robert
   Love's "Linux Kernel Development, Third Edition."
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/* init function - logs that initialization happened, returns success */
static int 
simple_init(void)
{
    extern unsigned long volatile jiffies;
    printk(KERN_ALERT "simple module initialized, and the current jiffies number is %lu\n", jiffies);

    return -12;
}

/* exit function - logs that the module is being removed */
static void 
simple_exit(void)
{
    extern unsigned long volatile jiffies;
    printk(KERN_ALERT "simple module is being unloaded, and the current jiffies number is %lu\n", jiffies);
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Bingxin Liu");
MODULE_DESCRIPTION ("cse522s studio3_6");
