#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>

#include <uapi/asm-generic/socket.h>


#include <linux/errno.h>
#include <linux/types.h>

#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>

#include <linux/delay.h>
#include <linux/un.h>
#include <linux/unistd.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/unistd.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <net/inet_connection_sock.h>
#include <net/request_sock.h>

#include <linux/inet.h>
#include <linux/types.h>
#include <linux/net.h>

#include <linux/delay.h>
#include <uapi/linux/limits.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/gfp.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/mm.h>

#include <linux/string.h>

#define CONNECT_IP "192.168.50.102"
#define CONNECT_PORT 8086
#define BUF_SIZE 1024

static struct task_struct * kthread_tcp_client = NULL;
static unsigned char buf[BUF_SIZE];

int
ktcp_send(struct socket * sock, unsigned char * buf)
{
    int err, flags;
    struct msghdr msg = \
    {
        .msg_name = NULL,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_namelen = 0
    };
    struct kvec vec = \
    {
        .iov_base = buf,
        .iov_len = BUF_SIZE
    };

    //debug
    printk(KERN_INFO "Prepare to send message\n");

    err = kernel_sendmsg(sock, &msg, &vec, 1, BUF_SIZE);
    if (err < 0)
    {
        printk(KERN_ERR "Failed to send message\n");
    }
    else
    {
        printk(KERN_INFO "Send message: %s\n", buf);
    }

    return err;
}

static int
ktcp_client_start_threadfn(void * data)
{
    int err;
    struct socket * sock;
    struct sockaddr_in addr = \
    {
        .sin_family = AF_INET,
        .sin_addr = in_aton(CONNECT_IP),
        .sin_port = { htons(CONNECT_PORT) }
    };

    printk("Starting ktcp client, thread %s.\n", current->comm);

    // create socket
    err = sock_create_kern(&init_net, PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if(err < 0)
    {
        printk(KERN_ERR "Create ktcp socket failed with error %d.\n", err);
        sock_release(sock);
        return err;
    }

    err = kernel_connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr), 0);
    if (err < 0)
    {
        printk(KERN_ERR "Connect with ktcp socket failed with error %d.\n", err);
        sock_release(sock);
        return err;
    }

    memset(buf, 0, BUF_SIZE);
    strcpy(buf, "hello from client\n");
    ktcp_send(sock, buf);

    while (!kthread_should_stop()) {
        schedule();
    }

    //tcp_fin(sock->sk);
    sock_release(sock);

    return 0;
}

static int
ktcp_client_init(void)
{
    printk(KERN_INFO "Loaded ktcp_client module\n");

    kthread_tcp_client = kthread_create(ktcp_client_start_threadfn, NULL, "ktcp_client");
    if (IS_ERR(kthread_tcp_client)) {
        printk(KERN_ERR "Failed to create ktcp_client thread\n");
        return PTR_ERR(kthread_tcp_client);
    }
    
    wake_up_process(kthread_tcp_client);

    return 0;
}

static void 
ktcp_client_exit(void)
{
    kthread_stop(kthread_tcp_client);
    printk(KERN_INFO "Unloaded ktcp_client module\n");
}

module_init(ktcp_client_init);
module_exit(ktcp_client_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR ("B. Liu & H. Huang");
MODULE_DESCRIPTION ("a test module for kernel tcp client");