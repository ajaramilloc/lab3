
# Publicador–Suscriptor en C (TCP / UDP)

Este proyecto implementa un sistema **publisher–subscriber** en C con soporte para **TCP** y **UDP**, permitiendo enviar y recibir mensajes entre múltiples publicadores y suscriptores a través de un **broker central**.

## Estructura del proyecto

```
├── broker_tcp.c
├── broker_udp.c
├── publisher_tcp.c
├── publisher_udp.c
├── subscriber_tcp.c
├── subscriber_udp.c
├── test_udp.sh
└── README.md
```

---

## Descripción general

El proyecto implementa un **modelo publicador–suscriptor (pub/sub)**:

- Los **publicadores (publishers)** envían mensajes al **broker**.  
- El **broker** recibe y reenvía los mensajes a los **suscriptores (subscribers)** conectados.  
- Se soportan dos modos de comunicación:
  - **TCP:** conexión orientada (fiable, garantiza entrega).
  - **UDP:** conexión no orientada (sin garantía de entrega, útil para pruebas de pérdida o velocidad).

---

## Ejecución

### 🔹 TCP
Cada componente se ejecuta en una terminal distinta:

```bash
# Compilar
gcc broker_tcp.c -o broker_tcp
gcc publisher_tcp.c -o publisher_tcp
gcc subscriber_tcp.c -o subscriber_tcp

# Ejecutar
./broker_tcp
./subscriber_tcp
./publisher_tcp
```

---

### 🔹 UDP

Para UDP se incluye un script que **automatiza** las pruebas y permite verificar **pérdidas de mensajes** con Wireshark.

```bash
# Dar permisos de ejecución al script
chmod +x test_udp.sh

# Ejecutar el test
./test_udp.sh n m x
```

El script lanza:
- 1 broker UDP
- n publishers UDP
- m subscribers UDP
Cada publisher envía x mensajes numerados, y los subscribers los registran para verificar errores o pérdidas.

---

## Librerías utilizadas y su interacción

El código usa las siguientes librerías estándar del sistema:

| Librería | Función principal | Interacción con el SO |
|-----------|------------------|------------------------|
| `<stdio.h>` | Manejo de entrada/salida estándar (printf, perror). | Llama a funciones del kernel para escribir en stdout/stderr. |
| `<stdlib.h>` | Asignación de memoria dinámica, control del flujo. | Administra heap y procesos en el espacio de usuario. |
| `<string.h>` | Manipulación de cadenas y buffers. | Se usa para copiar datos en memoria antes de enviarlos por socket. |
| `<unistd.h>` | Control de procesos y descriptores de archivo. | Permite cerrar sockets (`close()`) mediante syscalls del kernel. |
| `<arpa/inet.h>` | Manejo de direcciones IP (htonl, htons, inet_addr). | Traduce direcciones y puertos a formato compatible con el kernel. |
| `<sys/socket.h>` | Creación, envío y recepción de sockets. | Crea puntos de comunicación a través de llamadas al kernel (`socket()`, `sendto()`, `recvfrom()`). |
| `<netinet/in.h>` | Define estructuras y constantes para redes (AF_INET, SOCK_DGRAM, etc.). | Especifica cómo el sistema operativo debe manejar protocolos TCP/UDP. |
---

## Ejemplos de uso de las librerías en el código

### 1. Creación del socket (`sys/socket.h`, `netinet/in.h`)

La función `socket()` crea un **descriptor de archivo** en el sistema operativo, que representa el canal de comunicación entre procesos o equipos.

```c
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
if (sockfd < 0) {
    perror("Error al crear socket");
    exit(1);
}
```

    AF_INET indica que se usa IPv4.

    SOCK_DGRAM crea un socket UDP (si fuera SOCK_STREAM, sería TCP).

    El kernel asigna un número de descriptor de archivo, manejando internamente la conexión con la pila de red.

### 2. Asociación del socket a una dirección (bind())

El sistema operativo vincula el socket a un puerto y dirección IP.
Esto reserva el puerto en la tabla de conexiones del kernel.
```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;
server_addr.sin_port = htons(8080);

if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error en bind");
    exit(1);
}
```

    bind() comunica al kernel que este proceso atenderá el puerto 8080.

    Si otro proceso intenta usar el mismo puerto, el sistema devolverá un error.

### 3. Envío de datos (sendto() o send())

Dependiendo del protocolo, el envío de datos al kernel cambia:

    UDP usa sendto(), indicando destino en cada envío.

    TCP usa send(), tras haber establecido conexión.
```c
char *mensaje = "Hola desde Publisher";
sendto(sockfd, mensaje, strlen(mensaje), 0,
       (struct sockaddr *)&server_addr, sizeof(server_addr));
```

    El kernel encapsula los datos en un datagrama UDP.

    La interfaz de red se encarga de transmitir el paquete físicamente.

### 4. Recepción de datos (recvfrom() o recv())

Cuando llega un mensaje, el kernel lo almacena en el buffer de recepción del proceso.
Luego, recvfrom() lo extrae y lo coloca en memoria de usuario.

```c
char buffer[1024];
socklen_t addr_len = sizeof(client_addr);
int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                 (struct sockaddr *)&client_addr, &addr_len);
buffer[n] = '\0';
printf("Mensaje recibido: %s\n", buffer);
```

    recvfrom() bloquea la ejecución hasta que haya datos disponibles.

    El kernel borra el paquete del buffer una vez entregado al proceso.

### 5. Cierre de conexión (close())

Una vez terminada la comunicación, se libera el descriptor del socket.
Esto informa al sistema operativo que el recurso ya no se usará.

```c
close(sockfd);
```

    El kernel elimina el descriptor de su tabla interna.

    Cierra cualquier canal asociado, liberando memoria y puertos.

---

## Comunicación con el sistema operativo

Cada componente se comunica con el sistema operativo mediante **llamadas al sistema (syscalls)**:

- `socket()` → crea un descriptor de comunicación gestionado por el kernel.
- `bind()` → asocia el socket a una IP y puerto del sistema.
- `sendto()` / `recvfrom()` → envía o recibe datagramas a través del stack de red del kernel (UDP).
- `connect()` / `accept()` / `recv()` / `send()` → manejan conexiones persistentes en TCP.
- `close()` → libera el descriptor del socket.

El **kernel** se encarga del enrutamiento de paquetes, control de errores y gestión de buffers de red.  
En **UDP**, los mensajes se envían sin confirmación —por eso el script `test_udp.sh` permite detectar pérdidas—.  
En **TCP**, el kernel implementa la retransmisión automática y control de flujo.

---

## Cómo interactúan los componentes

1. **Broker**:
   - Crea un socket y lo mantiene escuchando (`bind`, `recvfrom` o `accept`).
   - Al recibir un mensaje, lo reenvía a todos los suscriptores registrados.
2. **Publisher**:
   - Envía mensajes numerados al broker usando `sendto()` o `send()`.
3. **Subscriber**:
   - Escucha mensajes provenientes del broker y los imprime en pantalla.
   - Puede analizar pérdidas en UDP comparando los números de secuencia.

---
