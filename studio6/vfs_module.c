#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/path.h>
#include <linux/fs_struct.h>
#include <linux/nsproxy.h>
#include <linux/dcache.h>
#include <linux/mount.h>
#include <linux/miscdevice.h>

static struct task_struct * kthread = NULL;

struct fs_struct * fs;
struct path pwd;
struct path root;
struct dentry *entry;
struct dentry *eentry;
struct dentry *subentry;

static int
thread_fn(void * data)
{

    printk("kernel thread %s created.\n", current->comm);

    // Q2
    printk("task_struct->fs = %p\n", current->fs);
    printk("task_struct->files = %p\n", current->files);
    printk("task_struct->nsproxy = %p\n", current->nsproxy);

    // Q3
    fs = current->fs;
    get_fs_pwd(fs, &pwd);
    get_fs_root(fs, &root);
    printk("fs->pwd->mnt = %p\n", pwd.mnt);
    printk("fs->root->mnt = %p\n", root.mnt);
    if ( root.mnt != pwd.mnt )
    {
        printk("fs->pwd->mnt->mnt_root = %p\n", pwd.mnt->mnt_root);
        printk("fs->pwd->mnt->mnt_sb = %p\n", pwd.mnt->mnt_sb);
        printk("fs->root->mnt->mnt_root = %p\n", root.mnt->mnt_root);
        printk("fs->root->mnt->mnt_sb = %p\n", root.mnt->mnt_sb);
    }

    // Q4
    printk("pwd->dentry = %p\n", pwd.dentry);
    printk("root->dentry = %p\n", root.dentry);
    if ( pwd.dentry != root.dentry)
    {
        printk("pwd->d_iname = %s\n", pwd.dentry->d_iname);
        printk("root->d_iname = %s\n", root.dentry->d_iname);
    }

    // Q5
    printk("All of files and directories under root directory:\n");
    list_for_each_entry(entry, &(root.dentry->d_subdirs), d_child)
    {
        printk("%s\t", entry->d_iname);
    }
    printk("\n");

    // Q6
    printk("All of subdirectories of directories under root directory:\n");
    list_for_each_entry(eentry, &(root.dentry->d_subdirs), d_child)
    {
        if (&(eentry->d_subdirs) != NULL)
        {
            list_for_each_entry(subentry, &(eentry->d_subdirs), d_child)
            {
                printk("%s\t", subentry->d_iname);
            }
        }
    }
    printk("\n");
    
    
    

    while (!kthread_should_stop()) {
        schedule();
    }

    return 0;
}

static int
kernel_memory_init(void)
{
    printk(KERN_INFO "Loaded vfs module\n");

    kthread = kthread_create(thread_fn, NULL, "vfs");
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
    printk(KERN_INFO "Unloaded vfs module\n");
}

module_init(kernel_memory_init);
module_exit(kernel_memory_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Bingxin Liu");
MODULE_DESCRIPTION ("cse522s studio6");