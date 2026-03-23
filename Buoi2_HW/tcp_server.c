#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    char cmd[20], welcome_file[100], output_file[100];
    int port;

    printf("Enter: tcp_server <PORT> <WELCOME_FILE> <OUTPUT_FILE>\n");
    scanf("%s %d %s %s", cmd, &port, welcome_file, output_file);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    if (listen(server, 5) < 0) {
        perror("listen failed");
        return 1;
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        int client = accept(server, NULL, NULL);
        if (client < 0) {
            perror("accept failed");
            continue;
        }

        printf("Client connected\n");

        // Gửi file chào
        FILE *f = fopen(welcome_file, "r");
        if (f != NULL) {
            char buf[1024];
            int len = fread(buf, 1, sizeof(buf), f);
            send(client, buf, len, 0);
            fclose(f);
        }

        // Ghi dữ liệu client vào file
        FILE *out = fopen(output_file, "a");
        if (out == NULL) {
            perror("open output file failed");
            close(client);
            continue;
        }

        char buf[1024];
        int n;
        while ((n = recv(client, buf, sizeof(buf), 0)) > 0) {
            fwrite(buf, 1, n, out);
        }

        fclose(out);
        close(client);
        printf("Client disconnected\n");
    }

    close(server);
    return 0;
}
