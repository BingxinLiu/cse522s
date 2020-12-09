#include <linux/kernel.h>
#include <uapi/asm-generic/socket.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/delay.h>
#include <linux/inet.h>

//#define LISTEN_IP 0xc0a83205
#define LISTEN_IP INADDR_ANY
#define LISTEN_PORT 9000
#define BUF_LEN 512

struct task_struct *server = NULL;

static int server_start(void *data) {
	int status, size;
	struct socket *sock;
	struct sockaddr_in addr;
	char buf[BUF_LEN];
    struct msghdr msg;
    struct kvec vec = {
        .iov_base = buf,
        .iov_len = BUF_LEN
    };
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 100000
    };
	
	if((status = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock)) < 0) {
		printk(KERN_ERR "Could not create a datagram socket, error = %d\n", -ENXIO);
		server = NULL;
		return 1;
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(LISTEN_IP);
	addr.sin_port = htons(LISTEN_PORT);
	
	if((status = kernel_bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr))) < 0) {
		printk(KERN_ERR "Could not bind or connect to socket, error = %d\n", -status);
		sock_release(sock);
		server = NULL;
		return 1;
	}

   	// if((status = kernel_listen(sock, 10)) < 0) {
	// 	printk(KERN_ERR "Could not listent to socket, error = %d\n", -status);
	// 	sock_release(sock);
	// 	server = NULL;
	// 	return 1;
	// }

    kernel_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO_OLD, (char *)&tv, sizeof(struct timeval));

	printk(KERN_INFO "Listening on port %d\n", LISTEN_PORT);

	while(!kthread_should_stop()) {
		memset(buf, 0, BUF_LEN);
        memset(&msg, 0, sizeof(struct msghdr));
		size = kernel_recvmsg(sock, &msg, &vec, 1, BUF_LEN, 0);
		if(size < 0) {
			//printk(KERN_ERR "Failed to receive message\n");
		}
		else {
			printk(KERN_INFO "Received message: %s\n", buf);

		}
	}
	sock_release(sock);
    return 0;
}

static int udp_server_init(void) {
	printk(KERN_INFO "UDP server loaded\n");
	server = kthread_create(server_start, NULL, "kernel_udp_server");
	if(IS_ERR(server)) {
		printk(KERN_ERR "Failed to create server thread\n");
		return PTR_ERR(server);
	}
	wake_up_process(server);
	return 0;
}

static void udp_server_exit(void) {
	if(server != NULL) {
        kthread_stop(server);
    }
	printk(KERN_INFO "Module unloaded\n");
}

module_init(udp_server_init);
module_exit(udp_server_exit);
MODULE_LICENSE("GPL");
