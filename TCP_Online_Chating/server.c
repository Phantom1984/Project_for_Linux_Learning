#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define SERVERIP "127.0.0.1"
#define PORT 6789

int connfd;  // 全局连接描述符

void *send_msg(void *arg) {
    char send_buf[1024];
    while (1) {
        fgets(send_buf, sizeof(send_buf), stdin);  // 从用户输入获取数据
        send_buf[strcspn(send_buf, "\n")] = '\0';  // 删除换行符
        if (strlen(send_buf) > 0) {
            write(connfd, send_buf, strlen(send_buf));  // 发送数据
        }
    }
    return NULL;
}

void *recv_msg(void *arg) {
    char recv_buf[1024];
    int len;
    while (1) {
        len = read(connfd, recv_buf, sizeof(recv_buf) - 1);
        if (len > 0) {
            recv_buf[len] = '\0';
            printf("Received: %s\n", recv_buf);
        } else if (len == 0) {
            printf("Server disconnected.\n");
            exit(0);
        } else {
            perror("Read error");
            exit(1);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd == -1) {
        perror("Socket creation failed");
        exit(-1);
    }

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(PORT);

    if (connect(connfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(-1);
    }

    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, send_msg, NULL);
    pthread_create(&recv_thread, NULL, recv_msg, NULL);
    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    close(connfd);
    return 0;
}
