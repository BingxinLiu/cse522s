#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/mm.h>

#include <asm/uaccess.h>
#include <linux/string.h>

#define FILE_PATH "/tmp/dts_fs/test"

static struct task_struct * kthread = NULL;

static int
thread_fn(void * data)
{




    while (!kthread_should_stop()) {
        schedule();
    }

    return 0;
}

static int
file_sync_module_init(void)
{
    printk(KERN_INFO "Loaded file_sync module\n");

    kthread = kthread_create(thread_fn, NULL, "file_sync");
    if (IS_ERR(kthread)) {
        printk(KERN_ERR "Failed to create file_sync kernel thread\n");
        return PTR_ERR(kthread);
    }
    
    wake_up_process(kthread);

    return 0;
}

static void 
file_sync_module_exit(void)
{
    kthread_stop(kthread);
    printk(KERN_INFO "Unloaded file_sync module\n");
}

module_init(file_sync_module_init);
module_exit(file_sync_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR ("B. Liu & H. Huang");
MODULE_DESCRIPTION ("a test module for kernel file sync\n");