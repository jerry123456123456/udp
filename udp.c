#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // 包含此头文件
		       //
#define PORT 5005

void *udp_server(void *arg) {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));  //用于将servaddr清零
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    printf("UDP server started\n");

    while (1) {
        socklen_t len = sizeof(cliaddr); 
        int n = recvfrom(sockfd, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);  //从sockfd中阻塞知道有数据发来，接收的数据存在buffer中，cliaddr用于存储发送端的信息
        buffer[n] = '\0';
        printf("Received message: %s\n", buffer);
    }

    close(sockfd);
    return NULL;
}

void *udp_client(void *arg) {
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.186.138"); //也可以使用本地回环地址127.0.0.1
    servaddr.sin_port = htons(PORT);

    while (1) {
        printf("Enter message to send: ");
        fgets(buffer, 1024, stdin);
        sendto(sockfd, buffer, strlen(buffer), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }

    close(sockfd);
    return NULL;
}

int main() {
    pthread_t server_thread, client_thread;

    pthread_create(&server_thread, NULL, udp_server, NULL);
    pthread_create(&client_thread, NULL, udp_client, NULL);

    pthread_join(server_thread, NULL);
    pthread_join(client_thread, NULL);

    return 0;
}
