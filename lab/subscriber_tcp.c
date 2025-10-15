// subscriber_tcp.c (Versión corregida)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define BUFFER_SIZE 4096
#define LINE_BUFFER_SIZE 8192

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char line_buffer[LINE_BUFFER_SIZE] = {0};  // Buffer para acumular líneas
    int line_pos = 0;
    char partido[50];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error creando socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Dirección inválida\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Conexión fallida\n");
        return -1;
    }

    printf("Ingrese partido a seguir (ej: A_vs_B): ");
    fgets(partido, sizeof(partido), stdin);
    partido[strcspn(partido, "\n")] = 0;

    char subMsg[BUFFER_SIZE];
    sprintf(subMsg, "SUB:%s", partido);
    send(sock, subMsg, strlen(subMsg), 0);

    printf("Suscrito a %s. Esperando eventos...\n", partido);
    fflush(stdout);

    while (1) {
        int valread = read(sock, buffer, BUFFER_SIZE - 1);
        if (valread <= 0) {
            printf("Conexión cerrada por el servidor\n");
            break;
        }
        
        buffer[valread] = '\0';
        
        // Agregar datos al buffer de línea
        for (int i = 0; i < valread && line_pos < LINE_BUFFER_SIZE - 1; i++) {
            if (buffer[i] == '\n') {
                // Fin de línea encontrado
                line_buffer[line_pos] = '\0';
                
                // Procesar la línea solo si no está vacía
                if (line_pos > 0) {
                    printf("[EVENTO] %s\n", line_buffer);
                    fflush(stdout);
                }
                
                // Resetear buffer de línea
                line_pos = 0;
                memset(line_buffer, 0, LINE_BUFFER_SIZE);
            } else if (buffer[i] != '\r') {  // Ignorar \r
                line_buffer[line_pos++] = buffer[i];
            }
        }
        
        // Si el buffer de línea está lleno, procesarlo aunque no haya \n
        if (line_pos >= LINE_BUFFER_SIZE - 1) {
            line_buffer[line_pos] = '\0';
            printf("[EVENTO] %s\n", line_buffer);
            fflush(stdout);
            line_pos = 0;
            memset(line_buffer, 0, LINE_BUFFER_SIZE);
        }
    }

    close(sock);
    return 0;
}