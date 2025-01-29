/**
* Servidor TCP concurrente DAYTIME con manejo de señal SIGINT
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
#include <signal.h>

#define BUFFER_SIZE 1024 //Indicamos el tamaño del buffer de envio de respuesta
#define DATA_BUFFER_SIZE 64 //Indicamos el tamaño del buffer para el mensaje de fecha y hora.

int main_socket; // Declaración global del socket principal para usar en el manejador de señal

//Hacemos uso del manejador de señal para capturar la señal de cierre (Crtl + C ) y cerrar el servidor limpiamente
void signal_handler(int sennal) {
    if (sennal == SIGINT) {
        
        
        // Intentamos cerrar el socket principal con shutdown para finalizar la conexion
        if (shutdown(main_socket, SHUT_RDWR) < 0) {
            perror("Error en shutdown del socket principal");
        }
        
        // Cerramos el socket principal con close
        if (close(main_socket) < 0) {
            perror("Error al cerrar el socket principal");
        } 
        exit(0); // acaba proceso principal
    }
}

// Función para comprobar si el puerto es un valor numérico y está en rango
int es_numerico(const char* cadena) {
    int i = 0;
    while (cadena[i] != '\0') {
        if (!isdigit(cadena[i])) {
            return 1;
        }
        i++;
    }
    return (atoi(cadena) >= 0 && atoi(cadena) <= 65535) ? 0 : 1;
}

//función para cada conexión con un cliente
void responderCliente(int client_socket) {
    char buffer[BUFFER_SIZE];
    FILE *fd;
    char date_buff[DATA_BUFFER_SIZE];
    ssize_t nbytes;

    //obtenemos la fecha y hora actuales
    system("date > /tmp/date.txt");
    fd = fopen("/tmp/date.txt", "r");
    if (fd == NULL || fgets(date_buff, DATA_BUFFER_SIZE, fd) == NULL) {
        perror("Error al obtener la fecha y hora");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    fclose(fd);

    // Formateamos la respuesta que vamos a enviar al cliente
    snprintf(buffer, sizeof(buffer), "vm2502: %s", date_buff);

    // Enviamos la respuesta al cliente
    if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
        perror("Error al enviar respuesta al cliente");
    } 

    //cerrar la conexión con shutdown()
    if (shutdown(client_socket, SHUT_RDWR) < 0) {
        perror("Error al cerrar la conexión con el cliente");
    }

    //recibir del cliente para verificar cierre
    char recv_buff[1000];
    nbytes = recv(client_socket, recv_buff, 1000, 0);
    if (nbytes == 0) {
        close(client_socket); //cerramos el socket del cliente
    } else if (nbytes < 0) {
        perror("Error al recibir confirmación de cierre del cliente");
    }

    
}

int main(int argc, char* argv[]) {
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    int puerto = 0;

    // Configurar para la señal de cierre 
    signal(SIGINT, signal_handler);

    if (argc == 3) { //si nos dieron un puerto
        if (strcmp(argv[1], "-p") == 0 && es_numerico(argv[2]) == 0) {
            puerto = atoi(argv[2]);
        } else {
            perror("Estructura incorrecta: ./daytime-tcp-server [-p puerto] (puerto 0-65535)");
            exit(EXIT_FAILURE);
        }
    }

    //creamos el socket tpc
    if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    //obtener el puerto de daytime 
    if (argc == 1) { 
          
        struct servent *serv = getservbyname("daytime", "tcp");
        if (serv != NULL) {
            puerto = ntohs(serv->s_port);
           
        }
        if (serv == NULL) {
            perror("No se pudo obtener el puerto para el servicio DAYTIME. Cerrando el servidor");
            close(main_socket); // Cerrar el socket principal
            exit(EXIT_FAILURE); // Terminar el programa
        } else {
            puerto = ntohs(serv->s_port);
        }
    } 
    serveraddr.sin_port = htons(puerto);
    //configurar la dirección del servidor
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    //la reutilización inmediata del puerto principal
    int opt = 1;
    if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error en setsockopt");
        close(main_socket);
        exit(EXIT_FAILURE);
    }

    // el socket lo anlazamos con el puerto
    if (bind(main_socket, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("Error al enlazar el socket");
        close(main_socket);
        exit(EXIT_FAILURE);
    }

    //escuchamos conexiones entrantes
    if (listen(main_socket, 10) < 0) {
        perror("Error en listen");
        close(main_socket);
        exit(EXIT_FAILURE);
    }

    

    while (1) {
        int client_socket = accept(main_socket, (struct sockaddr *)&clientaddr, &addrlen);
        if (client_socket < 0) {
            perror("Error en accept");
            continue;
        }

        

        //se crea proceso hijo para manejar al cliente
        pid_t pid = fork();
        if (pid == 0) { //si es proceso hijo
            close(main_socket); // Cierra el socket principal en el hijo
            responderCliente(client_socket);
            exit(0);
        } else if (pid > 0) { //si es el proceso padre
            close(client_socket); // cierra el socket del cliente
        } else { // Error en fork
            perror("Error al crear proceso hijo");
            close(client_socket);
        }
    }

    return 0;
}

