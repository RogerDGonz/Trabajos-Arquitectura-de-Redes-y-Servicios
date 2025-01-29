/**
* Practica Tema 6: DAYTIME TCP
*
* González Niebla, Roger
* Mejia Largo, Belid
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> // Para la manipulacion y creacion de sockets 
#include <netinet/in.h> // Para sockaddr_in
#include <arpa/inet.h> // Para la manipulacion de estructuras de red // Para utilizar la función inet_aton
#include <errno.h> // Para las definiciones de los errores
#include <unistd.h>  // Para close
#include <netdb.h> // Para manejo de getservbyname
#include <ctype.h> // Para la función isdigit que será utilizada en es_numerico

#define BUFFER_SIZE 1024

// Función que se encarga de comprobar si una cadena esta formada por solo numeros
// Retorna 0 en el caso de que sea un numero, 1 en el caso de que la cadena contenga un valor no numerico.
int es_numerico(const char* cadena) {
    int i = 0;
    while (cadena[i] != '\0') {
        if (!isdigit(cadena[i])) return 0;
        i++;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    int sockfd; //socket cliente 
    struct sockaddr_in serveraddr;
    char buffer[BUFFER_SIZE];
    struct servent *serv;
    char *ip_servidor;
    unsigned short puerto;

    // Verificamos los argumentos de entrada y establecemos IP y puerto
    if (argc == 2 || argc == 4) { // Comprobamos el caso en el que la cantidad de argumentos pasados son correctos.
        ip_servidor = argv[1]; // Guardamos la IP del servidor

        if (argc == 4 && strcmp(argv[2], "-p") == 0 && es_numerico(argv[3])) {// En el caso de que el puerto esté indicado.
            puerto = (unsigned short) atoi(argv[3]); // Obtenemos el puerto del argumento
        } else if (argc == 2) { // En el caso de no haber indicado el puerto, lo obtenemos mediante la función getservbyname()
            serv = getservbyname("daytime", "tcp");
            if (serv == NULL) {
                perror("No se pudo obtener el puerto para el servicio DAYTIME."); // Si no es posible obtener un puerto lanzamos mensaje de error correspondiente.
                exit(EXIT_FAILURE);
            }
            puerto = ntohs(serv->s_port);
        } else { // En el caso que los argumentos de entrada no sean válidos indicamos la estructura a seguir para pasarlos.
            fprintf(stderr, "Uso: %s ip_servidor [-p puerto]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } else {  // En el caso que los argumentos de entrada no sean válidos indicamos la estructura a seguir para pasarlos.
        fprintf(stderr, "Uso: %s ip_servidor [-p puerto]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Creamos el socket tcp
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    //Enlazamos el socket a  la ip y puerto local
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = 0;
    // realizamos el bind
    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Colocamos los datos de dirección del servidor
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(puerto);
   
    // Comprobamos que sea una dirreccion valida 
    if (inet_aton(ip_servidor, &serveraddr.sin_addr) == 0) {
        perror("Dirección IP no válida.");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Conectamos el socket con el servidor
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("Error al conectar con el servidor");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Enviamos la cadena al servidor y esperamos a recibir una respuesta
    char *mensaje = "¿Qué día es hoy?"; // Cadena lanzada al servidor.
    if (send(sockfd, mensaje, strlen(mensaje), 0) < 0) {
        perror("Error al enviar datos al servidor"); // Imprimimos mensaje correspondiente si error en el envio de la cadena.
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Recibimos la respuesta del servidor
    ssize_t recvlen = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (recvlen < 0) {
        perror("Error al recibir datos del servidor"); // Imprimimos mensaje correspondiente si error al recibir la respuesta del servidor.
    } else {
        buffer[recvlen] = '\0'; // Nos aseguramos de que el buffer tenga una caracter final.
        printf("Respuesta del servidor: %s\n", buffer);
    }

    // Finalmente pasamos al cierre de conexión
    if (shutdown(sockfd, SHUT_RDWR) < 0) { //Notificamos al servidor que se quiere cerrar la conexión
        perror("Error al cerrar la conexión con el servidor");
    } else {
        //Nos aseguramos de que no haya datos pendientes de lectura
        recvlen = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if(recvlen < 0) {
            perror("Error al verificar datos pendientes");
        }
    }

    //Cerramos el socket
    close(sockfd);
    return 0;
}
