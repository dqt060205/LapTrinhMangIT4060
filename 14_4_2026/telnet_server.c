/*******************************************************************************
 * Telnet server using poll()
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 9000
#define MAX_CLIENTS 100
#define BUF_SIZE 256

typedef struct {
    int fd;
    int state; // 0 user, 1 pass, 2 logged
    char user[32];
} Client;

Client clients[MAX_CLIENTS];
struct pollfd fds[MAX_CLIENTS + 1]; // + listener

int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[32], p[32];
    while (fscanf(f, "%s %s", u, p) == 2) {
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
    int n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        send(fd, buf, n, 0);
    }
    fclose(f);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);

    printf("Telnet poll server running...\n");

    // init
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;
    int nfds = 1;

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) break;

        // new connection
        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = client;
                    clients[i].state = 0;

                    fds[nfds].fd = client;
                    fds[nfds].events = POLLIN;
                    nfds++;

                    send(client, "Username: ", 10, 0);
                    break;
                }
            }
        }

        // handle clients
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {

                int fd = fds[i].fd;
                char buf[BUF_SIZE];

                int len = recv(fd, buf, sizeof(buf)-1, 0);

                if (len <= 0) {
                    close(fd);

                    // remove
                    fds[i] = fds[nfds - 1];
                    nfds--;

                    // remove client
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == fd) {
                            clients[j].fd = -1;
                            break;
                        }
                    }

                    i--;
                    continue;
                }

                buf[len] = 0;

                buf[strcspn(buf, "\r\n")] = 0;
                for (int k = strlen(buf)-1; k >= 0 && buf[k] < 32; k--)
                    buf[k] = 0;

                Client *c = NULL;
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].fd == fd) {
                        c = &clients[j];
                        break;
                    }
                }

                if (!c) continue;

                if (c->state == 0) {
                    strcpy(c->user, buf);
                    c->state = 1;
                    send(fd, "Password: ", 10, 0);
                }
                else if (c->state == 1) {
                    if (check_login(c->user, buf)) {
                        c->state = 2;
                        send(fd, "OK\n> ", 5, 0);
                    } else {
                        send(fd, "Fail\nUsername: ", 16, 0);
                        c->state = 0;
                    }
                }
                else {
                    printf("CMD = [%s]\n", buf);

                    char cmd[256];
                    snprintf(cmd, sizeof(cmd), "%s > out.txt", buf);

                    system(cmd);
                    usleep(100000);

                    send_file(fd);
                    send(fd, "\n> ", 3, 0);
                }
            }
        }
    }
}