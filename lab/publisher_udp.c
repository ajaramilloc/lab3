// publisher_auto.c - Publisher no interactivo para testing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

#define PORT 6000
#define BUFFER_SIZE 1024

long long current_timestamp_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <partido> <publisher_id> <num_mensajes>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s C_vs_D 1 10\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *partido = argv[1];
    int pub_id = atoi(argv[2]);
    int num_msgs = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Publisher %d iniciado para partido: %s\n", pub_id, partido);
    printf("Enviando %d mensajes...\n", num_msgs);

    for (int i = 1; i <= num_msgs; i++) {
        long long timestamp = current_timestamp_ms();
        snprintf(buffer, BUFFER_SIZE, 
                "%s:MSG#%d [PUB-%d] [TS:%lld] Evento del partido %s",
                partido, i, pub_id, timestamp, partido);
        
        sendto(sockfd, buffer, strlen(buffer), 0,
               (const struct sockaddr *)&servaddr, sizeof(servaddr));
        
        printf("Enviado: %s\n", buffer);
        fflush(stdout);
        
        usleep(100000); // 100ms entre mensajes
    }

    printf("Publisher %d terminó de enviar %d mensajes\n", pub_id, num_msgs);
    
    close(sockfd);
    return 0;
}