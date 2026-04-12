#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9000
#define MAX_CLIENTS 100
#define BUF_SIZE 256

typedef struct {
    int fd;
    int state; // 0 user, 1 pass, 2 logged
    char user[32];
} Client;

Client clients[MAX_CLIENTS];

int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[32], p[32];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (!strcmp(u, user) && !strcmp(p, pass)) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void send_file(int fd) {
    FILE *f = fopen("out.txt", "r");
    if (!f) return;

    char buf[BUF_SIZE];
    while (fgets(buf, sizeof(buf), f)) {
        send(fd, buf, strlen(buf), 0);
    }
    fclose(f);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);

    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    fd_set fdread;
    struct timeval tv;

    printf("Telnet server running...\n");

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);

        int maxfd = listener;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &fdread);
                if (clients[i].fd > maxfd)
                    maxfd = clients[i].fd;
            }
        }

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int ret = select(maxfd + 1, &fdread, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            break;
        }

        if (ret == 0) continue;

        if (FD_ISSET(listener, &fdread)) {
            int client = accept(listener, NULL, NULL);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = client;
                    clients[i].state = 0;
                    send(client, "Username: ", 10, 0);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i].fd;
            if (fd == -1) continue;

            if (FD_ISSET(fd, &fdread)) {
                char buf[BUF_SIZE];
                int len = recv(fd, buf, sizeof(buf)-1, 0);

                if (len <= 0) {
                    close(fd);
                    clients[i].fd = -1;
                    continue;
                }

                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                if (clients[i].state == 0) {
                    strcpy(clients[i].user, buf);
                    clients[i].state = 1;
                    send(fd, "Password: ", 10, 0);
                }
                else if (clients[i].state == 1) {
                    if (check_login(clients[i].user, buf)) {
                        clients[i].state = 2;
                        send(fd, "OK\n> ", 5, 0);
                    } else {
                        send(fd, "Fail\nUsername: ", 16, 0);
                        clients[i].state = 0;
                    }
                }
                else {
                    char cmd[256];
                    snprintf(cmd, sizeof(cmd), "%s > out.txt", buf);

                    system(cmd);
                    send_file(fd);

                    send(fd, "\n> ", 3, 0);
                }
            }
        }
    }
}