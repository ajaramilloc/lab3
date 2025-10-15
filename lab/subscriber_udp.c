// subscriber_udp.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 6000
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char buffer[BUFFER_SIZE];
    char partido[50];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Ingrese partido a seguir (ej: A_vs_B): ");
    fgets(partido, sizeof(partido), stdin);
    partido[strcspn(partido, "\n")] = 0;

    char subMsg[BUFFER_SIZE];
    sprintf(subMsg, "SUB:%s", partido);

    sendto(sockfd, subMsg, strlen(subMsg), 0,
           (const struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("Suscrito a %s. Esperando eventos...\n", partido);

    while (1) {
        len = sizeof(cliaddr);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';
        printf("[EVENTO] %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
