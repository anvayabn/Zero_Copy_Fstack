#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <sys/ioctl.h>

// to send to host
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "../lib/ff_config.h"
#include "../lib/ff_api.h"
#include "../lib/ff_veth.h"

/* dpdk libraries for manipulating the rte_mbuf */
#include "../dpdk/lib/mbuf/rte_mbuf.h"
#include "../dpdk/lib/mbuf/rte_mbuf_core.h"

#define MAX_EVENTS 512
/* kevent set */
struct kevent kevSet;
/* events */
struct kevent events[MAX_EVENTS];
/* kq */
int kq;
int sockfd;
void *ptr;
#ifdef INET6
int sockfd6;
#endif

char html[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: F-Stack\r\n"
    "Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 0\r\n"
    "Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
    "Connection: keep-alive\r\n"
    "Accept-Ranges: bytes\r\n"
    "\r\n";

char html2[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: F-Stack\r\n"
    "Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 363\r\n"
    "Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
    "Connection: keep-alive\r\n"
    "Accept-Ranges: bytes\r\n"
    "\r\n"
    "<!DOCTYPE html>\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<title>Welcome to F-Stack!</title>\r\n"
    "<style>\r\n"
    "    body {  \r\n"
    "        width: 35em;\r\n"
    "        margin: 0 auto; \r\n"
    "        font-family: Tahoma, Verdana, Arial, sans-serif;\r\n"
    "    }\r\n"
    "</style>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Welcome to F-Stack!</h1>\r\n"
    "<h2>Alice's Adventures in Wonderland</h2>\r\n"
    "<h3>Chapter I: Down the Rabbit-Hole</h3>\r\n"
    "</body>\r\n"
    "</html>";

char html33[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: F-Stack\r\n"
    "Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 438\r\n"
    "Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
    "Connection: keep-alive\r\n"
    "Accept-Ranges: bytes\r\n"
    "\r\n"
    "<!DOCTYPE html>\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<title>Welcome to F-Stack!</title>\r\n"
    "<style>\r\n"
    "    body {  \r\n"
    "        width: 35em;\r\n"
    "        margin: 0 auto; \r\n"
    "        font-family: Tahoma, Verdana, Arial, sans-serif;\r\n"
    "    }\r\n"
    "</style>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Welcome to F-Stack!</h1>\r\n"
    "\r\n"
    "<p>For online documentation and support please refer to\r\n"
    "<a href=\"http://F-Stack.org/\">F-Stack.org</a>.<br/>\r\n"
    "\r\n"
    "<p><em>Thank you for using F-Stack.</em></p>\r\n"
    "</body>\r\n"
    "</html>";

char html3[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: F-Stack\r\n"
    "Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 703\r\n"
    "Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
    "Connection: keep-alive\r\n"
    "Accept-Ranges: bytes\r\n"
    "\r\n"
    "<!DOCTYPE html>\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<title>Welcome to F-Stack!</title>\r\n"
    "<style>\r\n"
    "    body {  \r\n"
    "        width: 35em;\r\n"
    "        margin: 0 auto; \r\n"
    "        font-family: Tahoma, Verdana, Arial, sans-serif;\r\n"
    "    }\r\n"
    "</style>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Welcome to F-Stack!</h1>\r\n"
    "<h2>Alice's Adventures in Wonderland</h2>\r\n"
    "<h3>Chapter I: Down the Rabbit-Hole</h3>\r\n"
    "<p>So she was considering in her own mind (as well as she could, for the hot day made her feel very sleepy and stupid), whether the sound of making daisy-chains would be worth the trouble of getting up and picking the daisies, when suddenly a White Rabbit with pink eyes ran close by her.</p>\r\n"
    "<p><em>Thank you for using F-Stack.</em></p>\r\n"
    "</body>\r\n"
    "</html>";

char html4[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: F-Stack\r\n"
    "Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 1021\r\n"
    "Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
    "Connection: keep-alive\r\n"
    "Accept-Ranges: bytes\r\n"
    "\r\n"
    "<!DOCTYPE html>\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<title>Welcome to F-Stack!</title>\r\n"
    "<style>\r\n"
    "    body {  \r\n"
    "        width: 35em;\r\n"
    "        margin: 0 auto; \r\n"
    "        font-family: Tahoma, Verdana, Arial, sans-serif;\r\n"
    "    }\r\n"
    "</style>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Welcome to F-Stack!</h1>\r\n"
    "<h2>Alice's Adventures in Wonderland</h2>\r\n"
    "<h3>Chapter I: Down the Rabbit-Hole</h3>\r\n"
    "<p>Alice was beginning to get very tired of sitting by her sister on the bank, and of having nothing to do: once or twice she had peeped into the book her sister was reading, but it had no pictures or conversations in it, ‘and what is the use of a book,’ thought Alice ‘without pictures or conversation?’</p>\r\n"
    "<p>So she was considering in her own mind (as well as she could, for the hot day made her feel very sleepy and stupid), whether the sound of making daisy-chains would be worth the trouble of getting up and picking the daisies, when suddenly a White Rabbit with pink eyes ran close by her.</p>\r\n"
    "<p><em>Thank you for using F-Stack.</em></p>\r\n"
    "</body>\r\n"
    "</html>";

char html5[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: F-Stack\r\n"
    "Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 1222\r\n"
    "Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
    "Connection: keep-alive\r\n"
    "Accept-Ranges: bytes\r\n"
    "\r\n"
    "<!DOCTYPE html>\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<title>Welcome to F-Stack!</title>\r\n"
    "<style>\r\n"
    "    body {  \r\n"
    "        width: 35em;\r\n"
    "        margin: 0 auto; \r\n"
    "        font-family: Tahoma, Verdana, Arial, sans-serif;\r\n"
    "    }\r\n"
    "</style>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Welcome to F-Stack!</h1>\r\n"
    "<p>Alice was beginning to get get shs scdcd by her sister on the bank, and of having nothing to do: once or twice she had peeped into the book her sister was reading, but it had no pictures or conversations in it, ‘and what is the use of a book,’ thought Alice ‘without pictures or conversation?’</p>\r\n"
    "<p>So she was considering in her own mind (as well as she could, for the hot day made her feel very sleepy and stupid), whether the sound of making daisy-chains would be worth the trouble of getting up and picking the daisies, when suddenly a White Rabbit with pink eyes ran close by her.</p>\r\n"
    "<p>So she was considering in her own mind (as well as she could, for the hot day made her feel very sleepy and stupid), whether the sound of making daisy-chains would be worth the trouble of getting up and picking the daisies, when suddenly a White Rabbit with pink eyes ran close by her.</p>\r\n"
    "<p><em>Thank you for using F-Stack.</em></p>\r\n"
    "</body>\r\n"
    "</html>";

/* Global variable to host socket */
char *hostip = "192.168.1.1";
uint16_t hostport = 8000;
size_t len_from_hostdata = 158;

char *host_data[256];

int loop_num = 0;
struct fd_pair
{
    int hostfd;
    int clientfd;
};
#define MAX_CONNECTIONS 1024
struct fd_pair fd_map[MAX_CONNECTIONS] = {0};

void add_fd_pair(struct fd_pair *map, int hostfd, int clientfd)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if ((map[i].hostfd == 0 && map[i].clientfd == 0) || (map[i].hostfd == -1 && map[i].clientfd == -1))
        {
            map[i].hostfd = hostfd;
            map[i].clientfd = clientfd;
            return;
        }
    }
    printf("Map is full, cannot add new pair\n");
}

void remove_fd_pair(struct fd_pair *map, int hostfd)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (map[i].hostfd == hostfd)
        {
            map[i].hostfd = -1;
            map[i].clientfd = -1;
            return;
        }
    }
    printf("Pair not found in map\n");
}

int get_hostfd(struct fd_pair *map, int clientfd)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (map[i].clientfd == clientfd)
        {
            return map[i].hostfd;
        }
    }
    return -1;
}

