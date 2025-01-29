/**
* Practica Tema 5: DAYTIME UDP
*
* González Niebla, Roger
* Mejia Largo, Belid
*
*
*/

#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h> 
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <unistd.h>    
#include <ctype.h>
// Función que se encarga de comprobar si una cadena esta formada por solo numeros
// Retorna 0 en el caso de que sea un numero, 1 en el caso de que la cadena contenga un valor no numerico.
int es_numerico(const char* cadena) {   

    int i = 0;

    while (cadena[i] != '\0') {
       
        if (!isdigit(cadena[i])) { // Si al menos un carácter no es un número, devuelve 1
            return 1;
        }
        i++;
    }  
   
    return 0;  // Si todos los caracteres son números, devuelve 0
}

int main(int argc, char* argv[]){
   
    char* ip_servidor;
    struct servent *serv;   // Estrucuta para guardar el puerto que nos proporciona un servicio de red, en este caso DAYTIME
    int sockfd;
    struct sockaddr_in myaddr, remoteaddr; // Estructura para guardar la información sobre direcciones que utilizará el socket
    myaddr.sin_family = AF_INET; // Indciamos que trabajaremos con direcciones IP del tipo IPv4
    remoteaddr.sin_family = AF_INET;
    myaddr.sin_port = 0; 
    myaddr.sin_addr.s_addr = INADDR_ANY;
    socklen_t addrlen = sizeof(remoteaddr); 
    
    
    if(argc==2 || argc==4){ // Comprobamos el caso en el que la cantidad de argumentos pasados son correctos.
        
        ip_servidor=argv[1]; // Guardamos el primer argumento como la IP del servidor.
        
        if(argc == 4){  // En el caso de que el puerto esté indicado.

            char* c_puerto = argv[3];  
            int puerto = atoi(c_puerto);
            if(es_numerico(c_puerto) == 0 && strcmp(argv[2],"-p")==0 && (puerto >= 0 && puerto <= 6553) ){  // Comprobamos si el puerto es válido y si es precedido de un "-p"
                                                                                                         
                   
                    //printf("%s %s %d",ip_servidor,argv[2],puerto);          
                    remoteaddr.sin_port = htons(puerto); // Guardamos la direccion del puerto de comunicacion pedido para hablar con el servidor
               
            }
            else{   // Si alguno de los argumentos no es válido, se lanza error correspondiente y se detiene la ejecución.
                    perror("Estructura incorrecta: ./daytime-udp-server-grupoLab [-p puerto] puerto:0-65535");
                    exit(EXIT_FAILURE);
            }
                      
        }       
        if(argc == 2){ // En el caso de no haber indicado el puerto, lo obtenemos mediante la función getservbyname()
            
            serv = getservbyname("daytime", "udp");
            if (serv == NULL) {
                perror("No se pudo obtener el puerto para el servicio DAYTIME.\n");
                exit(EXIT_FAILURE);
            }
                               
            remoteaddr.sin_port = htons(serv->s_port); // Guardamos el puerto que nos ha obtenido el servicio DAYTIME
        }
        
        if ((sockfd = socket(myaddr.sin_family, SOCK_DGRAM, myaddr.sin_port)) < 0) {
            perror("Error al crear el socket");
            exit(EXIT_FAILURE);
        }
        
       // struct in_addr addr; // Estructura para almacenar la dirección IP del servidor
        if (inet_aton(ip_servidor, &remoteaddr.sin_addr) == 0) { // Convertirmos la dirección IP en un entero de 32 bits siguiendo el formato de red (network byte order)
            perror("Dirección IP no válida.\n"); // La ip no sigue la estructura adecuada
            exit(EXIT_FAILURE);
        }
    
        if(bind(sockfd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0){ // Procedemos a enlazar el socket          
            perror("Error al enlazar el socket.\n");
            exit(EXIT_FAILURE);
        }
        
        char *mensaje = "¿Qué día es hoy?"; // Mensaje que enviará el cliente al servidor.
        
        if(sendto(sockfd, mensaje, strlen(mensaje),0, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr))< 0){            
            perror("Error al enviar datos.\n");
            exit(EXIT_FAILURE);
        }
        
        char buf[1024]; // Buffer para recibir datos
        // Recibimos los datos enviados por el servidor a traves del socket
        ssize_t recvlen = recvfrom(sockfd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&remoteaddr, &addrlen); 
        
        if (recvlen < 0) {
            perror("Error al recibir la respuesta del servidor.\n");
            exit(EXIT_FAILURE);
        }
        
        buf[recvlen] = '\0'; // Asegúramos que el buffer tenga caracter final
        printf("%s\n", buf); // Imprimos el mensaje de respuesta del servidor
        
    } else { // Si el número de argumentos pasados directamente no es el adecuado, se manda mensaje de error y se detiene la ejecución.
        perror("Estructura incorrecta: ./daytime-udp-server-grupoLab [-p puerto] puerto:0-65535");
        exit(EXIT_FAILURE); 
    }
    
    close(sockfd);
    return 0;
}
