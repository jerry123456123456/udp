#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<pthread.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<sys/time.h>
#include<sys/resource.h>   //控制资源的使用时间

void createClient(int id,int myPort,int peerPort){
    int reuse=1;
    int socketFd;
    struct sockaddr_in peer_Addr;
    peer_Addr.sin_family=PF_INET;    //地址族和协议族一般是一样的，即AF_INET
    peer_Addr.sin_port=htons(peerPort);
    peer_Addr.sin_addr.s_addr=inet_addr("192.168.186.138");

    struct sockaddr_in self_Addr;
    self_Addr.sin_family=PF_INET;
    self_Addr.sin_port=htons(myPort);
    self_Addr.sin_addr.s_addr=inet_addr("0.0.0.0");

    //`SOCK_CLOEXEC` 表示在执行 `exec` 系统调用时关闭套接字，这可以在新程序中避免套接字被继承
    if((socketFd=socket(PF_INET,SOCK_DGRAM | SOCK_CLOEXEC,0))==-1){
        perror("chid socket");
        exit(1);
    }

    int opt=fcntl(socketFd,F_GETFL);    //获取socketFd文件描述符对应的标志状态
    fcntl(socketFd,F_SETFL,opt|O_NONBLOCK);   //将这个描述符对应的状态设置为非阻塞

    if(setsockopt(socketFd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))){
        exit(1);
    }
    //这段代码使用setsockopt()函数来设置套接字选项
    //SOL_SOCKET是表示所属的协议层，这里是通用套接字选项
    //SO_REUSEADDR是一个套接字选项，表示允许重用本地的地址和端口
    //&reuse是宇哥只想存储选项值的变量的指针，用来启用或禁用SO_REUSEADDR
    //sizeof(reuse) 是指定选项值的大小
    //这段代码的作用是设置套接字 `socketFd` 的 `SO_REUSEADDR` 选项，允许在套接字关闭之后立即重用本地地址和端口。这通常用于避免端口被占用而无法立即重启服务器程序的情况

    if(setsockopt(socketFd,SOL_SOCKET,SO_REUSEPORT,&reuse,sizeof(reuse))){
        exit(1);
    }

    if(bind(socketFd,(struct sockaddr*)&self_Addr,sizeof(struct sockaddr))==-1){
        perror("chid bind");
        exit(1);
    }
    
    usleep(1);  //-->key

    char buffer[1024]={0};
    memset(buffer,0,1024);
    sprintf(buffer,"hello %d",id);
    sendto(socketFd,buffer,strlen(buffer),0,(struct sockaddr*)&peer_Addr,sizeof(struct sockaddr_in));
}

void serial(int clinetNum){
    for(int i=1;i<=clinetNum;i++){
        createClient(i,2025+i,1234);
    }
}

int main(int argc,char *argv[]){
    serial(1024);
    printf("serial success\n");
    return 0;
}