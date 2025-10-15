// broker_udp.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 6000
#define BUFFER_SIZE 1024
#define MAX_SUBS 50

typedef struct {
    struct sockaddr_in addr;
    socklen_t addr_len;
    char partido[50];
    int activo;
} Subscriber;

Subscriber subs[MAX_SUBS];

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    int n, i;

    for (i = 0; i < MAX_SUBS; i++) subs[i].activo = 0;

    // Crear socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Broker UDP escuchando en puerto %d...\n", PORT);

    while (1) {
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0,
                     (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        if (strncmp(buffer, "SUB:", 4) == 0) {
            // Nueva suscripción
            char partido[50];
            strcpy(partido, buffer + 4);
            int registrado = 0;

            for (i = 0; i < MAX_SUBS; i++) {
                if (subs[i].activo &&
                    strcmp(subs[i].partido, partido) == 0 &&
                    subs[i].addr.sin_port == cliaddr.sin_port &&
                    subs[i].addr.sin_addr.s_addr == cliaddr.sin_addr.s_addr) {
                    registrado = 1;
                    break;
                }
            }

            if (!registrado) {
                for (i = 0; i < MAX_SUBS; i++) {
                    if (!subs[i].activo) {
                        subs[i].addr = cliaddr;
                        subs[i].addr_len = len;
                        strcpy(subs[i].partido, partido);
                        subs[i].activo = 1;
                        printf("Nuevo suscriptor a %s\n", partido);
                        break;
                    }
                }
            }
        } else {
            // Mensaje de publisher
            char partido[50], evento[100];
            sscanf(buffer, "%49[^:]:%99[^\n]", partido, evento);
            printf("Broker recibió (%s): %s\n", partido, evento);

            for (i = 0; i < MAX_SUBS; i++) {
                if (subs[i].activo && strcmp(subs[i].partido, partido) == 0) {
                    sendto(sockfd, evento, strlen(evento), 0,
                           (const struct sockaddr *)&subs[i].addr, subs[i].addr_len);
                }
            }
        }
    }

    return 0;
}
