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
#include "../lib/ff_config.h"
#include "../lib/ff_api.h"
#include "../lib/ff_veth.h"
#include "../dpdk/lib/mbuf/rte_mbuf.h"
#include "../dpdk/lib/mbuf/rte_mbuf_core.h"

#define MAX_EVENTS 512

int sockfd;
int request_sent = 0;
int received = 0;
int kq;
size_t nbytes=147;
struct kevent ke;
struct kevent events[MAX_EVENTS];

const char* request =
"GET / HTTP/1.1\r\n"
"Host: 10.10.1.2\r\n"
"User-Agent: f-stack/7.47.0\r\n"
"Accept: */*\r\n"
"\r\n";

int loop(void *arg) {

    /* Wait for events to happen */
    int nevents = ff_kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);

    if (nevents < 0) {
        printf("ff_kevent failed:%d, %s\n", errno,
               strerror(errno));
        return -1;
    }

    /* Iterating over all the events */
    for(int i = 0; i < nevents; i++) {
        struct kevent event = events[i];
        int clientfd = (int)event.ident;
        
        /* Todo:: Handle diconnects and error */


        // /* Handle disconnect */
        // if (event.flags & EV_EOF) {
        //     /* Simply close socket */
        //     ff_close(clientfd);
        // }
        // if (request_sent == 1 && received == 1){
        //     ff_close(clientfd);
        // }

        /* Handle write event */
        if (event.filter == EVFILT_WRITE && request_sent != 1) {
            unsigned lcore_id = rte_lcore_id();
            unsigned socketid = rte_lcore_to_socket_id(lcore_id);
            char s[64];
            snprintf(s, sizeof(s), "mbuf_pool_%d", socketid);
            struct rte_mempool *mp = rte_mempool_lookup(s);

            if (!mp) {
                printf("Cannot get memory pool.\n");
                return -1;
            }

            struct rte_mbuf *m = rte_pktmbuf_alloc(mp);
            if (!m) {
                printf("Cannot allocate mbuf.\n");
                return -1;
            }
            char *data11 = rte_pktmbuf_mtod_offset(m, char *, 0);
            printf("data11 points to %p\n", data11);
            memcpy(data11, request, strlen(request));
            m->data_len = strlen(request);
            m->pkt_len = strlen(request);
            
            void *bsd_mbuf = ff_mbuf_get(NULL, (void *)m, (void*)data11, strlen(request));
            printf("The bsd_mbuf is %p\n", bsd_mbuf);
            
            char *tmp = (char *) ff_mbuf_mtod(bsd_mbuf);
            printf("tmp points to %p\n", tmp);
            
            void *temprte = ff_rte_frm_extcl(bsd_mbuf);
            printf("temprte points to %p\n", temprte);
            if (ff_write(clientfd, bsd_mbuf, strlen(request)) < 0) {
                printf("ff_write failed, errno:%d, %s\n", errno, strerror(errno));
                rte_pktmbuf_free(m);
                return -1;
            }
            request_sent = 1;    

            EV_SET(&ke, clientfd, EVFILT_READ , EV_ADD, 0, MAX_EVENTS, NULL);
            if(ff_kevent(kq, &ke, 1, NULL, 0, NULL) < 0) {
                printf("ff_kevent error:%d, %s\n", errno,
                    strerror(errno));
                return -1;
            }

        } 
        if (event.filter == EVFILT_READ && request_sent == 1 && received == 0){
            /*Allocate double pointer to pass to the ff_read*/ 
            void *ptr=NULL;  
            void **ptr1 = &ptr;
            printf("Before Read ptr points to %p\n", ptr);    
            /* Pass the **ptr to ff_read */
            ssize_t readlen = ff_read(clientfd, ptr1, nbytes);
            if (readlen < 0) {
                printf("ff_read failed, errno:%d, %s\n", errno, strerror(errno));
                return -1;
            }
            nbytes -= readlen;
            printf("nbytes is %ld\n", nbytes);
            printf("readlen is %ld\n", readlen);
            /* The pointer here points to the free bsd mbuf containing the */
            printf("After Read ptr points to %p\n", ptr);
            /* Get the pointer to data from the freebsd mbuf */
            char *data = (char *) ff_mbuf_mtod(ptr);
            printf("Daata points to %p\n", data);
            for (ssize_t i = 0; i < readlen; ++i) {
                printf("%c", data[i]);
            }
            printf("\n"); 
            if (readlen == 0){
                break;
            }
            /* Simulate the application load should be changed ... not sure if it required */
            int i, j = 0;
            for (i = 0; i < 10000; i++){
                j++;
            }            
        } 
        if (nbytes == 0){
            received = 1;
            break;
        }
        
        
    }
    return 0;
}

 

int main(int argc, char *argv[]) 
{   
    /* Initialize f-stack and dpdk */
    ff_init(argc, argv);

    /* Create a kqueue */
    kq = ff_kqueue();
    if (kq < 0) {
        printf("ff_kqueue failed, errno:%d, %s\n", errno, strerror(errno));
        return -1;
    }

    /* Create a socket for connection Request */
    sockfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ff_socket failed, errno:%d, %s\n", errno, strerror(errno));
        return -1;
    }

    /* Set non Blocking and initiate Client Connection */
    if (sockfd > 0){
        int on = 1; 
        ff_ioctl(sockfd, FIONBIO, &on);
        struct sockaddr_in my_addr;
        bzero(&my_addr, sizeof(my_addr));
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(80);
        const char *ip = "10.10.1.2";
        inet_pton(AF_INET, ip, &(my_addr.sin_addr));

        int ret = ff_connect(sockfd,(struct linux_sockaddr *)&my_addr,sizeof(struct sockaddr_in));
        if (ret < 0 && errno != EINPROGRESS && errno != EISCONN)
           printf("conn failed, sockfd = %d,ret=%d,%d,%s\n",sockfd,ret,errno,strerror(errno));
        else 
            printf("conn suc\n");
        printf("create_socket_cn sockfd = %d,ret=%d,%d,%s\n",sockfd,ret,errno,strerror(errno));
    }

    
    EV_SET(&ke, sockfd, EVFILT_WRITE , EV_ADD, 0, MAX_EVENTS, NULL);
    assert(kq  > 0);
    /* Update kqueue */
    ff_kevent(kq, &ke, 1, NULL, 0, NULL);

    ff_run(loop, NULL);

    ff_close(sockfd);

    return 0;
}
