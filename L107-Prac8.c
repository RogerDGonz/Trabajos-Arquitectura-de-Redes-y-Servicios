/*
 * Practica Tema 8: ICMP-TIMESTAMP
 *
 * Gonzalez Niebla, Roger
 * Mejia Largo, Belid
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ip-icmp.h" // Libreria para manejar estructuras ICMP
#include<unistd.h> // Necesaria para getpid
#include <sys/time.h> // Necesaria para gettimeofday
#include <errno.h> // Para manejo de errores


// Con esta funcion rellenamos los campos necesarios para una solicitud de tipo TimeStamp Request
TimeStamp crearTimestampRequest(struct sockaddr_in remoteaddr){
    
    TimeStamp timeStamp;
    ICMPHeader icmpHeader;
    icmpHeader.type = 13; // Tipo TimeStamp Request
    icmpHeader.code = 0; 
    icmpHeader.checksum = 0; // Inicializamos el cehksum a 0
    timeStamp.icmpHdr = icmpHeader;
    timeStamp.pid = getpid(); // Con getpid() damos id al timestamp
    timeStamp.sequence = 0;

    struct timeval tv;    
    gettimeofday(&tv, NULL); // Obtenemos el tiempo actual de origen.
    long int milliseconds = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
    timeStamp.originate = milliseconds;
    timeStamp.receive = 0;
    timeStamp.transmit = 0;

    return timeStamp;

}

// Funcion que realiza la validacion del cheksum
unsigned short checksum(unsigned short* buffer, int len) {

	unsigned int acum = 0;
	int i=0;
	
	for(i=0;i<len;i++){ 

		acum+=(unsigned short) *buffer;
		buffer++;	
	}

	acum = (acum >> 16) + (acum & 0xFFFF);  // Para el acarreo   
	acum = (acum >> 16) + (acum & 0xFFFF);	
	return ~(acum & 0x0000ffff);
}

// Funcion para imprimir por salida los mensajes correspondientes a las respuestas que no sean un TimeStamp Reply
void mensajes(int type, int code) {

	switch (type) { 
	case 3: // Destination Unreachable

		printf("-> Destination Unreachable: ");

		switch (code) {

	        case 0: printf("Net Unreachabe"); break;
		case 1: printf("Host Unreachable"); break;
		case 2: printf("Protocol Unreachable"); break;
		case 3: printf("Port Unreachable"); break;
		case 4: printf("Fragmentation Needed"); break;
		case 5: printf("Code 5: Source Route Failed.\n"); break;
		case 6: printf("Destination Network Unknown"); break;
		case 7: printf("Destination Host Unknown"); break;			
		case 8: printf("Source Host Isolated"); break;				       		    
	       	case 11: printf("Destination Network Unreachable for Type of Service"); break;						
		case 12: printf("Destination Host Unreachable for Type of Service"); break;
		case 13: printf("Communication Administratively Prohibited"); break;	
		case 14: printf("Host Precedence Violation"); break;			
		case 15: printf("Precedence Cutoff in Effect"); break;

		}

		printf(" (Type %d, Code %d)\n", type, code);
    		break;

        case 5: // Redirect

	        printf("-> Redirect: ");

		switch (code) {

		case 1: printf("Redirect for Destination Host"); break;
		case 3: printf("Redirect for Host Based on Type-of-Service"); break;
		}
		
		printf(" (Type %d, Code %d)\n", type, code);
		break;

	case 11: // Time Exceeded
	
    	printf("Time Exceeded: ");	
	switch (code) {
	 	case 0: printf("Time-to-Live Exceeded in Transit"); break;		
		case 1: printf("Fragment Reassembly Time Exceeded"); break;
		}
		printf(" (Type %d, Code %d)\n", type, code);
													     
		break;

        case 12: // Parameter Problem
	
	printf("Parameter Problem: ");
	switch (code) {
			
		case 0: printf("Pointer indicates the error"); break;		
		case 1: printf("Missing a Required Option"); break;		
		case 2: printf("Bad Length"); break;
		}
		printf(" (Type %d, Code %d)\n", type, code);			
																			    
	break;

	}
}

	   

int main(int argc, char *argv[]) {
    

    if (argc < 2 || argc > 3) { //Si el numero de argumentos pasado no es el adecuado, se lanza error indicando el formato adecuado

        printf("Uso: ./timestamp [-v]\n");
        exit(EXIT_FAILURE);
    }
     
    char *ip_destino = argv[1];    
    int banderaV = 0;           //bandera que nos permitira conocer si se solicito mostrar el valor que contienen los campos del datagrama ICMP enviado y recibido. 

    if (argc == 3 && strcmp(argv[2], "-v") == 0) { //activamos bandera para informar de los valores de los campos del datagrama ICMP en el envio y en la recepcion.
        banderaV = 1; 
    } 

    int sockfd;
    struct sockaddr_in remoteaddr;
    socklen_t addrlen = sizeof(remoteaddr); 

    // Creacion del socket
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        perror("Error al crear el socket RAW");
        exit(EXIT_FAILURE);
    }

    // Configuracion de la direccion de destino
    memset(&remoteaddr, 0, sizeof(remoteaddr));
    remoteaddr.sin_family = AF_INET;

    if (inet_aton(argv[1], &remoteaddr.sin_addr) == 0) {  // Verificamos y asignamos la IP destino
        perror("Dirección IP destino no válida");
        exit(EXIT_FAILURE);
    }


    TimeStamp  mensaje = crearTimestampRequest(remoteaddr); // Creacion del datagrama ICMP que se va a enviar.
    
    size_t size = sizeof(mensaje);
    char buffer[size];    
    memset(buffer, 0, size);
    memcpy(buffer, &mensaje, sizeof(mensaje));

    printf("\n");
    printf("-> Enviando Datagrama ICMP a Destino con IP: %s\n",ip_destino);	
    mensaje.icmpHdr.checksum = checksum((unsigned short *)&mensaje,(int) sizeof(mensaje)/2); 


    if(banderaV == 1){ // Si se solicito informacion con -v se imprimen los mensajes de salida.
    	
	    printf("-> Type: %u \n",mensaje.icmpHdr.type);
	    printf("-> Code: %u \n", mensaje.icmpHdr.code);
	    printf("-> PID: %hu \n", mensaje.pid);
	    printf("-> Seq Numbr: %hu \n", mensaje.sequence);
	    printf("-> Originate: %u\n", mensaje.originate);
	    printf("-> Receive:  %u\n", mensaje.receive);
	    printf("-> Transmit: %u\n", mensaje.transmit);
	    printf("-> Tamanyo del Datagrama: %lu bytes\n",sizeof(mensaje));

    }

    // Envio de los datos 
    if (sendto(sockfd, &mensaje, sizeof(mensaje), 0, (struct sockaddr *)&remoteaddr, addrlen) < 0) {
         perror("Error al enviar TimeStamp Request");
         exit(EXIT_FAILURE);
    
    }

    printf("-> Timestamp Request enviado correctamente...\n");

    // Monitorización con selec()
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);              
    FD_SET(sockfd, &readfds);       
    timeout.tv_sec = 5;        
    timeout.tv_usec = 0;            

    int retval = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
    if (retval == -1) {
        perror("Error en select()");
        exit(EXIT_FAILURE);

    } else if (retval == 0) {

        printf("-> Tiempo de espera agotado. No se recibió respuesta.\n");
        close(sockfd);
        exit(EXIT_FAILURE);

    } else {

	TimeStampReply mensajeRecibido; // Creamos mensaje para guardar los datos de la respuesta.

	// Recepcion de datos
        ssize_t recvlen = recvfrom(sockfd, &mensajeRecibido, sizeof(TimeStampReply), 0, (struct sockaddr *)&remoteaddr, &addrlen);
       
	if (recvlen < 0) { //lanzamos error si se produce un fallo en la recepcion de datos
        
            perror("Error al recibir Timestamp Reply");
            exit(EXIT_FAILURE);
        
        }
	if(banderaV == 1){

		printf("\n");
	}
        printf("-> Timestamp Reply recibido desde %s\n", ip_destino);
       	

   	if (mensajeRecibido.icmpMsg.icmpHdr.type != 14 && mensajeRecibido.icmpMsg.icmpHdr.type != 13) { // Si la respuesta no se un reply entonces imprimos el mesnaje correspondiete obtenido de la funcion mensajes
   
	 	mensajes(mensajeRecibido.icmpMsg.icmpHdr.type,mensajeRecibido.icmpMsg.icmpHdr.code);
        	printf("\n");             

	}else{

	struct timeval tv;    
	gettimeofday(&tv, NULL); // Obtenemos el tiempo actual
	long int miliseconds = (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
	unsigned int h = (unsigned int) miliseconds;

	// Calculamos la desviacion horaria producida durante el envio y recepcion de datos.
        int RTT = (unsigned int)(mensajeRecibido.icmpMsg.receive - mensaje.originate) + (h - mensajeRecibido.icmpMsg.transmit);

        if(banderaV == 1){ // Si se solicito informacion con -v se imprimen los mensajes de salida

        	printf("-> Originate: %u\n", mensajeRecibido.icmpMsg.originate);
        	printf("-> Receive:  %u\n", mensajeRecibido.icmpMsg.receive);
        	printf("-> Transmit: %u\n", mensajeRecibido.icmpMsg.transmit);
        	printf("-> RTT: %d miliseconds\n", RTT); // (Preguntar mili o seg?)
        	printf("-> TTL: %u\n", mensajeRecibido.ipHdr.TTL);
        	printf("-> Tamanyo del Datagrama: %lu bytes\n", sizeof(mensajeRecibido));
        }


	if(mensajeRecibido.icmpMsg.icmpHdr.type == 14){ // Se imprime el mensaje correspondiente al reply
		printf("-> Respuesta Correcta (Type %u, Code %u)\n", mensajeRecibido.icmpMsg.icmpHdr.type, mensajeRecibido.icmpMsg.icmpHdr.code);	
	
	} else if( mensajeRecibido.icmpMsg.icmpHdr.type == 13){ // Si el datagrama no ha sido proceso se indica en la salida

		printf("-> ICMP Datagram Not Processed...\n");
	}

	printf("\n");


    }
	}

    close(sockfd);  
    return 0;

}
