#include <katja/proxy.h>
#include <katja/connection.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#define SAMPLE_PROXY_IPV4_ADDR_BYTES	(139, 177, 179, 5)
#define SAMPLE_PROXY_SMP_PROXY_PORT	(10007)
#define SAMPLE_PROXY_SMP_LOCAL_PORT	(CONFIG_MCUMGR_TRANSPORT_UDP_PORT)

static K_SEM_DEFINE(proxy_net_connected_sem, 0, 1);
static uint8_t recv_buf[1500];
static struct sockaddr proxy_smp_ai_addr;
static socklen_t proxy_smp_ai_addrlen;
static struct sockaddr local_smp_ai_addr;
static socklen_t local_smp_ai_addrlen;
static struct sockaddr recv_ai_addr;
static socklen_t recv_ai_addrlen;

static void proxy_init_sockaddr(void)
{
	net_sin(&proxy_smp_ai_addr)->sin_family = AF_INET;
	net_sin(&proxy_smp_ai_addr)->sin_port = htons(SAMPLE_PROXY_SMP_PROXY_PORT);
	net_sin(&proxy_smp_ai_addr)->sin_addr.s4_addr[0] = 139;
	net_sin(&proxy_smp_ai_addr)->sin_addr.s4_addr[1] = 177;
	net_sin(&proxy_smp_ai_addr)->sin_addr.s4_addr[2] = 179;
	net_sin(&proxy_smp_ai_addr)->sin_addr.s4_addr[3] = 5;
	proxy_smp_ai_addrlen = sizeof(struct sockaddr_in);

	net_sin(&local_smp_ai_addr)->sin_family = AF_INET;
	net_sin(&local_smp_ai_addr)->sin_port = htons(SAMPLE_PROXY_SMP_LOCAL_PORT);
	net_sin(&local_smp_ai_addr)->sin_addr.s4_addr[0] = 127;
	net_sin(&local_smp_ai_addr)->sin_addr.s4_addr[1] = 0;
	net_sin(&local_smp_ai_addr)->sin_addr.s4_addr[2] = 0;
	net_sin(&local_smp_ai_addr)->sin_addr.s4_addr[3] = 1;
	local_smp_ai_addrlen = sizeof(struct sockaddr_in);
}

static bool proxy_sockaddr_is_identical(const struct sockaddr* ai_addr1,
					const struct sockaddr* ai_addr2)
{
	return memcmp(ai_addr1, ai_addr2, sizeof(struct sockaddr_in)) == 0;
}

static void proxy_callback(const struct zbus_channel *channel)
{
	const struct connection_message *message;

	message = zbus_chan_const_msg(channel);

	if (message->cellular_status == CONNECTION_STATUS_CONNECTED) {
		k_sem_give(&proxy_net_connected_sem);
	}
}

ZBUS_LISTENER_DEFINE(proxy_listener, proxy_callback);

static void proxy_run_proxy(void)
{
	int sock_smp;
	int ret;

	printk("Opening sockets\n");
	sock_smp = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_smp < 0) {
		printk("Failed to open smp socket\n");
		return;
	}

	printk("Register sockets\n");
	ret = zsock_sendto(sock_smp, "h", 1, 0,
			   &proxy_smp_ai_addr, proxy_smp_ai_addrlen);
	if (ret < 0) {
		printk("Failed to register socket\n");
		return;
	}

	printk("Polling socket\n");
	while (true) {
		recv_ai_addrlen = sizeof(recv_ai_addr);
		ret = zsock_recvfrom(sock_smp, recv_buf, sizeof(recv_buf), 0,
					&recv_ai_addr, &recv_ai_addrlen);
		if (proxy_sockaddr_is_identical(&recv_ai_addr, &proxy_smp_ai_addr)) {
			zsock_sendto(sock_smp, recv_buf, (size_t)ret, 0,
					&local_smp_ai_addr, local_smp_ai_addrlen);
		} else {
			zsock_sendto(sock_smp, recv_buf, (size_t)ret, 0,
					&proxy_smp_ai_addr, proxy_smp_ai_addrlen);
		}
	}
}

static void proxy_routine(void)
{
	proxy_init_sockaddr();

	k_sem_take(&proxy_net_connected_sem, K_FOREVER);

	proxy_run_proxy();
}

K_THREAD_DEFINE(proxy_thread, CONFIG_MAIN_STACK_SIZE, proxy_routine, NULL, NULL, NULL,
		3, 0, 0);
