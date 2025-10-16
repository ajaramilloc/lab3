
# 🛰️ Sistema Publicador–Suscriptor en C (TCP / UDP)

Este proyecto implementa un sistema **publisher–subscriber** en C con soporte para **TCP** y **UDP**, permitiendo enviar y recibir mensajes entre múltiples publicadores y suscriptores a través de un **broker central**.

## 📂 Estructura del proyecto

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

## 🚀 Descripción general

El proyecto implementa un **modelo publicador–suscriptor (pub/sub)**:

- Los **publicadores (publishers)** envían mensajes al **broker**.  
- El **broker** recibe y reenvía los mensajes a los **suscriptores (subscribers)** conectados.  
- Se soportan dos modos de comunicación:
  - **TCP:** conexión orientada (fiable, garantiza entrega).
  - **UDP:** conexión no orientada (sin garantía de entrega, útil para pruebas de pérdida o velocidad).

---

## ⚙️ Ejecución

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

## 🧩 Librerías utilizadas y su interacción

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
| `<pthread.h>` *(si aplica)* | Manejo de hilos para paralelismo en broker o subs. | Crea threads en espacio de usuario, coordinados por el kernel. |

---

## 🧠 Comunicación con el sistema operativo

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

## 📡 Cómo interactúan los componentes

1. **Broker**:
   - Crea un socket y lo mantiene escuchando (`bind`, `recvfrom` o `accept`).
   - Al recibir un mensaje, lo reenvía a todos los suscriptores registrados.
2. **Publisher**:
   - Envía mensajes numerados al broker usando `sendto()` o `send()`.
3. **Subscriber**:
   - Escucha mensajes provenientes del broker y los imprime en pantalla.
   - Puede analizar pérdidas en UDP comparando los números de secuencia.

---

## 🧭 Diagrama de interacción



El diagrama muestra cómo se comunican los **publishers**, el **broker** y los **subscribers** tanto en **TCP (líneas continuas)** como en **UDP (líneas punteadas)**.

---

---
