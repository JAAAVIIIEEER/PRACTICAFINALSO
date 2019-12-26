#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/wait.h>

/*DUDAS
¿Como hacer que el hilo espere a que termine de ser atendido?
¿Mensajes del log?
¿Variable global para contar actividades?
*/

/*
Usar sprintf para 
*/

void writeLogMessage(char *id, char *msg);
void * accionSolicitud(void *arg);
void nuevaSolicitud();
void *accionesCoordinador();
void *accionesAtendedor(void * tipo);

typedef struct Solicitudes {
	int id;
	int atendido;
	int tipo;
}Solicitud;

Solicitud colaSolicitudes[15];
Solicitud colaActividad[4];
int contadorSolicitudes;
int contadorActividades;
FILE *logFile;


int main(int argc, char* argv[]){
	int i;
	pthread_t invi, QR, ambos, coordinador;
	int atendedores[3]={0,1,2};
	
	//Modificamos el comportamiento de las 3 señales
	signal(SIGUSR1, nuevaSolicitud);
	signal(SIGUSR2, nuevaSolicitud);
	signal(SIGINT, nuevaSolicitud);
	pid_t procesoPrincipal=getpid();

	//Inicializamos los contadores
	contadorSolicitudes=0;
	contadorActividades=0;
	//Inicializar semaforos, contadores, etc.
	
	for(i=0; i<15; i++){
		colaSolicitudes[i].id=0;
		colaSolicitudes[i].atendido=0;
		colaSolicitudes[i].tipo=0;
	}
	for(i=0; i<4; i++){
		colaActividad[i].id=0;
		colaActividad[i].atendido=0;
		colaActividad[i].tipo=0;
	}

	//Creamos los hilos de atendedores y coordinador.
	pthread_create(&invi, NULL, accionesAtendedor, &atendedores[2]); 
	pthread_create(&QR, NULL, accionesAtendedor, &atendedores[1]); 
	pthread_create(&ambos, NULL, accionesAtendedor, &atendedores[0]); 
	pthread_create(&coordinador, NULL, accionesCoordinador, NULL); 
	
	//Ponemos a la espera infinita de las señales
	for(;;){
		pause();
	}
}

void nuevaSolicitud(int sig){
int i, colaLibre=0, posLibre;
pthread_t solicitud;

//Buscamos un sitio libre en la cola de solicitudes
	for(i=0; i<15;i++){
		if(colaSolicitudes[i].id==0){
			colaLibre++;
			posLibre=i;
			break;
		}	
	}
	
	//Colocamos la nueva solicitud en la posición libre
	if(colaLibre>0){
		contadorSolicitudes++;
		colaSolicitudes[posLibre].id=contadorSolicitudes;
		if(sig==SIGUSR1){
			colaSolicitudes[posLibre].tipo=1;
		}else{
			colaSolicitudes[posLibre].tipo=2;
		}
		pthread_create(&solicitud, NULL, accionSolicitud, posLibre);
	}
}


void * accionSolicitud(void *arg){
int rand=40, estado, i, atendido=0, aleatorio;
	writeLogMessage("Solicitud", "Nueva solicitud recibida");
	//Comprobamos el tipo de solicitud que es.
	if(colaSolicitudes[arg].tipo==1){
		writeLogMessage("Solicitud", "Invitación");	
	
	}else{
		writeLogMessage("Solicitud", "QR");
	}
	while(atendido==0){
		sleep(4);
		if(colaSolicitudes[arg].atendido==0){
			//Calculamos si va a ser expulsada y el porque.
			if(rand){
				//En caso de que se salga.
				writeLogMessage("Solicitud", "Abandonando la cola");
				colaSolicitudes[arg].id=0;
				colaSolicitudes[arg].tipo=0;
				pthread_exit(NULL);
			}
			//Sino volvera a dormir los 4 segundos.
		}else{
			atendido=1;
			//Esperar a que termine de ser atendido.
		}
	}
	//Calculamos de participar en una actividad
	if(aleatorio==1){
		while(1){
			if(contadorActividades<3){
				contadorActividades++;
				colaActividades.id=colaSolicitudes[arg].id;
				colaSolicitudes[arg].id=0;
				colaSolicitudes[arg].atendido=0;
				colaSolicitudes[arg].tipo=0;
				wruteLogMessage("Usuario", "Preparando actividad");
				//Esperamos a que este lista la actividad.
				sleep(3);
				colaActividades.id=0;
				wirteLogMessage("Usuario", "Acabando actividad");
				pthread_exit(NULL);
			}else{
				sleep(3);		
			}
		}
	}else {
		//Liberamos el espacio en la cola en caso de no querer participar.
		colaSolicitudes[arg].id=0;
		colaSolicitudes[arg].atendido=0;
		colaSolicitudes[arg].tipo=0;
		writeLogMessage("Usuario", "Abandona la aplicación sin realizar actividad");
		pthread_exit(NULL);
	}
}

void *accionesAtendedor(void * tipo){
	int atendidas=0,i;
	int auxTipo=(int *) i;
	while(1){
		//SEMAFORO PROTEGER ZONA
		for(int i=0; i<15; i++){
			if(auxTipo==0){
			//Buscamos la que menor id tenga.	
			} else  if (auxTipo==1){
			//Buscamos la que menor id tenga con tipo 1.
			} else {
			//Buscamos la que menor id tenga con tipo 2.
			}		
		}
		//Fin semaforo proteger zona.
		if(cont==0){
			sleep(1);
		} else {
			atendidas++;
		//Calculamos tiempo de atencion
		//Protegemos la zona.
			colaSolicitudes[i].atendido=1;
		//Fin semaforo preogeger zona
			writeLogMessage("Solicitud", "Atendida");
			sleep(5); //Tiene que ser el tiempo calculado antes.	
			writeLogMessage("Solicitud", "Hora finaliza atención");
			writeLogMessage("Solicitud", "Tipo atención");
			//Semaforo
			colaSolicitudes[i].atendido=2;
			//Fin semaforo
		}
		if(atendidas==9){
			atendidas=0;
			sleep(10);	
		}
	}
}

void *accionesCoordinador(){
	//Esperar a que avisen de que hay 4.
	//Avisa a los procesos que empiecen.
	writeLogMessage("Actividad", "Actividad comenzando");
	//Esperar a que le avisen que finaliza.
	writeLogMessage("Actividad", "Actividad finalizando");
	//Abrir la lista.
}

void writeLogMessage(char *id, char *msg) {
	time_t now = time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal);
	logFile = fopen("logFinal.log", "a");
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
	fclose(logFile);
}