int get_clientfd(struct fd_pair *map, int hostfd)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (map[i].hostfd == hostfd)
        {
            return map[i].clientfd;
        }
    }
    return -1;
}
/* Connect to host */
int connect_to_host(char *ip, uint16_t port)
{
    int i;
    int hostfd_local = socket(AF_INET, SOCK_STREAM, 0);
    if (hostfd_local < 0)
    {
        printf("socket failed, hostfd:%d, errno:%d, %s\n", hostfd_local, errno, strerror(errno));
        exit(1);
    }
    struct sockaddr_in host_addr;
    bzero(&host_addr, sizeof(host_addr));
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &host_addr.sin_addr) <= 0)
    {
        perror("Failed to set server address\n");
        close(hostfd_local);
        return -1;
    }

    if (connect(hostfd_local, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0)
    {
        printf("connect failed, hostfd:%d, errno:%d, %s\n", hostfd_local, errno, strerror(errno));
        close(hostfd_local);
        return -1;
    }

    return hostfd_local;
}

int loop(void *arg)
{
    /* Scan for events on the kq*/
    int nevents = ff_kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
    if (nevents < 0)
    {
        printf("ff_kevent failed:%d, %s\n", errno, strerror(errno));
        exit(1);
    }
    /* Loop on the event notification provided by the kq */
    int i;
    for (i = 0; i < nevents; ++i)
    {
        struct kevent event = events[i];
        int clientfd = (int)event.ident;

        /* If end of flag noticed on fstack socket close the fd*/
        if (event.flags & EV_EOF)
        {
            /* Simply close socket */
            ff_close(clientfd);

            /* Get the associated host fd */
            int host_fd = get_hostfd(fd_map, clientfd);
            if (host_fd < 0)
            {
                printf("hostfd not found\n");
                return -1;
            }

            /* Close host fd */
            int ret = close(host_fd);
            if (ret < 0)
            {
                printf("close failed, hostfd:%d, errno:%d, %s\n", host_fd, errno, strerror(errno));
                return -1;
            }
            printf("Closed hostfd %d\n", host_fd);
            /*Remove from fd pair */
            remove_fd_pair(fd_map, host_fd);
        }
        else if (clientfd == sockfd)
        {
            int available = (int)event.data;
            do
            {
                int nclientfd = ff_accept(clientfd, NULL, NULL);
                if (nclientfd < 0)
                {
                    printf("ff_accept failed, clientfd:%d, errno:%d, %s\n", clientfd, errno, strerror(errno));
                    break;
                }

                /* Create connection to host and map the hostfd to clientfd */
                int host_fd = connect_to_host(hostip, hostport);
                if (host_fd < 0)
                {
                    printf("connect to host failed\n");
                    break;
                }
                add_fd_pair(fd_map, host_fd, nclientfd);

                EV_SET(&kevSet, nclientfd, EVFILT_READ, EV_ADD, 0, 0, NULL);

                if (ff_kevent(kq, &kevSet, 1, NULL, 0, NULL) < 0)
                {
                    printf("ff_kevent error:%d, %s\n", errno,
                           strerror(errno));
                    return -1;
                }

                available--;
            } while (available);
        }
        else if (event.filter == EVFILT_READ)
        {
            size_t nbytes = 256;

            /* Allocate double pointer to pass to the ff_read*/
            ptr = NULL;
            void **ptr1 = &ptr;

            /* Pass the **ptr to ff_read */
            ssize_t readlen = ff_read(clientfd, ptr1, nbytes);
            if (readlen < 0)
            {
                printf("ff_read failed, clientfd:%d, errno:%d, %s\n", clientfd, errno, strerror(errno));
                break;
            }
            /* Get the pointer to data from the freebsd mbuf */
            char *data = (char *)ff_mbuf_mtod(ptr);

            /* Immediently try to send to host */
            int hostfd = get_hostfd(fd_map, clientfd);
            if (hostfd < 0)
            {
                printf("hostfd not initialized\n");
                break;
            }
            int bytes_sent = write(hostfd, data, readlen);
            if (bytes_sent < 0)
            {
                printf("send to host failed\n");
                break;
            }
            /* Clean the data buffer */
            memset(host_data, 0, sizeof(host_data));
            // int k;
            // for (int j = 0 ; j < 100000000; j++){
            //     k++;
            // }
            /* Read from host */
            int bytes_read = read(hostfd, host_data, sizeof(host_data));
            printf("bytes_read %d\n", bytes_read);
            /* if read was successfull then send data back to client */
            if (bytes_read >= 0){
                void *rteMbuf_void = ff_rte_frm_extcl(ptr);
                struct rte_mbuf *rteMbuf = (struct rte_mbuf *)rteMbuf_void;

                ff_mbuf_detach_rte(ptr);
                ff_mbuf_free(ptr);
                rte_pktmbuf_reset(rteMbuf);
                char *data11 = rte_pktmbuf_mtod_offset(rteMbuf, char *, 0);

                memcpy(data11, host_data, bytes_read);
                rteMbuf->pkt_len = bytes_read;
                rteMbuf->data_len = bytes_read;

                void *bsd_mbuf = ff_mbuf_get(NULL, rteMbuf_void, (void*)data11, bytes_read);
                ff_write(clientfd, bsd_mbuf, bytes_read);

            }else{
                /* Do something */
                goto ret;
            }
        }
        else
        {
            printf("unknown event: %8.8X\n", event.flags);
        }
    }
ret:    
    return 0;
}

int main(int argc, char *argv[])
{
    ff_init(argc, argv);

    kq = ff_kqueue();
    if (kq < 0)
    {
        printf("ff_kqueue failed, errno:%d, %s\n", errno, strerror(errno));
        exit(1);
    }

    sockfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("ff_socket failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }
    /* Set non blocking */
    int on = 1;
    ff_ioctl(sockfd, FIONBIO, &on);

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(80);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = ff_bind(sockfd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0)
    {
        printf("ff_bind failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    ret = ff_listen(sockfd, MAX_EVENTS);
    if (ret < 0)
    {
        printf("ff_listen failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    EV_SET(&kevSet, sockfd, EVFILT_READ, EV_ADD, 0, MAX_EVENTS, NULL);
    /* Update kqueue */
    ff_kevent(kq, &kevSet, 1, NULL, 0, NULL);

    /* Create a host socket */
    ff_run(loop, NULL);
    return 0;
}