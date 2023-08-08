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
int epfd;
struct epoll_event host_events[MAX_EVENTS];
struct epoll_event host_event;

struct fd_pair {
    int hostfd;
    int clientfd;
};
#define MAX_CONNECTIONS 1024
struct fd_pair fd_map[MAX_CONNECTIONS] = {0};

void add_fd_pair(struct fd_pair *map, int hostfd, int clientfd) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if ((map[i].hostfd == 0 && map[i].clientfd == 0) || (map[i].hostfd == -1 && map[i].clientfd == -1)) {
            map[i].hostfd = hostfd;
            map[i].clientfd = clientfd;
            return;
        }
    }
    printf("Map is full, cannot add new pair\n");
}

void remove_fd_pair(struct fd_pair *map, int hostfd) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (map[i].hostfd == hostfd) {
            map[i].hostfd = -1;
            map[i].clientfd = -1; 
            return;
        }
    }
    printf("Pair not found in map\n");
}

int get_hostfd(struct fd_pair *map, int clientfd) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (map[i].clientfd == clientfd) {
            return map[i].hostfd;
        }
    }
    return -1; 
}

int get_clientfd(struct fd_pair *map, int hostfd) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (map[i].hostfd == hostfd) {
            return map[i].clientfd;
        }
    }
    return -1; 
}
/* Set the socket descriptor to non-blocking */
int set_to_nonblocking(int hostfd){
    int flags = fcntl(hostfd, F_GETFL, 0);
    if (flags == -1){
        printf("fcntl failed\n");
        return -1;
    }
    flags |= O_NONBLOCK;
    int s = fcntl(hostfd, F_SETFL, flags);
    if (s == -1){
        printf("fcntl failed\n");
        return -1;
    }
    return 0;
}

/* Connect to host */
int connect_to_host(char *ip, uint16_t port){
    int i;
    int hostfd_local = socket(AF_INET, SOCK_STREAM, 0);
    if (hostfd_local < 0) {
        printf("socket failed, hostfd:%d, errno:%d, %s\n", hostfd_local, errno, strerror(errno));
        exit(1);
    }
    int ret = set_to_nonblocking(hostfd_local);
    if (ret < 0){
        printf("set to non blocking failed\n");
        close(hostfd_local);
        return -1;
    }
    host_event.events = EPOLLOUT;
    host_event.data.fd = hostfd_local;
    /* Add to the e-poll */
    epoll_ctl(epfd, EPOLL_CTL_ADD, hostfd_local, &host_event);
    struct sockaddr_in host_addr;
    bzero(&host_addr, sizeof(host_addr));

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &host_addr.sin_addr) <= 0) {
        perror("Failed to set server address\n");
        close(hostfd_local);
        return -1;
    }

    if (connect(hostfd_local, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
        if (errno != EINPROGRESS) {
            printf("connect failed, hostfd:%d, errno:%d, %s\n", hostfd_local, errno, strerror(errno));
            close(hostfd_local);
            return -1;
        }
    }

    return hostfd_local;
}

/* Send to host */
int send_to_host(int hostfd, char *data, size_t len){
    int i;
    int bytes_sent = 0;
    int bytes_read = 0;
    if (hostfd < 0){
        printf("hostfd not initialized\n");
        // if(connect_to_host(hostip, hostport) < 0){
        //     printf("connect to host failed at branch 1\n");
        //     return -1;
        // }
    }
    int num_ready = epoll_wait(epfd, host_events, MAX_EVENTS, 0);
    if (num_ready < 0){
        printf("epoll wait failed\n");
        return -1;
    }
    for (i = 0 ; i < num_ready; i++){
        if(host_events[i].events & EPOLLOUT){
            printf("am i here\n");
            bytes_sent = write(hostfd, data, len);
            printf("The number of bytes sent to host is %d\n", bytes_sent);
            if (bytes_sent < 0){
                printf("send to host failed\n");
                return -1;
            }

        }
    }
    if (bytes_sent > 0){
        host_event.events = EPOLLIN;
        host_event.data.fd = hostfd;
        /* Add to the e-poll */
        epoll_ctl(epfd, EPOLL_CTL_ADD, hostfd, &host_event);
    }
    return bytes_sent;
}

/* Read from host */
int read_from_host(int hostfd, char *data, size_t len){
    int i;
    int bytes_read = 0;

    if (hostfd < 0){
        printf("hostfd not initialized\n");
        return -1;
    }

    int num_ready = epoll_wait(epfd, host_events, MAX_EVENTS, 10000);
    if (num_ready < 0){
        printf("epoll wait failed while reading\n");
        return -1;
    }
    printf("The number of events ready to be read from host is %d\n", num_ready);
    printf("The hostfd is %d\n", hostfd);
    for (i = 0; i < num_ready; i++){          
        printf("The hostfd is %d\n", host_events[i].data.fd);
        printf("The events are %d\n", host_events[i].events);
        if(host_events[i].data.fd == hostfd && host_events[i].events & EPOLLIN){
            bytes_read = read(hostfd, data, len);
            if (bytes_read < 0){
                printf("here 1");
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("No more data ready to be read\n");
                    break;
                }
            }
        }    
    }
    printf("The number of bytes read from host is %d\n", bytes_read);
    return bytes_read;
}

