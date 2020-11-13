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

static struct task_struct * kthread1 = NULL;
static struct task_struct * kthread2 = NULL;
static struct task_struct * kthread3 = NULL;
static struct task_struct * kthread4 = NULL;
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
wait(void)
{
    atomic_sub_return(1, &counter);
    while(1)
    {
        if (atomic_read(&counter) == 0) return;
    }
}

static int
thread_fn(void * data)
{
    int number;
    number = *((int *) data);

    aTimestamps.ts_array[number * 2] = ktime_get();
    printk("Hello from thread %s.\n", current->comm);
    aTimestamps.ts_array[number * 2 + 1] = ktime_get();

    wait();

    return 0;
}

/* init function - logs that initialization happened, returns success */
static int 
timing_init(void)
{
    unsigned int i, data1, data2, data3, data4;

    struct sched_param sp1, sp2, sp3, sp4;

    data1 = 0;  data2 = 1;  data3 = 2;  data4 = 3;

    setup(4, &counter);

    aTimestamps.size = 8;
    aTimestamps.ts_array = (ktime_t *) kmalloc(sizeof(ktime_t) * 8, GFP_KERNEL);

    printk(KERN_ALERT "simple module initialized\n");
    for (i = 0; i < aTimestamps.size; i++)
    {
        aTimestamps.ts_array[i] = ktime_set(0, 0);
    }

    kthread1 = kthread_create(thread_fn, &data1, "kthrd_1");
    if (IS_ERR(kthread1))
    {
        printk(KERN_ERR "Failed to create kernel thread1.\n");
        return PTR_ERR(kthread1);
    }
    kthread2 = kthread_create(thread_fn, &data2, "kthrd_2");
    if (IS_ERR(kthread2))
    {
        printk(KERN_ERR "Failed to create kernel thread2.\n");
        return PTR_ERR(kthread2);
    }
    kthread3 = kthread_create(thread_fn, &data3, "kthrd_3");
    if (IS_ERR(kthread3))
    {
        printk(KERN_ERR "Failed to create kernel thread3.\n");
        return PTR_ERR(kthread3);
    }
    kthread4 = kthread_create(thread_fn, &data4, "kthrd_4");
    if (IS_ERR(kthread4))
    {
        printk(KERN_ERR "Failed to create kernel thread4.\n");
        return PTR_ERR(kthread4);
    }
    sp1.sched_priority = 60;
    sp2.sched_priority = 55;
    sp3.sched_priority = 50;
    sp4.sched_priority = 45;
    sched_setscheduler(kthread1, SCHED_FIFO, &sp1);
    sched_setscheduler(kthread2, SCHED_FIFO, &sp2);
    sched_setscheduler(kthread3, SCHED_FIFO, &sp3);
    sched_setscheduler(kthread4, SCHED_FIFO, &sp4);
    kthread_bind(kthread1, 0);
    kthread_bind(kthread2, 1);
    kthread_bind(kthread3, 2);
    kthread_bind(kthread4, 4);
    wake_up_process(kthread1);
    wake_up_process(kthread2);
    wake_up_process(kthread3);
    wake_up_process(kthread4);

    return 0;
}

/* exit function - logs that the module is being removed */
static void 
timing_exit(void)
{
    unsigned int i, j;
    ktime_t tmp;
    ktime_t array[5];
    printk(KERN_ALERT "simple module is being unloaded\n");
    kthread_stop(kthread1);
    kthread_stop(kthread2);
    kthread_stop(kthread3);
    kthread_stop(kthread4);
    for (i = 0; i < aTimestamps.size; i++)
    {
        printk("timestamp[%d]: %lld\n", i, ktime_to_us(aTimestamps.ts_array[i]));
    }
    array[0] = aTimestamps.ts_array[0];
    for (i = 0; i < 4; i++)
    {
        if (ktime_after(aTimestamps.ts_array[i * 2], array[0]))
            array[0] = aTimestamps.ts_array[i * 2];
    }
    for (i = 0; i < 4; i++)
    {
        array[i+1] = aTimestamps.ts_array[i * 2 + 1];
    }
    for (i = 4; i > 0; i--)
    {
        for (j = 1; j < i; j++)
            if (ktime_after(array[j], array[j+1]))
            {
                tmp = array[i];
                array[i] = array[j];
                array[j] = tmp;
            }
    }
    printk("The difference between the last pre-barrier timestamps and first post-barrier is %lld.\n", ktime_us_delta(array[0], array[1]));
    printk("The difference between the last pre-barrier timestamps and second post-barrier is %lld.\n", ktime_us_delta(array[0], array[2]));
    printk("The difference between the last pre-barrier timestamps and third post-barrier is %lld.\n", ktime_us_delta(array[0], array[3]));
    printk("The difference between the last pre-barrier timestamps and forth post-barrier is %lld.\n", ktime_us_delta(array[0], array[4]));


    kfree(aTimestamps.ts_array);

}

module_init(timing_init);
module_exit(timing_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Bingxin Liu");
MODULE_DESCRIPTION ("Studio8");