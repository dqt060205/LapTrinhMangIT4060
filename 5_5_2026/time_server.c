/*******************************************************************************
 * @file    time_server.c
 * @brief   Time server multiprocessing (fork)
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define PORT 9000
#define BUF_SIZE 1024


/* ======================= FORMAT TIME ======================= */

void get_time_string(char *format,
                     char *out,
                     int out_size) {

    time_t now = time(NULL);

    struct tm *t = localtime(&now);

    if (strcmp(format, "dd/mm/yyyy") == 0) {

        strftime(out, out_size, "%d/%m/%Y", t);
    }

    else if (strcmp(format, "dd/mm/yy") == 0) {

        strftime(out,out_size,"%d/%m/%y", t);
    }

    else if (strcmp(format, "mm/dd/yyyy") == 0) {

        strftime(out, out_size, "%m/%d/%Y", t);
    }

    else if (strcmp(format, "mm/dd/yy") == 0) {

        strftime(out, out_size, "%m/%d/%y", t);
    }

    else {

        strcpy(out, "Invalid format");
    }
}


/* ======================= HANDLE CLIENT ======================= */

void handle_client(int client) {

    char buf[BUF_SIZE];

    while (1) {

        memset(buf, 0, sizeof(buf));

        int len = recv(client, buf, sizeof(buf)-1, 0);

        if (len <= 0)
            break;

        buf[len] = 0;

        buf[strcspn(buf, "\r\n")] = 0;

        char *cmd = strtok(buf, " ");
        char *format = strtok(NULL, " ");

        if (cmd == NULL || format == NULL) {

            send(client, "Invalid command\n", 16, 0);

            continue;
        }

        if (strcmp(cmd, "GET_TIME") != 0) {

            send(client, "Invalid command\n", 16, 0);

            continue;
        }
        /* ================= GET TIME ================= */

        char result[128];

        get_time_string(format, result, sizeof(result));

        strcat(result, "\n");

        send(client, result, strlen(result), 0);
    }

    close(client);

    printf("Client disconnected: %d\n", client);

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

    printf("Time server listening on port %d...\n", PORT);

    /* ================= ACCEPT LOOP ================= */

    while (1) {

        int client = accept(listener, NULL, NULL);

        if (client < 0)
            continue;

        printf("Client connected: %d\n", client);

        pid_t pid = fork();

        /* ================= CHILD ================= */

        if (pid == 0) {

            close(listener);

            printf("[PID %d] Handling client %d\n",
                getpid(),
                client);

            handle_client(client);
        }

        /* ================= PARENT ================= */

        else if (pid > 0) {

            close(client);

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