int loop(void *arg)
{   
    /* Wait for events to happen */
    int nevents = ff_kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
    int i;

    if (nevents < 0) {
        printf("ff_kevent failed:%d, %s\n", errno,
                        strerror(errno));
        return -1;
    }

    for (i = 0; i < nevents; ++i) {
        struct kevent event = events[i];
        int clientfd = (int)event.ident;

        /* Handle disconnect */
        if (event.flags & EV_EOF) {
            /* Simply close socket */
            ff_close(clientfd);

            /* Get the associated host fd */
            int host_fd = get_hostfd(fd_map, clientfd);
            if (host_fd < 0){
                printf("hostfd not found\n");
                return -1;
            }

            /* Close host fd */
            int ret = close(host_fd);
            if (ret < 0) {
                printf("close failed, hostfd:%d, errno:%d, %s\n", host_fd, errno, strerror(errno));
                return -1;
            }
            printf("Closed hostfd %d\n", host_fd);
            /*Remove from fd pair */
            remove_fd_pair(fd_map, host_fd);
#ifdef INET6
        } else if (clientfd == sockfd || clientfd == sockfd6) {
#else
        } else if (clientfd == sockfd) {
#endif
            int available = (int)event.data;
            do {
                int nclientfd = ff_accept(clientfd, NULL, NULL);
                if (nclientfd < 0) {
                    printf("ff_accept failed:%d, %s\n", errno,
                        strerror(errno));
                    break;
                }
                /* Create a connection to host and map the hostfd to clientfd*/
                int hostfd = connect_to_host(hostip, hostport);
                printf("The hostfd is %d\n", hostfd);
                if (hostfd < 0){
                    printf("connect to host failed at branch 2\n");
                    return -1;
                }
                add_fd_pair(fd_map, hostfd, nclientfd);

                /* Add to event list */
                EV_SET(&kevSet, nclientfd, EVFILT_READ, EV_ADD, 0, 0, NULL);

                if(ff_kevent(kq, &kevSet, 1, NULL, 0, NULL) < 0) {
                    printf("ff_kevent error:%d, %s\n", errno,
                        strerror(errno));
                    return -1;
                }

                available--;
            } while (available);            
        } else if (event.filter == EVFILT_READ) {
            size_t nbytes=256;

            /* Allocate double pointer to pass to the ff_read*/ 
            ptr = NULL;  
            void **ptr1 = &ptr;

            /* Pass the **ptr to ff_read */
            ssize_t readlen = ff_read(clientfd, ptr1, nbytes);
            printf("clientfd is %d\n", clientfd);
            /* Get the pointer to data from the freebsd mbuf */
            char *data = (char *) ff_mbuf_mtod(ptr);\

        }else if (event.filter == EVFILT_WRITE){
            /*Write to the client*/


        } else {
            printf("unknown event: %8.8X\n", event.flags);
        }
    }
    // E-polling on Linux socket 
    int nhost_events = epoll_wait(epfd, host_events, MAX_EVENTS, 0);
    if (nhost_events < 0){
        printf("epoll wait failed\n");
        return -1;
    }

    for (i = 0; i < nhost_events; i++){
        if (host_events[i].events & EPOLLOUT){
            // Get the request reacieved from the client

            // Write to that request to the host 


        }else if (host_events[i].events & EPOLLIN){
            
            // Read from the host 

            // Store the data for sending to client
        }else{
            printf("unknown event: %8.8X\n", host_events[i].events);
        }
    }
    return 0;
}   

int main(int argc, char * argv[])
{
    ff_init(argc, argv);

    kq = ff_kqueue();
    if (kq < 0) {
        printf("ff_kqueue failed, errno:%d, %s\n", errno, strerror(errno));
        exit(1);
    }

    sockfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
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
    if (ret < 0) {
        printf("ff_bind failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    ret = ff_listen(sockfd, MAX_EVENTS);
    if (ret < 0) {
        printf("ff_listen failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    EV_SET(&kevSet, sockfd, EVFILT_READ, EV_ADD, 0, MAX_EVENTS, NULL);
    /* Update kqueue */
    ff_kevent(kq, &kevSet, 1, NULL, 0, NULL);

    /* Create a e-poll file descriptor for host id  */
    epfd = epoll_create(MAX_EVENTS);

#ifdef INET6
    sockfd6 = ff_socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd6 < 0) {
        printf("ff_socket failed, sockfd6:%d, errno:%d, %s\n", sockfd6, errno, strerror(errno));
        exit(1);
    }

    struct sockaddr_in6 my_addr6;
    bzero(&my_addr6, sizeof(my_addr6));
    my_addr6.sin6_family = AF_INET6;
    my_addr6.sin6_port = htons(80);
    my_addr6.sin6_addr = in6addr_any;

    ret = ff_bind(sockfd6, (struct linux_sockaddr *)&my_addr6, sizeof(my_addr6));
    if (ret < 0) {
        printf("ff_bind failed, sockfd6:%d, errno:%d, %s\n", sockfd6, errno, strerror(errno));
        exit(1);
    }

    ret = ff_listen(sockfd6, MAX_EVENTS);
    if (ret < 0) {
        printf("ff_listen failed, sockfd6:%d, errno:%d, %s\n", sockfd6, errno, strerror(errno));
        exit(1);
    }

    EV_SET(&kevSet, sockfd6, EVFILT_READ, EV_ADD, 0, MAX_EVENTS, NULL);
    ret = ff_kevent(kq, &kevSet, 1, NULL, 0, NULL);
    if (ret < 0) {
        printf("ff_kevent failed:%d, %s\n", errno, strerror(errno));
        exit(1);
    }
#endif
    /* Create a host socket */
    ff_run(loop, NULL);
    return 0;
}