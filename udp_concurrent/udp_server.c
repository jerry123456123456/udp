#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <assert.h>

#define SO_REUSEPORT    15

#define MAXBUF 10240
#define MAXEPOLLSIZE 100

int flag = 0;
int count = 0;

int read_data(int sd){
    char recvbuf[MAXBUF+1];
    int ret;
    struct sockaddr_in client_addr;
    socklen_t cli_len=sizeof(client_addr);

    bzero(recvbuf,MAXBUF);

    ret=recvfrom(sd,recvbuf,MAXBUF,0,(struct sockaddr*)&client_addr,&cli_len);
    if(ret>0){
        printf("read[%d]: %s from %d\n",ret,recvbuf,sd);
    }else{
        printf("read err: %s  %d\n",strerror(errno),ret);
    }
    return 0;
}

int udp_accept(int sd,struct sockaddr_in my_addr){
    int new_sd=-1;
    int ret=0;
    int reuse=1;
    char buf[16]={0};
    struct sockaddr_in peer_addr;
    socklen_t cli_len=sizeof(peer_addr);

    ret=recvfrom(sd,buf,16,0,(struct sockaddr*)&peer_addr,&cli_len);   //将发送方的地址存在peer_addr中
    if(ret<0){
        return -1;
    }

    if((new_sd=socket(PF_INET,SOCK_DGRAM,0))==-1){
        perror("child socket");
        exit(1);
    }else{
        printf("%d, parent:%d  new:%d\n",count++, sd, new_sd); //1023
    }

    ret=setsockopt(new_sd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    if(ret){
        exit(1);
    }

    ret = setsockopt(new_sd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }

    ret=bind(new_sd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr));
    if (ret){
        perror("chid bind");
        exit(1);
    }

    peer_addr.sin_family=PF_INET;
    if(connect(new_sd,(struct sockaddr*)&peer_addr,sizeof(struct sockaddr))==-1){
        perror("chid connect");
        exit(1);
    }

    out:
        return new_sd;
}

int main(int argc,char *argv[]){
    int listener,kdpfd,nfds,n,curfds;
    socklen_t len;
    struct sockaddr_in my_addr,their_addr;
    unsigned int port;
    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];
    int opt=1;
    int ret=0;

    port=1234;

    if((listener=socket(PF_INET,SOCK_DGRAM,0))==-1){
        perror("SOCKET");
        exit(1);
    }else{
        printf("SOCKET OK\n");
    }

    ret = setsockopt(listener,SOL_SOCKET,SO_REUSEPORT,&opt,sizeof(opt));  //设置端口复用
    if(ret){
        exit(1);
    }

    int flags=fcntl(listener,F_GETFL,0);
    flags |=O_NONBLOCK;
    fcntl(listener,F_SETFL,flags);

    bzero(&my_addr,sizeof(my_addr));
    my_addr.sin_family=PF_INET;
    my_addr.sin_port=htons(port);
    my_addr.sin_addr.s_addr=INADDR_ANY;
    if(bind(listener,(struct sockaddr*)&my_addr,sizeof(struct sockaddr))==-1){
        perror("bind");
        exit(1);
    }else{
        printf("IP bind ok");
    }

    kdpfd=epoll_create(MAXEPOLLSIZE);
    ev.events=EPOLLIN | EPOLLET;
    ev.data.fd=listener;

    if(epoll_ctl(kdpfd,EPOLL_CTL_ADD,listener,&ev)<0){
        fprintf(stderr, "epoll set insertion error: fd=%dn", listener);
        return -1;
    }else{
        printf("ep add ok\n");
    }

    while(1){
        nfds=epoll_wait(kdpfd,events,10000,-1);
        if(nfds==-1){
            perror("epoll_wait");
            break;
        }

        for(n=0;n<nfds;n++){
            if(events[n].data.fd==listener){
                int new_sd;
                struct epoll_event child_ev;
                while(1){
                    new_sd=udp_accept(listener,my_addr);
                    if(new_sd==-1)break;
                    child_ev.events=EPOLLIN;
                    child_ev.data.fd=new_sd;
                    if(epoll_ctl(kdpfd,EPOLL_CTL_ADD,new_sd,&child_ev)<0){
                        fprintf(stderr, "epoll set insertion error: fd=%dn", new_sd);
	                    return -1;
                    }
                }
            }else{
                read_data(events[n].data.fd);
            }
        }
    }

    return 0;
}

