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

static uint nr_structs = 2000;
module_param(nr_structs, uint, 0644);

//Q2
#define ARR_SIZE 8
typedef struct datatype_t
{
    /* data */
    unsigned int array[ARR_SIZE];
} datatype;

// Q3
static unsigned int nr_pages;
static unsigned int order;
static unsigned int nr_structs_per_page;
static unsigned int tail;

// Q4
static struct page * pages;
static struct datatype_t * dt;

static struct task_struct * kthread = NULL;

static unsigned int
my_get_order(unsigned int nr_pages)
{
    unsigned int shifts = 0;

    if (!nr_pages)
        return 0;

    if (!(nr_pages & (nr_pages - 1)))
        nr_pages--;

    while (nr_pages > 0) {
        nr_pages >>= 1;
        shifts++;
    }

    return shifts;
}

static int
thread_fn(void * data)
{
    // variables
    unsigned int i, j, k;
    unsigned long cur_page, cur_struct;
    struct datatype_t * this_struct;

    printk("Hello from thread %s. nr_structs=%u\n", current->comm, nr_structs);

    // Q2
    printk("The kernel's page size is %lu.\n", PAGE_SIZE);
    printk("The size of \"datatype\" struct is %u.\n", sizeof(datatype));
    printk("The number of \"datatype\" structs that fill fit in a single page of memory is %lu", PAGE_SIZE/sizeof(datatype));

    // Q3
    if ((nr_structs * sizeof(datatype)) / PAGE_SIZE == 0)
    {
        tail = 0;
    } else
    {
        tail = 1;
    } 
    nr_pages = (nr_structs * sizeof(datatype)) / PAGE_SIZE + tail;
    order  = my_get_order(nr_pages);
    nr_structs_per_page  = nr_structs / nr_pages;

    printk("nr_pages = %u\n", nr_pages);
    printk("order = %u\n", order);
    printk("nr_structs_per_page = %u\n", nr_structs_per_page);

    //Q4
    pages = alloc_pages(GFP_KERNEL, order);
    if (!pages)
        return -ENOMEM;
    dt = (struct datatype_t *)__va(PFN_PHYS(page_to_pfn(pages)));
    
    for (i = 0; i < nr_pages; i++)
    {
        cur_page = PFN_PHYS(page_to_pfn(pages + i));
        for (j = 0; j < nr_structs_per_page; j++)
        {
            cur_struct = cur_page + j * 32;
            this_struct = (struct datatype_t *)__va(cur_struct);
            for (k  = 0; k < ARR_SIZE; k++) {
                this_struct->array[k] = i * nr_structs_per_page * 8 + j * 8 + k;
                /*
                if (k == 0 && j == 0)
                {
                    printk("In page %u, the value of the first element is: %u", \
                            i, this_struct->array[k]);
                }
                */
            }
        }
        
    }

    // end of Q, after materials in this function are original
    while (!kthread_should_stop()) {
        schedule();
    }

    // Q5
    for (i = 0; i < nr_pages; i++)
    {
        cur_page = PFN_PHYS(page_to_pfn(pages + i));
        for (j = 0; j < nr_structs_per_page; j++)
        {
            cur_struct = cur_page + j * 32;
            this_struct = (struct datatype_t *)__va(cur_struct);
            for (k  = 0; k < ARR_SIZE; k++) {
                if(this_struct->array[k] != (i * nr_structs_per_page * 8 + j * 8 + k))
                    printk("Error: the address of the %uth struct's %uth element in page %u doesn't match.\n", j, k, i);
            }
        }
        
    }
    __free_pages(pages, order);

    return 0;
}

static int
kernel_memory_init(void)
{
    printk(KERN_INFO "Loaded kernel_memory module\n");

    kthread = kthread_create(thread_fn, NULL, "k_memory");
    if (IS_ERR(kthread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(kthread);
    }
    
    wake_up_process(kthread);

    return 0;
}

static void 
kernel_memory_exit(void)
{
    kthread_stop(kthread);
    printk(KERN_INFO "Unloaded kernel_memory module\n");
}

module_init(kernel_memory_init);
module_exit(kernel_memory_exit);

MODULE_LICENSE ("GPL");
