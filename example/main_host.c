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

void *ptr;  

/* Global variable to host socket */
int hostfd = -1;
char *hostip = "192.168.1.1";
uint16_t hostport = 8000;
char *from_hostdata;
size_t len_from_hostdata =1024;

/* Function to Connect to host */
int connect_to_host(char *ip, uint16_t port){
    hostfd = socket(AF_INET, SOCK_STREAM, 0);
    if (hostfd < 0) {
        printf("ff_socket failed, hostfd:%d, errno:%d, %s\n", hostfd, errno, strerror(errno));
        exit(1);
    }
    struct sockaddr_in host_addr;
    bzero(&host_addr, sizeof(host_addr));

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &host_addr.sin_addr) <= 0) {
        perror("Failed to set server address\n");
        close(hostfd);
        return -1;
    }

    if (connect(hostfd, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
        perror("Failed to connect to server\n");
        close(hostfd);
        return -1;
    }
    return 0;
}

/* Function to send to host */
int send_to_host(char *data, size_t len){
    if (hostfd < 0){
        if (connect_to_host(hostip, hostport) < 0){
            printf("connect to host failed\n");
            return -1;
        }
    }
    if (write(hostfd, data, len) < 0){
        printf("write to host failed\n");
        return -1;
    }
    return 0;
}
/* Read from host */
int read_from_host(char *data, size_t len){
    if (hostfd < 0){
        printf("hostfd not initialized\n");
        return -1;
    }
    if (read(hostfd, data, len) < 0){
        printf("read from host failed\n");
        return -1;
    }
    return 0;
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
            // close (hostfd);
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

            /*Allocate double pointer to pass to the ff_read*/ 
            ptr = NULL;  
            void **ptr1 = &ptr;

            /* Pass the **ptr to ff_read */
            ssize_t readlen = ff_read(clientfd, ptr1, nbytes);

            /* Get the pointer to data from the freebsd mbuf */
            char *data = (char *) ff_mbuf_mtod(ptr);

            /* Write the data to host */
            if (send_to_host(data, readlen) < 0){
                printf("send to host failed\n");
                return -1;
            }

            /* Read from data */
            if (read_from_host(from_hostdata, len_from_hostdata) < 0){
                printf("read from host failed\n");
                return -1;
            }
            printf("The contents of from_hostdata is %s\n", from_hostdata);

            /* Get the rte_mbuf associated with the freebsd mbuf */
            void *rteMbuf_void = ff_rte_frm_extcl(ptr);
            struct rte_mbuf *rteMbuf = (struct rte_mbuf *)rteMbuf_void;

           /* Detach the rte_mbuf from the freebsd mbuf fo that the free mbuf can be released */
            ff_mbuf_detach_rte(ptr);
            ff_mbuf_free(ptr);

            /* Reset the rte_mbuf to default values */
            rte_pktmbuf_reset(rteMbuf);
            /* Get the pointer to data from the rte_mbuf */
            char *data11 = rte_pktmbuf_mtod_offset(rteMbuf, char *, 0);

            /*Replace the contents of data11 and update the pkt flags */
            memcpy(data11, html, sizeof(html) - 1);
            rteMbuf->data_len = sizeof(html) - 1;
            rteMbuf->pkt_len = sizeof(html) - 1;
            
            /* Get a new freebsd mbuf with ext_arg set as the rte_mbf*/
            void *bsd_mbuf = ff_mbuf_get(NULL, rteMbuf_void, (void*)data11, sizeof(html) - 1);

            /* Write the bsd_mbuf to the socket */
            ff_write(clientfd, bsd_mbuf, sizeof(html) - 1);
        } else {
            printf("unknown event: %8.8X\n", event.flags);
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
    if (connect_to_host(hostip, hostport) < 0){
        printf("connect to host failed\n");
        return -1;
    }

    ff_run(loop, NULL);
    return 0;
}