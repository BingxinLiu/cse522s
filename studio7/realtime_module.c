#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <uapi/linux/sched/types.h>

static struct task_struct * kthread = NULL;
static struct task_struct * kthread1 = NULL;
static struct task_struct * kthread2 = NULL;

// parameter, default == 1 second
static ulong period_sec = 1;
module_param(period_sec, ulong, 0644);
static ulong period_nsec = 0;
module_param(period_nsec, ulong, 0644);

static unsigned int data = 4500000;
static unsigned int data1 = 18000000;
static unsigned int data2 = 36000000;

static struct hrtimer ahrtimer;
static struct ktime_t kt;

static unsigned int rate = 0;

static enum hrtimer_restart
timer_function(struct hrtimer * ahrtimer)
{
    wake_up_process(kthread);
    if (rate % 2 == 0) wake_up_process(kthread1);  
    if (rate % 4 == 0) wake_up_process(kthread2);
    ++rate;
    hrtimer_forward_now(ahrtimer, kt);
    return HRTIMER_RESTART;
}


static int
thread_fn(void * data)
{
    unsigned int i;
    unsigned int iter;
    // iter = (unsigned int *) data;

    printk("kernel thread %s created.\n", current->comm);
    // printk("%p\n", data);
    iter = *((unsigned int *) data);
    // printk("%u\n", iter);

    while (!kthread_should_stop()) {
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
        printk("The kthread %s has just woken up\n", current->comm);
        for (i = 0; i < iter; ++i)
        {
            ktime_get();
        }
    }

    return 0;
}

static int
realtime_init(void)
{
    struct sched_param sp, sp1, sp2;

    sp.sched_priority = 60;
    sp1.sched_priority = 50;
    sp2.sched_priority = 40;

    printk(KERN_INFO "Loaded realtime module\n");
    // printk("%u, %u\n", data, data1);

    kt = ktime_set(period_sec, period_nsec);
    hrtimer_init(&ahrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    ahrtimer.function = timer_function;

    // printk("%p, %p\n", &data, &data1);
    kthread = kthread_create(thread_fn, &data, "realtime");
    kthread_bind(kthread, 3);
    sched_setscheduler(kthread, SCHED_FIFO, &sp);
    
    if (IS_ERR(kthread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(kthread);
    }
    wake_up_process(kthread);

    kthread1 = kthread_create(thread_fn, &data1, "realtime1");
    //printk("%u, %u\n", data, data1);
    // bind kthread with cpu[0];
    kthread_bind(kthread1, 3);
    sched_setscheduler(kthread1, SCHED_FIFO, &sp1);
    if (IS_ERR(kthread1)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(kthread1);
    }
    wake_up_process(kthread1);

    kthread2 = kthread_create(thread_fn, &data2, "realtime2");
    //printk("%u, %u\n", data, data1);
    // bind kthread with cpu[0];
    kthread_bind(kthread2, 3);
    sched_setscheduler(kthread2, SCHED_FIFO, &sp2);
    if (IS_ERR(kthread2)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(kthread2);
    }
    wake_up_process(kthread2);

    hrtimer_start(&ahrtimer, kt, HRTIMER_MODE_REL);

    return 0;
}

static void 
realtime_exit(void)
{
    hrtimer_cancel(&ahrtimer);
    kthread_stop(kthread);
    kthread_stop(kthread1);
    kthread_stop(kthread2);
    printk(KERN_INFO "Unloaded realtime module\n");
}

module_init(realtime_init);
module_exit(realtime_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Bingxin Liu");
MODULE_DESCRIPTION ("cse522s studio7");