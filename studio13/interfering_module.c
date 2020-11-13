#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <uapi/linux/sched/types.h>

#define MS_TO_NS(x) (x * 1E6L)

static uint input_policy = 0;
module_param(input_policy, uint, 0644);

static uint priority = 50;
module_param(priority, uint, 0644);

static uint core = 1;
module_param(core, uint, 0644);

static ulong period = 100;
module_param(period, ulong, 0644);

static ulong period_sec = 1;
module_param(period_sec, ulong, 0644);
static ulong period_nsec = 0;
module_param(period_nsec, ulong, 0644);

static uint numIter = 100;
module_param(numIter, uint, 0644);

static struct task_struct * kthread = NULL;

static struct hrtimer ahrtimer;
static ktime_t kt;

static int
task_fn(void)
{
    ktime_t startTime, endTime;
    uint i;

    startTime = ktime_get();

    for (i = 0; i < numIter; ++i)
    {
        endTime = ktime_get();
    }

    printk("The startTime of currect task is: %lld, the running time is %lld, the # iteration is %d.\n", ktime_to_us(startTime), ktime_us_delta(endTime, startTime), i);
    return 0;
}

static enum hrtimer_restart
timer_function(struct hrtimer * ahrtimer)
{
    wake_up_process(kthread);
    hrtimer_forward_now(ahrtimer, kt);
    return HRTIMER_RESTART;
}

static int
thread_fn(void * data)
{
    printk("kernel thread %s created.\n", current->comm);
    while( !kthread_should_stop() )
    {
        if ( period != 0 )
        {
            set_current_state(TASK_INTERRUPTIBLE);
            schedule();
            printk("The kthread %s has just woken up\n", current->comm);
        }
        task_fn();
    }
    return 0;
}


/* init function - logs that initialization happened, returns success */
static int 
interfering_init(void)
{
    int policy;

    struct sched_param sp;
    sp.sched_priority = priority;

    printk(KERN_ALERT "interfering module initialized\n");

    kt = ktime_set(0, period * 1000000);
    hrtimer_init(&ahrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    ahrtimer.function = timer_function;

    if (input_policy == 0)
    {
        policy = SCHED_NORMAL;
        sp.sched_priority = 0;
    }
    else if (input_policy == 1)  policy = SCHED_FIFO;
    else if (input_policy == 2)  policy = SCHED_RR;
    else
    {
        printk("Error: the policy should be one of [OTHER, FIFO, RR].\n");
        return -1;
    }

    kthread = kthread_create(thread_fn, NULL, "interfering module");
    if (IS_ERR(kthread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(kthread);
    }

    sched_setscheduler(kthread, policy, &sp);
    kthread_bind(kthread, core);

    wake_up_process(kthread);
    hrtimer_start(&ahrtimer, kt, HRTIMER_MODE_REL);


    return 0;
}

/* exit function - logs that the module is being removed */
static void 
interfering_exit(void)
{
    hrtimer_cancel(&ahrtimer);
    kthread_stop(kthread);
    printk(KERN_ALERT "interfering module is being unloaded\n");
}

module_init(interfering_init);
module_exit(interfering_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Bingxin Liu");
MODULE_DESCRIPTION ("Studio13");