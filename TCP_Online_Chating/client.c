#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define SERVERIP "127.0.0.1"
#define PORT 6789
#define MAX_CLIENTS 128

struct sockInfo {
    int fd;
    pthread_t tid;
    struct sockaddr_in addr;
};

struct sockInfo sockinfos[MAX_CLIENTS];

void* working(void *arg) {
    struct sockInfo *pinfo = (struct sockInfo*)arg;
    char client_ip[16];
    inet_ntop(AF_INET, &pinfo->addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
    unsigned short client_port = ntohs(pinfo->addr.sin_port);
    printf("Client connected: %s:%d\n", client_ip, client_port);

    char recv_buf[1024];
    while (1) {
        int len = read(pinfo->fd, recv_buf, sizeof(recv_buf));
        if (len > 0) {
            recv_buf[len] = '\0';
            printf("Received from [%s:%d]: %s\n", client_ip, client_port, recv_buf);
            // Broadcast message to all clients
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (sockinfos[i].fd != -1 && sockinfos[i].fd != pinfo->fd) {
                    write(sockinfos[i].fd, recv_buf, strlen(recv_buf));
                }
            }
        } else if (len == 0) {
            printf("Client disconnected: %s:%d\n", client_ip, client_port);
            break;
        } else {
            perror("Read error");
            break;
        }
    }

    close(pinfo->fd);
    pinfo->fd = -1;
    pinfo->tid = -1;
    return NULL;
}

void* server_input_handler(void *arg) {
    char msg[1024];
    while (1) {
        fgets(msg, sizeof(msg), stdin);  // 从服务器控制台读取输入
        msg[strcspn(msg, "\n")] = '\0';  // 去除输入的换行符

        // 发送消息给所有连接的客户端
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (sockinfos[i].fd != -1) {
                write(sockinfos[i].fd, msg, strlen(msg));
            }
        }
    }
    return NULL;
}

int main() {
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, server_input_handler, NULL);
    pthread_detach(input_thread);
    
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("Socket creation failed");
        exit(-1);
    }

    // 初始化sockInfo数组
    for (int i = 0; i < MAX_CLIENTS; i++) {
        sockinfos[i].fd = -1;
        sockinfos[i].tid = -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(PORT);
    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(-1);
    }

    if (listen(listenfd, 10) == -1) {
        perror("Listen failed");
        exit(-1);
    }

    printf("Server is running on %s:%d\n", SERVERIP, PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (connfd == -1) {
            perror("Accept failed");
            continue;
        }

        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (sockinfos[i].fd == -1) {
                sockinfos[i].fd = connfd;
                sockinfos[i].addr = client_addr;
                pthread_create(&sockinfos[i].tid, NULL, working, &sockinfos[i]);
                pthread_detach(sockinfos[i].tid);
                break;
            }
        }
        if (i == MAX_CLIENTS) {
            char *msg = "Server full";
            write(connfd, msg, strlen(msg));
            close(connfd);
        }
    }
    close(listenfd);
    return 0;
}
