#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s port_s ip_d port_d\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in addr_s, addr_d;
    char buffer[BUF_SIZE];

    //socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    
    memset(&addr_s, 0, sizeof(addr_s));
    addr_s.sin_family = AF_INET;
    addr_s.sin_addr.s_addr = INADDR_ANY;
    addr_s.sin_port = htons(port_s);

    if (bind(sockfd, (struct sockaddr *)&addr_s, sizeof(addr_s)) < 0) {
        perror("bind");
        return 1;
    }

   
    memset(&addr_d, 0, sizeof(addr_d));
    addr_d.sin_family = AF_INET;
    addr_d.sin_port = htons(port_d);
    inet_pton(AF_INET, ip_d, &addr_d.sin_addr);

    printf("UDP chat started on port %d...\n", port_s);

    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);         // stdin
        FD_SET(sockfd, &readfds);   // socket

        int maxfd = sockfd;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(0, &readfds)) {
            memset(buffer, 0, BUF_SIZE);
            fgets(buffer, BUF_SIZE, stdin);

            sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&addr_d, sizeof(addr_d));
        }

        if (FD_ISSET(sockfd, &readfds)) {
            struct sockaddr_in sender;
            socklen_t sender_len = sizeof(sender);

            memset(buffer, 0, BUF_SIZE);
            int n = recvfrom(sockfd, buffer, BUF_SIZE, 0,
                             (struct sockaddr *)&sender, &sender_len);

            if (n > 0) {
                printf("Received: %s", buffer);
            }
        }
    }

    close(sockfd);
    return 0;
}