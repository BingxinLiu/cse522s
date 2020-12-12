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

#define LISTEN_IP INADDR_ANY
#define LISTEN_PORT 8086
#define CONNECT_PORT 8087
#define BUF_SIZE 1024

static struct task_struct * kthread_ktcp_server = NULL;
static struct ktcp_service * ktcp_svc;
static unsigned char buf[BUF_SIZE];

struct ktcp_service
{
    int running;
    struct task_struct *thread;
    struct socket *listen_socket;
};

int ktcp_recv(struct socket * sock, unsigned char * buf)
{
    int err, flags;
    
    struct msghdr msg;
    // struct sockaddr_storage address;
    struct kvec vec = \
    {
        .iov_base = buf,
        .iov_len = BUF_SIZE 
    };

    if (sock == NULL)
    {
        printk(KERN_ERR "ERROR: client socket is empty!\n");
    }

    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
	msg.msg_iocb = NULL;
	msg.msg_flags = 0;

    flags = 0;

    // if (sock->file->f_flags & O_NONBLOCK)   flags |= MSG_DONTWAIT;
    // debug
    printk(KERN_INFO "Prepare to receive message\n");

    err = kernel_recvmsg(sock, &msg, &vec, 1, BUF_SIZE, flags);
    if (err < 0)
    {
        printk(KERN_ERR "Failed to receive message\n");
    }
    else
    {
        printk(KERN_INFO "Server Received: %s\n", buf);
    }
    
    return err;    
}

static int
connection_threadfn(void * data)
{
    struct socket * sock;

    sock = (struct socket *) data;

    memset(&buf, 0, BUF_SIZE);
    printk(KERN_INFO "receive a package");
    while(ktcp_recv(sock, buf))
    {
        memset(&buf, 0, BUF_SIZE);
    }

    printk(KERN_INFO "End of TCP connect, release client sock.\n");

    sock_release(sock);

    kfree(sock);

    printk(KERN_INFO "Release client sock, done.\n");

    return 0;
}

static int
ktcp_server_start_threadfn(void * data)
{
    DECLARE_WAITQUEUE(wq, current);
    int err;
    struct socket * sock, * client_sock;
    struct inet_connection_sock *isock;
    struct task_struct * connection_thread = NULL;
    struct sockaddr_in addr = \
    {
        .sin_family = AF_INET,
        .sin_port = htons( LISTEN_PORT ),
        .sin_addr = { htonl ( LISTEN_IP )}
    };

    printk(KERN_INFO "starting ktcp server, thread %s\n", current->comm);

    // update ktcp status, record ktcp is running
    ktcp_svc->running = 1;

    // create socket
    err = sock_create_kern(&init_net, PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if(err < 0)
    {
        printk(KERN_ERR "create ktcp socket failed with error %d.\n", err);
        sock_release(sock);
        return err;
    }

    printk(KERN_INFO "ktcp socket created.\n");

    // bind socket
    err = sock->ops->bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    if(err < 0)
    {
        printk(KERN_ERR "bind ktcp socket failed with error %d.\n", err);
        sock_release(sock);
        return err;
    }

    // listen on socket
    err = sock->ops->listen(sock, 128);
    if(err < 0)
    {
        printk(KERN_ERR "listen to ktcp socket failed with error %d.\n", err);
        sock_release(sock);
        return err;
    }

    isock = inet_csk(sock->sk);
    // creat waiting socket
    // debug
    printk(KERN_INFO "inet_csk done");
    

    while(!kthread_should_stop())
    {

        // while loop for waiting connection

        // check waiting queue empty
        if (reqsk_queue_empty(&isock->icsk_accept_queue))
        {
            add_wait_queue(&sock->sk->sk_wq->wait, &wq);
            set_current_state(TASK_INTERRUPTIBLE);
            schedule();
            printk(KERN_INFO "icsk queue empty?: %d\n",reqsk_queue_empty(&isock->icsk_accept_queue));
			printk(KERN_INFO "recv queue empty?: %d\n",skb_queue_empty(&sock->sk->sk_receive_queue));
            remove_wait_queue(&sock->sk->sk_wq->wait, &wq);
            continue;
        }

        // not empty accept connect
        client_sock = (struct socket *) kmalloc(sizeof(struct socket), GFP_KERNEL);
        err = sock_create_kern(&init_net, PF_INET, SOCK_STREAM, IPPROTO_TCP, &client_sock);
        if(err < 0)
        {
            printk(KERN_ERR "create ktcp client socket failed with error %d.\n", err);
            sock_release(sock);
            return err;
        }
        err = kernel_accept(sock, &client_sock, O_NONBLOCK);
        if(err < 0)
        {
            printk(KERN_ERR "accept ktcp client socket failed with error %d.\n", err);
            return err;
        }

        connection_thread = kthread_create(connection_threadfn, (void *) client_sock, "connection_thread");
        if (IS_ERR(connection_thread))
        {
            printk(KERN_ERR "Failed to create connection thread\n");
            return PTR_ERR(connection_thread);
        }
        wake_up_process(connection_thread);

    }

    // after break from while loop, release socket
    sock_release(sock);

    printk(KERN_INFO "Release server sock, done.\n");

    return 0;
}

static int
ktcp_server_init(void)
{
    printk(KERN_INFO "Loaded ktcp_server module\n");

    ktcp_svc = (struct ktcp_service *) kmalloc(sizeof(struct ktcp_service), GFP_KERNEL);

    kthread_ktcp_server = kthread_create(ktcp_server_start_threadfn, NULL, "ktcp_server");
    if (IS_ERR(kthread_ktcp_server))
    {
        printk(KERN_ERR "Failed to create ktcp_server thread\n");
        return PTR_ERR(kthread_ktcp_server);
    }

    ktcp_svc->thread = kthread_ktcp_server;

    
    wake_up_process(kthread_ktcp_server);

    return 0;
}

static void
ktcp_server_exit(void)
{
    kthread_stop(kthread_ktcp_server);
    kfree(ktcp_svc);
    printk(KERN_INFO "Unloaded ktcp_server module\n");
}

module_init(ktcp_server_init);
module_exit(ktcp_server_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR ("B. Liu & H. Huang");
MODULE_DESCRIPTION ("a test module for kernel tcp");
