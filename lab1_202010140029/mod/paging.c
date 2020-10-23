#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/memory.h>
#include <linux/mm.h>

#include <paging.h>

static atomic_t alloc_number;
static atomic_t free_number;

static unsigned int demand_paging = 1;
module_param(demand_paging, uint, 0644);

struct private_data
{
    atomic_t reference;
    struct list_head ptr_to_pagelist;
};

struct pages
{
    struct page * page_address;
    struct list_head page_list;
};

static int
do_fault(struct vm_area_struct * vma,
         unsigned long           fault_address)
{
    struct page * aPage = NULL;
    struct private_data * cur_pdata;
    struct pages * cell;
    printk(KERN_INFO "paging_vma_fault() invoked: took a page fault at VA 0x%lx\n", fault_address);
    aPage = alloc_page(GFP_KERNEL);
    if (aPage == NULL)
    {
        printk(KERN_INFO "Error: alloc_page fail.\n");
        return VM_FAULT_OOM;
    }

    if (remap_pfn_range(vma, fault_address & (~(PAGE_SIZE - 1)), page_to_pfn(aPage), PAGE_SIZE, vma->vm_page_prot) == -1)
    {
        printk(KERN_INFO "Error: remap_pfn_range fail.\n");
        return VM_FAULT_SIGBUS;
    }
    atomic_inc(&alloc_number);
    cur_pdata = (struct private_data *) vma->vm_private_data;
    cell = kmalloc(sizeof(struct pages), GFP_KERNEL);
    cell->page_address = aPage;
    INIT_LIST_HEAD(&cell->page_list);
    list_add(&cell->page_list, &cur_pdata->ptr_to_pagelist);

    return VM_FAULT_NOPAGE;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static int
paging_vma_fault(struct vm_area_struct * vma,
                 struct vm_fault       * vmf)
{
    unsigned long fault_address = (unsigned long)vmf->virtual_address
#else
static int
paging_vma_fault(struct vm_fault * vmf)

{
    struct vm_area_struct * vma = vmf->vma;
    unsigned long fault_address = (unsigned long)vmf->address;
#endif

    return do_fault(vma, fault_address);
}

static void
paging_vma_open(struct vm_area_struct * vma)
{
    struct private_data * cur = (struct private_data *)vma->vm_private_data;
    atomic_inc(&cur->reference);
    printk(KERN_INFO "paging_vma_open() invoked\n");
}

static void
paging_vma_close(struct vm_area_struct * vma)
{
    printk(KERN_INFO "paging_vma_close() invoked\n");

    struct private_data * cur = (struct private_data *)vma->vm_private_data;
    if (atomic_dec_return(&cur->reference) == 0)
    {
        if (demand_paging) {
            struct pages * p;
            list_for_each_entry(p, &cur->ptr_to_pagelist, page_list)
            {
                __free_page(p->page_address);
                atomic_inc(&free_number);
                kfree(p);
            }
            list_del(&cur->ptr_to_pagelist);
        }
        else {
            unsigned int length = vma->vm_end - vma->vm_start;
            unsigned int order = 0;
            unsigned int page_num = (length % PAGE_SIZE) ? (length / PAGE_SIZE + 1) : (length / PAGE_SIZE);
            while ( (1 << order) < page_num )
            {
                order++;
            }
            free_pages(vma->vm_start, order);
        }
        kfree(vma->vm_private_data);
    }

}

static struct vm_operations_struct
paging_vma_ops = 
{
    .fault = paging_vma_fault,
    .open  = paging_vma_open,
    .close = paging_vma_close
};

static int
do_prepaging(struct vm_area_struct * vma)
{
        unsigned int length = vma->vm_end - vma->vm_start;
        unsigned int order = 0;
        unsigned int page_num = (length % PAGE_SIZE) ? (length / PAGE_SIZE + 1) : (length / PAGE_SIZE);
        struct page * pages;
        while ( (1 << order) < page_num )
        {
            order++;
        }
        pages = alloc_pages(GFP_KERNEL, order);
        if (!pages)
        {
            printk("Error: allocate pages fail.\n");
            return VM_FAULT_OOM;
        }
        if (remap_pfn_range(vma, vma->vm_start, page_to_pfn(pages), length, vma->vm_page_prot))
        {
            printk(KERN_INFO "Error: remap_pfn_range fail.\n");
            return VM_FAULT_SIGBUS;
        }
        return VM_FAULT_NOPAGE;
}


/* vma is the new virtual address segment for the process */
static int
paging_mmap(struct file           * filp,
            struct vm_area_struct * vma)
{
    printk(KERN_INFO "paging_mmap() invoked: new VMA for pid %d from VA 0x%lx to 0x%lx\n", current->pid, vma->vm_start, vma->vm_end);

    /* prevent Linux from mucking with our VMA (expanding it, merging it 
     * with other VMAs, etc.)
     */
    vma->vm_flags |= VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_NORESERVE
              | VM_DONTDUMP | VM_PFNMAP;

    /* setup the vma->vm_ops, so we can catch page faults */
    vma->vm_ops = &paging_vma_ops;

    struct private_data * current_private_data = kmalloc(sizeof(struct private_data), GFP_KERNEL);
    atomic_set(&current_private_data->reference, 1);

    vma->vm_private_data = current_private_data;

    if (demand_paging == 0)
    {   
        switch (do_prepaging(vma))
        {
        case VM_FAULT_OOM:
            return -ENOMEM;
            break;
        case VM_FAULT_SIGBUS:
            return -EFAULT;
            break;
        case VM_FAULT_NOPAGE:
            return 0;
            break;
        default:
            return 1;
            break;
        }
    }

    INIT_LIST_HEAD(&current_private_data->ptr_to_pagelist);
    return 0;
}

static struct file_operations
dev_ops =
{
    .mmap = paging_mmap,
};

static struct miscdevice
dev_handle =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = PAGING_MODULE_NAME,
    .fops = &dev_ops,
};
/*** END device I/O **/

/*** Kernel module initialization and teardown ***/
static int
kmod_paging_init(void)
{
    int status;

    /* Create a character device to communicate with user-space via file I/O operations */
    status = misc_register(&dev_handle);
    if (status != 0) {
        printk(KERN_ERR "Failed to register misc. device for module\n");
        return status;
    }

    printk(KERN_INFO "Loaded kmod_paging module\n");

    return 0;
}

static void
kmod_paging_exit(void)
{
    /* Deregister our device file */
    misc_deregister(&dev_handle);
    printk("The alloc_number is %d\n", atomic_read(&alloc_number));
    printk("The free_number is %d\n", atomic_read(&free_number));
    printk(KERN_INFO "Unloaded kmod_paging module\n");
}

module_init(kmod_paging_init);
module_exit(kmod_paging_exit);

/* Misc module info */
MODULE_LICENSE("GPL");
