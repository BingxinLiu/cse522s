#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ktime.h>
#include <linux/gfp.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/hrtimer.h>

static struct task_struct * kthread = NULL;
static struct task_struct * child_kthread = NULL;
static struct hrtimer ahrtimer;
static struct hrtimer childtimer;
static atomic_t counter;

typedef struct timestamps_t
{
    unsigned int size;
    ktime_t * ts_array;

} timestamps;

static struct timestamps_t aTimestamps = {
    .size = 0,
    .ts_array = NULL
};

static void
setup(int number, atomic_t * ptr_to_atomic)
{
    atomic_set(ptr_to_atomic, number);
}

static void
wait()
{
    atomic_sub_return(1, &counter);
    while(1)
    {
        if (atomic_read(&counter) == 0) return;
    }
}

static enum hrtimer_restart
child_timer_function(struct hrtimer * ahrtimer)
{
    // 6 - timestamp, before imediately wake up the higher priority thread by calling the wake_up_process() function.
    aTimestamps.ts_array[6] = ktime_get();
    wake_up_process(kthread);
    return HRTIMER_NORESTART;
}

static enum hrtimer_restart
timer_function(struct hrtimer * ahrtimer)
{
    // 3 - timestamp, before imediately wake up the kernel thread by calling the wake_up_process() function.
    aTimestamps.ts_array[3] = ktime_get();
    wake_up_process(kthread);
    return HRTIMER_NORESTART;
}

static int
child_thread_fn(void * data)
{
    printk("Hello from the child thread %s.", current->comm);
    int i;
    i = 0;
    while(i < 10000000)
    {
        ktime_get();
        i++;
    }

    return 0;
}

static int
thread_fn(void * data)
{
    // 4 - timestamp, Modify the thread function so that the first thing it does is to record a timestamp
    aTimestamps.ts_array[4] = ktime_get();
    printk("Hello from thread %s. The 4th position of timestamp is%lld\n", current->comm, ktime_to_us(aTimestamps.ts_array[4]));

    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    // 5 - timestamp, as the very next thing it does after it wakes back up
    aTimestamps.ts_array[5] = ktime_get();

    child_kthread = kthread_create(child_thread_fn, NULL, "child_timing_thread");
    kthread_bind(child_kthread, 3);
    struct sched_param sp = 
    {
        .sched_priority = 45
    };
    sched_setscheduler(child_kthread, SCHED_FIFO, &sp);
    wake_up_process(child_kthread);


    hrtimer_init(&childtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    childtimer.function = child_timer_function;
    hrtimer_start(&childtimer, ktime_set(1, 0), HRTIMER_MODE_REL);

    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    // 7 - timestamp, After the higher-priority thread is woken up
    aTimestamps.ts_array[7] = ktime_get();

    return 0;
}

/* init function - logs that initialization happened, returns success */
static int 
timing_init(void)
{
    unsigned int i;

    struct sched_param sp =
    {
        .sched_priority = 50
    };

    aTimestamps.size = 8;
    aTimestamps.ts_array = (ktime_t *) kmalloc(sizeof(ktime_t) * 8, GFP_KERNEL);

    hrtimer_init(&ahrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    ahrtimer.function = timer_function;

    printk(KERN_ALERT "simple module initialized\n");
    for (i = 0; i < aTimestamps.size; i++)
    {
        aTimestamps.ts_array[i] = ktime_set(0, 0);
    }
    // 0 - timestamp, after initializing the timestamp variables
    aTimestamps.ts_array[0] = ktime_get();

    kthread = kthread_create(thread_fn, NULL, "timing_thread");
    // 1 - timestamp, right after the kthread_create() function returns
    aTimestamps.ts_array[1] = ktime_get();
    hrtimer_start(&ahrtimer, ktime_set(1, 0), HRTIMER_MODE_REL);
    if (IS_ERR(kthread))
    {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(kthread);
    }
    sched_setscheduler(kthread, SCHED_FIFO, &sp);
    // 2 - timestamp, immediately after that use the sched_setscheduler()
    aTimestamps.ts_array[2] = ktime_get();

    kthread_bind(kthread, 3);

    return 0;
}

/* exit function - logs that the module is being removed */
static void 
timing_exit(void)
{
    unsigned int i;
    printk(KERN_ALERT "simple module is being unloaded\n");
    kthread_stop(kthread);
    kthread_stop(child_kthread);
    for (i = 0; i < aTimestamps.size; i++)
    {
        printk("timestamp[%d]: %lld\n", i, ktime_to_us(aTimestamps.ts_array[i]));
    }
    printk("The difference between the timestamps at positions1 and 0 is %lld.\n", ktime_us_delta(aTimestamps.ts_array[1], aTimestamps.ts_array[0]));
    printk("The difference between the timestamps at positions2 and 1 is %lld.\n", ktime_us_delta(aTimestamps.ts_array[2], aTimestamps.ts_array[1]));
    printk("The difference between the timestamps at positions4 and 0 is %lld.\n", ktime_us_delta(aTimestamps.ts_array[4], aTimestamps.ts_array[0]));
    printk("The difference between the timestamps at positions5 and 3 is %lld.\n", ktime_us_delta(aTimestamps.ts_array[5], aTimestamps.ts_array[3]));
    printk("The difference between the timestamps at positions7 and 6 is %lld.\n", ktime_us_delta(aTimestamps.ts_array[7], aTimestamps.ts_array[6]));
    hrtimer_cancel(&ahrtimer);
    hrtimer_cancel(&childtimer);
    kfree(aTimestamps.ts_array);

}

module_init(timing_init);
module_exit(timing_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Bingxin Liu");
MODULE_DESCRIPTION ("Studio8");