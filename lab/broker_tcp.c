// broker_tcp.c (Versión corregida)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 5000
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

typedef struct {
    int sock;
    char partido[50];
} Subscriber;

Subscriber subscribers[MAX_CLIENTS];

int main() {
    int server_fd, new_socket, max_sd, activity, i, valread;
    int client_socket[MAX_CLIENTS] = {0};
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    socklen_t addrlen = sizeof(address);

    // Inicializar lista de subs
    for (i = 0; i < MAX_CLIENTS; i++) {
        subscribers[i].sock = 0;
        strcpy(subscribers[i].partido, "");
    }

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Permitir reutilizar el puerto
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Escuchar
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Broker TCP escuchando en puerto %d...\n", PORT);
    fflush(stdout);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = subscribers[i].sock;
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("Nueva conexión, socket fd: %d\n", new_socket);
            fflush(stdout);

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (subscribers[i].sock == 0) {
                    subscribers[i].sock = new_socket;
                    break;
                }
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = subscribers[i].sock;
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);  // Limpiar buffer
                
                if ((valread = read(sd, buffer, BUFFER_SIZE - 1)) <= 0) {
                    // Cliente desconectado
                    printf("Cliente %d desconectado\n", sd);
                    fflush(stdout);
                    close(sd);
                    subscribers[i].sock = 0;
                    strcpy(subscribers[i].partido, "");
                } else {
                    buffer[valread] = '\0';
                    
                    // Eliminar saltos de línea
                    char *newline = strchr(buffer, '\n');
                    if (newline) *newline = '\0';
                    newline = strchr(buffer, '\r');
                    if (newline) *newline = '\0';

                    // Si empieza con "SUB:", es suscripción
                    if (strncmp(buffer, "SUB:", 4) == 0) {
                        strncpy(subscribers[i].partido, buffer + 4, sizeof(subscribers[i].partido) - 1);
                        subscribers[i].partido[sizeof(subscribers[i].partido) - 1] = '\0';
                        printf("Cliente %d suscrito a '%s'\n", sd, subscribers[i].partido);
                        fflush(stdout);
                    } else {
                        // Es un mensaje de publisher
                        printf("Broker recibió: %s\n", buffer);
                        fflush(stdout);
                        
                        char partido[50], evento[200];
                        memset(partido, 0, sizeof(partido));
                        memset(evento, 0, sizeof(evento));
                        
                        // Parsear mensaje partido:evento
                        char *colon = strchr(buffer, ':');
                        if (colon != NULL) {
                            int partido_len = colon - buffer;
                            if (partido_len < sizeof(partido)) {
                                strncpy(partido, buffer, partido_len);
                                partido[partido_len] = '\0';
                                
                                strncpy(evento, colon + 1, sizeof(evento) - 1);
                                evento[sizeof(evento) - 1] = '\0';

                                // Reenviar a suscriptores del partido
                                int sent_count = 0;
                                for (int j = 0; j < MAX_CLIENTS; j++) {
                                    if (subscribers[j].sock != 0 && 
                                        strcmp(subscribers[j].partido, partido) == 0) {
                                        
                                        // Añadir salto de línea al final
                                        char mensaje_con_newline[BUFFER_SIZE];
                                        snprintf(mensaje_con_newline, sizeof(mensaje_con_newline), 
                                                "%s\n", evento);
                                        
                                        int sent = send(subscribers[j].sock, mensaje_con_newline, 
                                                      strlen(mensaje_con_newline), 0);
                                        if (sent > 0) {
                                            sent_count++;
                                        }
                                    }
                                }
                                printf("Mensaje enviado a %d suscriptores de '%s'\n", sent_count, partido);
                                fflush(stdout);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}