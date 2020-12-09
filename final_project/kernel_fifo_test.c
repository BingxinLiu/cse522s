#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>

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

/* fifo size in elements (bytes) */
#define FIFO_SIZE 1024

#define BUF_SIZE 128

// name of the proc entry
#define PROC_FIFO "DTS-fifo"

/* lock for procfs read access */
static DEFINE_MUTEX(read_lock);

/* lock for procfs write access */
static DEFINE_MUTEX(write_lock);

static DECLARE_KFIFO(fifo, unsigned char, FIFO_SIZE);

static struct task_struct * read_thread = NULL;
static struct task_struct * write_thread = NULL;

static int
read_thread_fn(void * data)
{
	unsigned char buf[BUF_SIZE]; 
	unsigned int read_length;

    printk("Hello from read_thread %s.\n", current->comm);

    while (!kthread_should_stop()) {

		read_length = kfifo_out(&fifo, buf, BUF_SIZE);
		if (read_length > 0)
		{
			printk(KERN_INFO "buf: %.*s\n", read_length, buf);
		}

        schedule();
    }

    return 0;
}

static int
write_thread_fn(void * data)
{
	struct file * f;
	ssize_t write_length;

    printk("Hello from write_thread %s.\n", current->comm);

	

	f = filp_open("/proc/DTS-fifo", O_WRONLY | O_APPEND, 0);
	write_length = kernel_write(f, "write into fifo", 15, NULL);
	if (write_length > 0)
	{
		printk(KERN_INFO "buf write: %d\n", write_length);
	}

    while (!kthread_should_stop()) {
        schedule();
    }

	filp_close(f, NULL);

    return 0;
}

static ssize_t fifo_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	int ret;
	unsigned int copied;

	if (mutex_lock_interruptible(&write_lock))
		return -ERESTARTSYS;

	ret = kfifo_from_user(&fifo, buf, count, &copied);

	mutex_unlock(&write_lock);

	return ret ? ret : copied;
}

static ssize_t fifo_read(struct file *file, char __user *buf,
						size_t count, loff_t *ppos)
{
	int ret;
	unsigned int copied;

	if (mutex_lock_interruptible(&read_lock))
		return -ERESTARTSYS;

	ret = kfifo_to_user(&fifo, buf, count, &copied);

	mutex_unlock(&read_lock);

	return ret ? ret : copied;
}

static const struct file_operations fifo_fops = {
	.owner		= THIS_MODULE,
	.read		= fifo_read,
	.write		= fifo_write,
	.llseek		= noop_llseek,
};

/* init function - logs that initialization happened, returns success */
static int 
kernel_fifo_test_init(void)
{
	unsigned int i;
    printk(KERN_ALERT "kernel fifo module initialized\n");

    INIT_KFIFO(fifo);
    if (proc_create(PROC_FIFO, 0, NULL, &fifo_fops) == NULL)
    {
        return -ENOMEM;
    }

	read_thread = kthread_create(read_thread_fn, NULL, "read_thread");
    if (IS_ERR(read_thread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(read_thread);
    }
    
    wake_up_process(read_thread);

	write_thread = kthread_create(write_thread_fn, NULL, "k_memory");
    if (IS_ERR(write_thread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(write_thread);
    }

	for (i=0; i <= 10000000; ++i)
	{
		ktime_get();
	}
    
    wake_up_process(write_thread);

    

    return 0;
}

/* exit function - logs that the module is being removed */
static void 
kernel_fifo_test_exit(void)
{
    printk(KERN_ALERT "kernel fifo module is being unloaded\n");
    remove_proc_entry(PROC_FIFO, NULL);
	kthread_stop(write_thread);
	kthread_stop(read_thread);
}

module_init(kernel_fifo_test_init);
module_exit(kernel_fifo_test_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("B. Liu & H. Huang");
MODULE_DESCRIPTION ("a test module for kernel fifo");