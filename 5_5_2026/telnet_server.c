/*******************************************************************************
 * @file    telnet_server_fork.c
 * @brief   Telnet server multiprocessing (fork)
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 9000
#define BUF_SIZE 1024

typedef struct {
    int state;          // 0=user, 1=pass, 2=logged
    char user[32];
} Session;


/* ======================= CHECK LOGIN ======================= */

int check_login(char *user, char *pass) {

    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[32], p[32];

    while (fscanf(f, "%s %s", u, p) != EOF) {

        if (strcmp(user, u) == 0 &&
            strcmp(pass, p) == 0) {

            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}


/* ======================= SEND FILE ======================= */

void send_file(int fd) {

    FILE *f = fopen("out.txt", "r");
    if (!f) return;

    char buf[BUF_SIZE];

    while (fgets(buf, sizeof(buf), f)) {
        send(fd, buf, strlen(buf), 0);
    }

    fclose(f);
}


/* ======================= HANDLE CLIENT ======================= */

void handle_client(int client) {

    Session s = {0};

    char buf[BUF_SIZE];

    send(client, "Username: ", 10, 0);

    while (1) {

        memset(buf, 0, sizeof(buf));

        int len = recv(client, buf, sizeof(buf)-1, 0);

        if (len <= 0) {
            break;
        }

        buf[len] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        /* ================= USER ================= */

        if (s.state == 0) {

            strcpy(s.user, buf);

            s.state = 1;

            send(client, "Password: ", 10, 0);
        }

        /* ================= PASS ================= */

        else if (s.state == 1) {

            if (check_login(s.user, buf)) {

                s.state = 2;

                send(client, "Login success\n> ", 16, 0);
            }
            else {

                send(client,
                     "Login failed\nUsername: ",
                     25,
                     0);

                s.state = 0;
            }
        }

        /* ================= COMMAND ================= */

        else {

            if (strcmp(buf, "exit") == 0) {

                send(client, "Bye\n", 4, 0);

                break;
            }

            char cmd[512];

            snprintf(cmd,
                     sizeof(cmd),
                     "%s > out.txt",
                     buf);

            system(cmd);

            send_file(client);

            send(client, "\n> ", 3, 0);
        }
    }

    close(client);

    printf("Client disconnected\n");

    exit(0);
}


/* ======================= MAIN ======================= */

int main() {

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

    printf("Telnet server listening on port %d...\n", PORT);

    while (1) {

        int client = accept(listener, NULL, NULL);

        if (client < 0) {
            continue;
        }

        printf("Client connected: %d\n", client);

        pid_t pid = fork();

        /* ================= CHILD ================= */

        if (pid == 0) {

            close(listener);

            handle_client(client);
        }

        /* ================= PARENT ================= */

        else if (pid > 0) {

            close(client);

            // delete zombie process
            waitpid(-1, NULL, WNOHANG);
        }

        else {

            perror("fork");
            close(client);
        }
    }

    close(listener);

    return 0;
}