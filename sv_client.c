#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
    char mssv[20];
    char name[50];
    char dob[20];
    float gpa;
} Student;

int main() {
    char cmd[20], ip[20];
    int port;

    printf("Enter: sv_client <IP> <PORT>\n");
    scanf("%s %s %d", cmd, ip, &port);
    getchar();

    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        return 1;
    }

    Student sv;

    printf("MSSV: ");
    fgets(sv.mssv, sizeof(sv.mssv), stdin);
    sv.mssv[strcspn(sv.mssv, "\n")] = 0;

    printf("Ho ten: ");
    fgets(sv.name, sizeof(sv.name), stdin);
    sv.name[strcspn(sv.name, "\n")] = 0;

    printf("Ngay sinh (YYYY-MM-DD): ");
    fgets(sv.dob, sizeof(sv.dob), stdin);
    sv.dob[strcspn(sv.dob, "\n")] = 0;

    printf("GPA: ");
    scanf("%f", &sv.gpa);

    // Gửi struct
    send(client, &sv, sizeof(sv), 0);

    close(client);
    return 0;
}