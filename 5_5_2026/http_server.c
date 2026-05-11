/*******************************************************************************
 * @file    http_server.c
 * @brief   HTTP server preforking 
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 8080
#define NUM_PROCESSES 4
#define BUF_SIZE 4096

int main() {

    /* ================= SOCKET ================= */

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* ================= BIND ================= */

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {

        perror("bind");
        return 1;
    }

    /* ================= LISTEN ================= */

    if (listen(listener, 10) < 0) {
        perror("listen");
        return 1;
    }

    printf("HTTP server listening on port %d...\n", PORT);

    /* ================= PREFORK ================= */

    for (int i = 0; i < NUM_PROCESSES; i++) {

        pid_t pid = fork();

        /* ================= CHILD ================= */

        if (pid == 0) {

            char buf[BUF_SIZE];

            while (1) {

                /* ===== accept ===== */

                int client = accept(listener, NULL, NULL);

                if (client < 0)
                    continue;

                printf("[PID %d] Client connected: %d\n", getpid(), client);

                /* ===== recv ===== */

                memset(buf, 0, sizeof(buf));

                int ret = recv(client, buf, sizeof(buf)-10, 0);

                if (ret <= 0) {
                    close(client);
                    continue;
                }

                buf[ret] = 0;

                printf("\n========== HTTP REQUEST ==========\n");
                puts(buf);

                /* ===== response ===== */

                char *msg =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "\r\n"
                    "<html>"
                    "<body>"
                    "<h1>Hello from HTTP Server</h1>"
                    "<p>This server uses preforking.</p>"
                    "</body>"
                    "</html>";

                send(client, msg, strlen(msg), 0);

                /* ===== close ===== */

                close(client);

                printf("[PID %d] Client disconnected\n\n",
                       getpid());
            }
        }
    }

    /* ================= PARENT ================= */

    while (1) {
        wait(NULL);
    }

    close(listener);

    return 0;
}