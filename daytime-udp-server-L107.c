/**
* Practica Tema 5: DAYTIME UDP
*
* González Niebla, Roger
* Mejia Largo, Belid
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // Para sockaddr_in
#include <arpa/inet.h>  // Para inet_ntoa
#include <unistd.h>     // Para close
#include <time.h>       // Para obtener la fecha y hora
#include <netdb.h>      // Para getservbyname
#include <ctype.h>

#define BUFFER_SIZE 1024

/* Función para comprobar si el puerto es un valor numérico y está dentro de los parámetros */
int es_numerico(const char* cadena) {
    int i = 0;

    // Recorre cada carácter de la cadena
    while (cadena[i] != '\0') {
        // Si al menos un carácter no es un número, devuelve 1
        if (!isdigit(cadena[i])) {
            return 1;
        }
        i++;
    }
    if (atoi(cadena) >= 0 && atoi(cadena) <= 65535) {
        return 0; // Si todos los caracteres son números y está en el rango de puertos, devuelve 0
    }
    return 1;
}

int main(int argc, char* argv[]) {
    int sockfd, puerto;
    puerto = 0;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    char buffer[BUFFER_SIZE];

    if (argc == 3 || argc == 1) {
        if (argc == 3) { // Caso donde hay puerto en el comando
            // Comprobamos que los argumentos sean correctos
            if (strcmp(argv[1], "-p") == 0 && es_numerico(argv[2]) == 0) {
                puerto = atoi(argv[2]);
            } else {
                perror("Estructura incorrecta ./daytime-udp-server-grupoLab [-p puerto] puerto:0-65535");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Crear el socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Obtener el puerto del servicio "daytime"
    struct servent *serv = getservbyname("daytime", "udp");
    //verificamos si devolvio un valor
    if (serv == NULL) {
        perror("No se pudo obtener el puerto para el servicio DAYTIME.");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configuración del servidor
    serveraddr.sin_family = AF_INET;
    
    // Verificamos si el puerto es el automático o el puesto por comando
    if (argc == 1) { // Puerto base
        serveraddr.sin_port = htons(serv->s_port);  // Puerto del servicio "daytime"
    } else { // Puerto por comando
        serveraddr.sin_port = htons(puerto);      // Puerto fijado en comando
    }
    serveraddr.sin_addr.s_addr = INADDR_ANY;    // Aceptar conexiones de cualquier interfaz

    // Enlazar el socket al puerto y dirección del servidor
    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //printf("Servidor DAYTIME UDP esperando solicitudes en puerto %d...\n", ntohs(serveraddr.sin_port));

    while (1) {
        // Esperar por un mensaje del cliente
        ssize_t recvlen = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&clientaddr, &addrlen);

        // Caso donde la respuesta es incorrecta
        if (recvlen < 0) {
            perror("Error al recibir mensaje del cliente");
            continue;  // Continuar esperando más mensajes
        }

        buffer[recvlen] = '\0'; // Asegurarse de que el buffer tenga terminación
        //printf("Mensaje recibido de %s:%d: %s\n",inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buffer);

        // Obtener la fecha y hora actuales
        FILE *fd;
        char buff[BUFFER_SIZE];
        system("date > /tmp/date.txt");
        fd = fopen("/tmp/date.txt", "r");

        // Ponemos la fecha y hora en el buffer a enviar
        if (fgets(buff, BUFFER_SIZE, fd) == NULL) {
            printf("Error en system(), fopen() o fgets()\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Ponemos el inicio de la respuesta con mi ID de máquina virtual (roger)
        char respuesta[BUFFER_SIZE] = "vm2502: ";

        // Unimos la respuesta a enviar
        strcat(respuesta, buff);

        // Enviar la respuesta al cliente
        ssize_t sentlen = sendto(sockfd, respuesta, strlen(respuesta), 0, (struct sockaddr *)&clientaddr, addrlen);

        // Verifico si se ha enviado correctamente
        if (sentlen < 0) {
            perror("Error al enviar respuesta al cliente");
        } else {
            printf("Respuesta enviada a %s\n", inet_ntoa(clientaddr.sin_addr));
        }
    }

    // Cerrar el socket (nunca llegaremos aquí en este código)
    close(sockfd);
    return 0;
}

