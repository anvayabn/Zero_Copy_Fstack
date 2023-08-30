/*
 * Some sort of Copyright
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <ff_config.h>
#include <ff_api.h>
#include "connection.h"
#include "listen_sock.h"
#include "signal.h"
#include "shm.h"
#include "../common/error.h"
#include "../common/shm.h"

#define GATEWAY_ID 0
#define IVSHMEM_PROT_VERSION 0
#define MGR_SOCK_PATH "/tmp/ivshmem_socket"
#define CONTROL_SHM_SIZE (sizeof(struct unimsg_shm))

extern struct unimsg_ring *rte_mempool_unimsg_ring;
static int peers_fds[UNIMSG_MAX_VMS];
static struct unimsg_shm *control_shm;
static void *buffers_shm;
static pthread_t mgr_handler_t;
static volatile int stop = 0;

static void ivshmem_client_read_one_msg(int sock_fd, int64_t *index, int *fd)
{
	int ret;
	struct msghdr msg;
	struct iovec iov[1];
	union {
		struct cmsghdr cmsg;
		char control[CMSG_SPACE(sizeof(int))];
	} msg_control;
	struct cmsghdr *cmsg;

	iov[0].iov_base = index;
	iov[0].iov_len = sizeof(*index);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &msg_control;
	msg.msg_controllen = sizeof(msg_control);

	ret = recvmsg(sock_fd, &msg, 0);
	if (ret == 0)
		SYSERROR("Lost connection to manager");
	else if (ret < sizeof(*index))
		SYSERROR("Error receiving data from manager");

	*fd = -1;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_len != CMSG_LEN(sizeof(int))
		    || cmsg->cmsg_level != SOL_SOCKET
		    || cmsg->cmsg_type != SCM_RIGHTS)
			continue;

		memcpy(fd, CMSG_DATA(cmsg), sizeof(*fd));
	}
}

void *mgr_notification_handler(void *arg)
{
	int rc;
	int sock = (int)(unsigned long)arg;

	struct pollfd pfd = {0};
	pfd.fd = sock;
	pfd.events = POLLIN;

	while (!stop) {
		rc = poll(&pfd, 1, 1000);
		if (rc < 0)
			SYSERROR("Error polling manager connection");
		if (rc == 0)
			continue;

		if (pfd.revents & ~POLLIN)
			ERROR("Error polling manager connection");

		if (!(pfd.revents & POLLIN))
			continue;

		int64_t val;
		int fd;
		ivshmem_client_read_one_msg(sock, &val, &fd);
		if (fd > 0) {
			/* New peer */
			if (peers_fds[val] != 0) {
				/* Unimsg allows a single fd per peer */
				ERROR("Notification for existing peer");
			}

			peers_fds[val] = fd;
			printf("Peer %ld registered\n", val);
		
		} else {
			/* Peer disconnected */
			peers_fds[val] = 0;
			printf("Peer %ld unregistered\n", val);
		}
	}

	close(sock);
}

static void *setup_manager_connection()
{
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
		SYSERROR("Error creating socket");

	printf("Connecting to Unimsg manager...\n");

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, MGR_SOCK_PATH);
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)))
		SYSERROR("Error connecting to manager");

	printf("Connected to Unimsg manager\n");
	printf("Receiving configuration...\n");

	int64_t val;
	int aux_fd;

	/* Receive protocol version IVSHMEM_PROT_VERSION */
	ivshmem_client_read_one_msg(sock, &val, &aux_fd);
	if (val != IVSHMEM_PROT_VERSION)
		ERROR("Received unexpected procol version %lu", val);

	/* Receive id GATEWAY_ID */
	ivshmem_client_read_one_msg(sock, &val, &aux_fd);
	if (val != GATEWAY_ID)
		ERROR("Expected id %d, received %ld", GATEWAY_ID, val);

	/* Receive control shm fd */
	ivshmem_client_read_one_msg(sock, &val, &aux_fd);
	if (val != -1 || aux_fd < 0)
		ERROR("Error receiving control shm");

	void *shm = mmap(0, CONTROL_SHM_SIZE, PROT_READ | PROT_WRITE,
			 MAP_SHARED, aux_fd, 0);
	if (!shm)
		SYSERROR("Error mapping control shm");

	/* Receive interrupt configuration */
	ivshmem_client_read_one_msg(sock, &val, &aux_fd);
	if (val != GATEWAY_ID)
		ERROR("Gateway should be the first connecting to manager");
	if (aux_fd < 0)
		ERROR("Error receiving interrupt configuration");

	peers_fds[val] = aux_fd;

	/* Create a thread to handle nofitications from manager */
	if (pthread_create(&mgr_handler_t, NULL, mgr_notification_handler,
			   (void *)(unsigned long)sock))
		SYSERROR("Error creating thread");

	printf("Configuration completed\n");
	printf("Manager connection established\n");

	return shm;
}

