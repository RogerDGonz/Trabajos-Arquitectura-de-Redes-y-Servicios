/**
 * Practica Tema 7: CLIENTE TFTP
 *
 * Gonzalez Niebla, Roger
 * Mejia Largo, Belid
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> //manejo de los sockets
#include <netinet/ip.h> //guardar las IP
#include <errno.h> //para el uso de perror
#include <arpa/inet.h> //para el uso de inet_aton
#include <unistd.h> //para usar el close

#define TFTP_PORT 69 // Para indicar el puerto donde se comunica el servidor
#define BUFFER_SIZE 516 //es el tamaÃ±o mÃ¡ximo para TFTP (512 bytes de datos + 4 bytes de cabecera) con los paquete

void crear_paquete_tftp(char *buffer, int opcode, const char *fichero, const char *modo) {
    uint16_t opcodeAux = htons(opcode); //convertir opcode a formato de red
    int offset = 0;
    // el opcode de 2 bytes al buffer
    memcpy(buffer + offset, (void *)&opcodeAux, sizeof(uint16_t));
    offset += 2;
    //nombre del archivo
    strcpy(buffer + offset, fichero);
    offset += strlen(fichero) + 1; //+1 para incluir el '\0' y cerrar el string
    //modo
    strcpy(buffer + offset, modo);
    offset += strlen(modo) + 1; //+1 para incluir el '\0' y cerrar el string

}


int main(int argc, char *argv[]) {
    
    if (argc < 4 || argc > 5) { //el numero de argumentos pasado no es el adecuado, se lanza error indicando el formato adecuado

        fprintf(stderr, "Uso: /tftp-client ip_servidor {-r|-w} fichero [-v]\n");
        exit(EXIT_FAILURE);
    }
    
    // Declaracion de las variables pasadas como argumentos de entrada    
    char *ip_servidor = argv[1];
    char *operacion = argv[2];
    char *fichero = argv[3];
    int banderaV = 0;           //bandera que nos permitira conocer si se solicito por entrada la informacion de los mensajes enviados entre servidor y cliente
    int permisoEscritura = 0;   //bandera que nos permite en caso de solicitud de escritura saber si tenemos el permiso de escribir el fichero por parte del servidor TFTP

    if (argc == 5 && strcmp(argv[4], "-v") == 0) { //activamos bandera para informar de los mensajes enviados durante la lectura/escritura del fichero
        banderaV = 1; 
    } 
    else if(argc == 5 && strcmp(argv[4],"-v") != 0){
	perror("Uso: /tftp-client ip_servidor {-r|-w} fichero [-v]\n");
	exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in remoteaddr;
    socklen_t addrlen = sizeof(remoteaddr);
    char buffer[BUFFER_SIZE];

    // Creacion del socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket"); //si hay un error durante la creacion del socket se comunica y se detiene la ejecucion
        exit(EXIT_FAILURE);
    }

    memset(&remoteaddr, 0, sizeof(remoteaddr));
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_port = htons(TFTP_PORT);
    if (inet_aton(ip_servidor, &remoteaddr.sin_addr) == 0) {
        perror("DirecciÃ³n IP no vÃ¡lida");
        exit(EXIT_FAILURE);
    }

    //vemos si la operacion indicada se trata de escritura o lectura
    int opcode;
    if (strcmp(operacion, "-r") == 0) {
        opcode = 1; // Solicitud de lectura
    } else if (strcmp(operacion, "-w") == 0) {
        opcode = 2; // Solicitud de escritura
    } else {
        fprintf(stderr, "OperaciÃ³n no vÃ¡lida Usa -r (lectura) o -w (escritura)\n");
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, BUFFER_SIZE);
    char *modo = "octet";
    crear_paquete_tftp(buffer, opcode, fichero, modo); //creamos el paquete del mensaje que enviara el cliente al servidor 

    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&remoteaddr, addrlen) < 0) {
         perror("Error al enviar la solicitud TFTP");
         exit(EXIT_FAILURE);
    }

    if(banderaV == 1){ // Si -v  esta activo se imprime mensaje solicitud enviada al servidor TFTP
        printf("Enviada solicitud de %s de \"%s\" a servidor TFTP (%s)\n",opcode == 1 ? "lectura" : "escritura", fichero, ip_servidor);
    }

    
    FILE *file = fopen(fichero, (opcode == 1) ? "w" : "r");
    if (file == NULL) { 
        perror("Error del servidor TFTP: errcode:00, errstring: Not defined\n"); // error de archivo 
        exit(EXIT_FAILURE);
    }
    
    int bloque_esperado = 1; // Numero de bloque que espera recibir el cliente

    while (1) {

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t recvlen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&remoteaddr, &addrlen); // Recibimos la respuesta del servidor frente a nuestra peticion
        
        if (recvlen < 0) { //lanzamos error si se produce un fallo en la recepcion de datos
            perror("Error al recibir datos del servidor");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        uint16_t opcode_resp = ntohs(*(uint16_t *)buffer); //obtenemos del mensaje de respuesta del servidor el opcode enviado
        uint16_t bloque_recibido = ntohs(*(uint16_t *)(buffer + 2)); 	//obtenemos el numero de bloque que nos devuelve el servidor
        unsigned char octetos[4];
        uint16_t bloqueRecibidoOcteto = htons(bloque_recibido);    //a formato de red
        uint16_t bloqueEsperadoOcteto = htons(bloque_esperado);    //a formato de red

        // Extraer bytes alto y bajo
        octetos[0] = ((unsigned char *)&bloqueRecibidoOcteto)[0];  //byte alto del bloque recibido
        octetos[1] = ((unsigned char *)&bloqueRecibidoOcteto)[1];  //byte bajo del bloque recibido
        octetos[2] = ((unsigned char *)&bloqueEsperadoOcteto)[0];  //byte alto del bloque esperado
        octetos[3] = ((unsigned char *)&bloqueEsperadoOcteto)[1];  //byte bajo del bloque esperado

        unsigned char ack[] = {0x00, 0x04, octetos[0], octetos[1]}; //creamos el mensaje ACK para responder al servidor

        
        if (opcode_resp == 3) { //comprobamos si el mensaje de respuesta del servidor es un data
        
            if(banderaV == 1){
            fprintf(stderr,"Recibido bloque del servidor tftp\n");
            }

            if(bloque_esperado == 1){

		  if(banderaV == 1){
                	fprintf(stderr,"Es el primer bloque (numero de bloque 1)\n");
		  }
            }
            else if(banderaV == 1){
                fprintf(stderr,"Es el bloque numero %d ", bloque_esperado);
            }

            if (bloque_recibido != bloque_esperado && opcode == 1) { 
                fprintf(stderr, "Esperado bloque %d, peroctetos[0], octetos[1] bloque %d Ignorando este bloque\n", bloque_esperado, bloque_recibido);
                
                if (sendto(sockfd, ack, 4, 0, (struct sockaddr *)&remoteaddr, addrlen) < 0) {
                        perror("Error al reenviar ACK");
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }
                    continue; //no procesamos el bloque recibido, seguimos esperando el bloque correcto
                }

            else if(bloque_recibido == bloque_esperado && opcode == 1){ // Si comunica al servidor mediante un ACK el bloque recibido
                
                if(banderaV == 1){ //si se solicito -v se imprime mensaje de envio de ACK del bloque al servidor TFTP
                    fprintf(stderr,"Enviamos ACK del bloque %d\n", bloque_recibido);
                }

                if(fwrite(buffer + 4, 1, recvlen - 4, file) != recvlen - 4){ //se escribe en el fichero los datos recibidos del data
                    perror("Error al escribir en el archivo");
                }	
            
                bloque_esperado++;

                if (sendto(sockfd, ack, 4, 0, (struct sockaddr *)&remoteaddr, addrlen) < 0) { //enviamos el ACK del bloque al servidor
                        perror("Error al enviar ACK");
                        fclose(file);
                        exit(EXIT_FAILURE);
                }
            }

            if (recvlen < 516) { //el mensaje recibido data tiene una longitud menor a 516 bytes, es el ultimo bloque y cerramos
            
                if(banderaV == 1){  //con -v se imprime mensaje de recepcion del ultimo bloque por parte del servidor	
                    fprintf(stderr,"El bloque %d era el Ãºltimo: cerramos el fichero\n",bloque_recibido);
                }

                break; //salir del bucle
            }

        } else if(opcode_resp == 4){ //comprobamos si el mensaje de respuesta del servidor es un ACK
        
	
    	    if(permisoEscritura == 0 && bloque_recibido == 0){ //se imprimen mensajes de informacion 
	    	   if(banderaV == 1){

		   	fprintf(stderr,"Recibido bloque del servidor tftp\n");
	   	   	fprintf(stderr,"Es el bloque 0\n");
		   }	   
		   permisoEscritura = 1;
	    }
	     
	    else if(permisoEscritura == 0 && bloque_recibido != 0){ // si no tenemos permiso para escribir un archivo ya puesto	
		   perror("La operacion de escritura no esta permitida");
	    	   fclose(file);
		   exit(EXIT_FAILURE);

	    } else if(bloque_recibido != 0 && permisoEscritura == 1){ //si esta -v, informacion solicitados por el cliente
		 
		    if(banderaV == 1){
		 	fprintf(stderr,"Recibido ACK del servidor tftp\n");
		 	fprintf(stderr,"Es el bloque numero %d\n",bloque_recibido);
		 }	
	    }	    

	    if(banderaV == 1){
	    	fprintf(stderr, "Enviamos el Data del bloque %d\n", bloque_esperado);
	    }
            
	    //creacion del mensaje data con el contenido del fichero del cual se quiere escribir
	    
	    unsigned char data[4] = {0x00, 0x03, octetos[2],octetos[3]};
            unsigned char buffer2[512];
            size_t bytesRead;


            bytesRead = fread(buffer2,1,512,file); //lectura de 512 bytes del fichero indicado

            size_t newSize = 4 + bytesRead;
            unsigned char *resultBuffer = malloc(newSize);
            
            if(resultBuffer == NULL){
                perror("Error al asignar memoria");
                fclose(file);

                exit(EXIT_FAILURE);
            }
            memcpy(resultBuffer, data, 4);
            memcpy(resultBuffer + 4,buffer2, bytesRead);


            if(sendto(sockfd, resultBuffer, newSize, 0, (struct sockaddr *)&remoteaddr, addrlen) < 0){ //enviamos un bloque data con el contenido del fichero al servidor
                perror("Error al enviar data");
                fclose(file);
                exit(EXIT_FAILURE);
            }

            free(resultBuffer);

	     if(bytesRead < 512){ //si el ya no hay mas bytes para leer del fichero entonces se termina la comunicacion con el servidor
		                     
		 if(banderaV == 1){  // con -v se imprime mensaje de ya haber realizado el envio del ultimo bloque al servidor	
		     
			 fprintf(stderr,"El bloque %d era el ultimo: cerramos el ficher",bloque_esperado); 

		 }

		 break;//salimos del bucle
	   }

            bloque_esperado++;


            
        }else if (opcode_resp == 5) { //si el mensaje de repsuesta del servidor se corresponde con un error

                fprintf(stderr, "Error del servidor TFTP: errcode:%02x, errstring: %s\n",octetos[1],buffer + 4); //ponemos el error surgido al usuario
                fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    //al acabar el envio de mensajes entre el servidor y el cliente se cierra el fichero y el socket
    fclose(file);    
    close(sockfd);
    fprintf(stderr,"\n");
    return 0;
}

