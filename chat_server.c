#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

#define PORT 9000
#define MAX_CLIENTS 100
#define BUF_SIZE 512

typedef struct {
    int fd;
    int registered;
    char id[32];
} Client;

Client clients[MAX_CLIENTS];

void get_time_str(char *buf, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, size, "%Y/%m/%d %H:%M:%S", t);
}

void broadcast(int sender, char *msg) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && clients[i].fd != sender) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
}

void remove_newline(char *s) {
    s[strcspn(s, "\r\n")] = 0;
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

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listener, 10) < 0) {
        perror("listen");
        return 1;
    }

    // init clients
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    fd_set fdread;
    struct timeval tv;

    printf("Chat server running on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        FD_SET(STDIN_FILENO, &fdread);

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

        if (ret == 0) {
            // timeout (optional log)
            continue;
        }

        // ===== Server input (debug) =====
        if (FD_ISSET(STDIN_FILENO, &fdread)) {
            char tmp[BUF_SIZE];
            fgets(tmp, sizeof(tmp), stdin);

            char timebuf[64];
            get_time_str(timebuf, sizeof(timebuf));

            char out[BUF_SIZE];
            snprintf(out, sizeof(out), "%s SERVER: %s", timebuf, tmp);

            broadcast(-1, out);
        }

        // ===== New client =====
        if (FD_ISSET(listener, &fdread)) {
            int client = accept(listener, NULL, NULL);
            if (client < 0) {
                perror("accept");
                continue;
            }

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = client;
                    clients[i].registered = 0;

                    send(client, "Enter id: ", 10, 0);
                    break;
                }
            }
        }

        // ===== Handle clients =====
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i].fd;
            if (fd == -1) continue;

            if (FD_ISSET(fd, &fdread)) {
                char buf[BUF_SIZE];
                int len = recv(fd, buf, sizeof(buf)-1, 0);

                if (len <= 0) {
                    // client disconnect
                    char timebuf[64], out[BUF_SIZE];
                    get_time_str(timebuf, sizeof(timebuf));

                    if (clients[i].registered) {
                        snprintf(out, sizeof(out),
                                 "%s %s left\n",
                                 timebuf, clients[i].id);
                        broadcast(fd, out);
                    }

                    close(fd);
                    clients[i].fd = -1;
                    continue;
                }

                buf[len] = 0;
                remove_newline(buf);

                // ===== Register =====
                if (!clients[i].registered) {
                    if (sscanf(buf, "%[^:]:", clients[i].id) == 1) {
                        clients[i].registered = 1;

                        send(fd, "Registered OK\n", 14, 0);

                        // notify others
                        char timebuf[64], out[BUF_SIZE];
                        get_time_str(timebuf, sizeof(timebuf));

                        snprintf(out, sizeof(out),
                                 "%s %s joined\n",
                                 timebuf, clients[i].id);

                        broadcast(fd, out);
                    } else {
                        send(fd, "Wrong format!\n", 14, 0);
                    }
                }
                // ===== Chat =====
                else {
                    char timebuf[64], out[BUF_SIZE];
                    get_time_str(timebuf, sizeof(timebuf));

                    snprintf(out, sizeof(out),
                             "%s %s: %s\n",
                             timebuf,
                             clients[i].id,
                             buf);

                    broadcast(fd, out);
                }
            }
        }
    }

    close(listener);
    return 0;
}