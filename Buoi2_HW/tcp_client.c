#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    char cmd[20], ip[20];
    int port;

    printf("Enter: tcp_client <IP> <PORT>\n");
    scanf("%s %s %d", cmd, ip, &port);
    getchar(); // bỏ '\n'

    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        printf("Invalid IP\n");
        return 1;
    }

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        return 1;
    }

    // Nhận lời chào
    char buf[1024];
    int n = recv(client, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        printf("Server: %s\n", buf);
    }

    // Gửi dữ liệu
    while (1) {
        printf("Enter: ");
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;

        if (send(client, buf, strlen(buf), 0) <= 0) {
            perror("send failed");
            break;
        }

        if (strncmp(buf, "exit", 4) == 0)
            break;
    }

    close(client);
    return 0;
}
