#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

typedef struct {
    char mssv[20];
    char name[50];
    char dob[20];
    float gpa;
} Student;

int main() {
    char cmd[20], logfile[100];
    int port;

    printf("Enter: sv_server <PORT> <LOG_FILE>\n");
    scanf("%s %d %s", cmd, &port, logfile);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server listening on port %d...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int client = accept(server, (struct sockaddr *)&client_addr, &len);
        if (client < 0) continue;

        Student sv;
        int n = recv(client, &sv, sizeof(sv), 0);
        if (n <= 0) {
            close(client);
            continue;
        }

        // Lấy IP
        char *ip = inet_ntoa(client_addr.sin_addr);

        // Lấy thời gian
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char time_str[50];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

        // In ra màn hình
        printf("%s %s %s %s %s %.2f\n",
               ip, time_str,
               sv.mssv, sv.name, sv.dob, sv.gpa);

        // Ghi file
        FILE *f = fopen(logfile, "a");
        if (f != NULL) {
            fprintf(f, "%s %s %s %s %s %.2f\n",
                    ip, time_str,
                    sv.mssv, sv.name, sv.dob, sv.gpa);
            fclose(f);
        }

        close(client);
    }

    close(server);
    return 0;
}
