/*******************************************************************************
 * @file    chat_server.c
 * @brief   Chat server dùng poll()
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <time.h>

#define MAX_CLIENTS 1000
#define BUF_SIZE 256

typedef struct {
    int fd;
    char id[32];
    char name[32];
    int registered; // 0: chưa nhập id, 1: đã nhập
} client_t;

int main() {
    
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);
    
    printf("Server listening on 8080...\n");
    
    struct pollfd fds[MAX_CLIENTS];
    client_t clients[MAX_CLIENTS];

    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    char buf[BUF_SIZE];

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll()");
            break;
        }

        // Listener
        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);

            if (nfds < MAX_CLIENTS) {
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;

                clients[nfds].fd = client;
                clients[nfds].registered = 0;

                printf("New client: %d\n", client);
                send(client, "Enter: client_id: client_name\n", 32, 0);

                nfds++;
            } else {
                close(client);
            }
        }

        // Client sockets
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {

                int len = recv(fds[i].fd, buf, sizeof(buf)-1, 0);

                if (len <= 0) {
                    printf("Client %d disconnected\n", fds[i].fd);
                    close(fds[i].fd);

                    // remove khỏi mảng
                    fds[i] = fds[nfds - 1];
                    clients[i] = clients[nfds - 1];
                    nfds--;
                    i--;
                    continue;
                }

                buf[len] = 0;

                // chưa đăng ký
                if (!clients[i].registered) {
                    char id[32], name[32];

                    if (sscanf(buf, "%[^:]: %s", id, name) == 2) {
                        strcpy(clients[i].id, id);
                        strcpy(clients[i].name, name);
                        clients[i].registered = 1;

                        printf("Client %d registered: %s (%s)\n",
                               fds[i].fd, id, name);

                        send(fds[i].fd, "OK\n", 3, 0);
                    } else {
                        send(fds[i].fd,
                             "Wrong format. Use: client_id: client_name\n",
                             48, 0);
                    }
                    continue;
                }

                // Broadcast
                char msg[512];

                // lấy timestamp
                time_t now = time(NULL);
                struct tm *t = localtime(&now);

                char timebuf[64];
                strftime(timebuf, sizeof(timebuf),
                         "%Y/%m/%d %H:%M:%S", t);

                snprintf(msg, sizeof(msg),
                         "%s %s: %s",
                         timebuf,
                         clients[i].id,
                         buf);

                printf("%s", msg);

                // gửi cho client khác
                for (int j = 1; j < nfds; j++) {
                    if (j != i && clients[j].registered) {
                        send(fds[j].fd, msg, strlen(msg), 0);
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}