static void *get_buffers_shm()
{
	int fd = open(UNIMSG_BUFFERS_PATH, O_RDWR);
	if (fd < 0)
		SYSERROR("Error opening unimsg buffers file");

	void *shm = mmap(0, UNIMSG_BUFFER_SIZE * UNIMSG_BUFFERS_COUNT,
			 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (!shm)
		SYSERROR("Error mapping unimsg buffers");

	close(fd);

	/* Read a byte to map the first page in the page table */
	*(volatile char *)shm;

	return shm;
}

static int connect_to_peer(uint32_t addr, uint16_t port, struct conn **c)
{
	int rc;

	/* TODO: replace with hash lookup */
	unsigned peer_id =
		control_shm->rt.routes[addr % UNIMSG_RT_SIZE].peer_id;
	if (!peer_id)
		return -ENETUNREACH;

	struct listen_sock *ls;
	rc = listen_sock_lookup_acquire(addr, port, &ls);
	if (rc)
		return rc;

	struct conn_id id;
	/* TODO: replace with addr and port of remote client */
	id.client_id = 0;
	id.client_addr = 100; /* Addr of the remote client */
	id.client_port = 100; /* Port of the remote client */
	id.server_id = peer_id;
	id.server_addr = addr;
	id.server_port = port;
	rc = conn_alloc(c, &id);
	if (rc)
		goto err_release_ls;

	rc = listen_sock_send_conn(ls, *c);
	if (rc)
		goto err_free_conn;

	listen_sock_release(ls);

	return 0;

err_free_conn:
	conn_free(*c);
err_release_ls:
	listen_sock_release(ls);
	return rc;
}

static int handle_peer_connection()
{
	int rc;
	unsigned idx;
	
	rc = unimsg_ring_dequeue(&control_shm->gw_backlog.r, &idx, 1);
	if (rc == -EAGAIN)
		return rc;
	if (rc)
		ERROR("Error accepting connection: %s\n", strerror(-rc));

	struct conn *c = conn_from_idx(idx);

	printf("Received connection from %u:%u to %u:%u\n", c->id.client_addr,
	       c->id.client_port, c->id.server_addr, c->id.server_port);

	/* Echo all messages until the peer closes the conection */
	struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
	unsigned ndescs;
	for (;;) {
		ndescs = UNIMSG_MAX_DESCS_BULK;
		do
			rc = conn_recv(c, descs, &ndescs, CONN_SIDE_SRV);
		while (rc == -EAGAIN);
		if (rc == -ECONNRESET)
			break;
		else if (rc)
			ERROR("Error receiving desc: %s\n", strerror(-rc));

		do
			rc = conn_send(c, descs, ndescs, CONN_SIDE_SRV);
		while (rc == -EAGAIN);
		if (rc == -ECONNRESET) {
			unimsg_buffer_put(descs, ndescs);
			break;
		} else if (rc) {
			unimsg_buffer_put(descs, ndescs);
			ERROR("Error sending desc: %s\n", strerror(-rc));
		}
	}

	conn_close(c, CONN_SIDE_SRV);

	return 0;
}

int loop(void *arg)
{
	return 0;
}

static void sigint_handler(int signum)
{
	stop = 1;
}

int main(int argc, char *argv[])
{
	control_shm = setup_manager_connection();
	buffers_shm = get_buffers_shm();

	/* Configure the ring for the rte mempool */
	/* TODO: this is awful, find a best API to set the ring */
	rte_mempool_unimsg_ring = &control_shm->shm_pool.r;

	ff_init(argc, argv, buffers_shm, UNIMSG_BUFFERS_COUNT,
		UNIMSG_BUFFER_SIZE);

	shm_init(control_shm, buffers_shm);
	signal_init(control_shm, peers_fds);
	conn_init(control_shm);
	listen_sock_init(control_shm);

	struct sigaction sigact = { .sa_handler = sigint_handler };
	if (sigaction(SIGINT, &sigact, NULL))
		SYSERROR("Error setting SIGINT handler");

	while (!stop)
		handle_peer_connection();

	pthread_join(mgr_handler_t, NULL);

	return 0;
}