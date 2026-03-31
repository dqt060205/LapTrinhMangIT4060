#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>

#define MAX_CLIENTS 64

typedef struct {
    int fd;
    int state; // 0: ask name, 1: ask id
    char name[128];
    char mssv[64];
} Client;

Client clients[MAX_CLIENTS];
int nclients = 0;

void build_email(char *name, char *mssv, char *email) {
    char temp[128];
    strcpy(temp, name);

    char *tokens[20];
    int n = 0;

    char *p = strtok(temp, " ");
    while (p) {
        tokens[n++] = p;
        p = strtok(NULL, " ");
    }

    // Ten
    char ten[64];
    strcpy(ten, tokens[n - 1]);

    // H (initials)
    char H[32] = "";
    for (int i = 0; i < n - 1; i++) {
        int len = strlen(H);
        H[len] = tokens[i][0];
        H[len + 1] = 0;
    }

    // MSSV_trimmed
    char *trimmed = mssv + 2;

    sprintf(email, "%s.%s%s@sis.hust.edu.vn", ten, H, trimmed);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Server listening 8080...\n");

    char buf[256];
    int len;

    while (1) {
        // Accept (non-blocking)
        int client = accept(listener, NULL, NULL);
        if (client != -1) {
            printf("New client %d\n", client);

            ioctl(client, FIONBIO, &ul);

            clients[nclients].fd = client;
            clients[nclients].state = 0;
            memset(clients[nclients].name, 0, sizeof(clients[nclients].name));
            memset(clients[nclients].mssv, 0, sizeof(clients[nclients].mssv));

            send(client, "Ho ten: ", 8, 0);

            nclients++;
        }

        // Handle clients
        for (int i = 0; i < nclients; i++) {
            len = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);

            if (len == -1) {
                if (errno == EWOULDBLOCK)
                    continue;
                else
                    continue;
            }

            if (len == 0)
                continue;

            buf[len] = 0;

            // Remove newline
            buf[strcspn(buf, "\r\n")] = 0;

            if (clients[i].state == 0) {
                strcpy(clients[i].name, buf);
                clients[i].state = 1;

                send(clients[i].fd, "MSSV: ", 6, 0);
            }
            else if (clients[i].state == 1) {
                strcpy(clients[i].mssv, buf);

                char email[256];
                build_email(clients[i].name, clients[i].mssv, email);

                char output[300];
                sprintf(output, "Email: %s\n", email);
                send(clients[i].fd, output, strlen(output), 0);

                printf("Client %d -> %s\n", clients[i].fd, email);

                clients[i].state = 2; // done
            }
        }
    }

    return 0;
}
