#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h> 

pthread_mutex_t mutexColaSolicitudes;
pthread_mutex_t mutexColaSocial;
pthread_mutex_t mutexLog;
typedef struct {
	int id;
	int atendido;
	int tipo;
	pthread_t hilo;
} Solicitud;
/*typedef struct {
	int id = 0;
	int atendiendo = 0;
} Atendedor;*/
Solicitud colaSolicitudes[15];
//Solicitud colaSocial[4];
FILE * logFile;
int contadorSolicitudes;

void manejadoraSolicitudes(int sig);
void manejadoraTerminar(int sig);
void *hiloSolicitudes(void *solicitud);
void *hiloAtendedores(void *tipo);
int isEspacioEnColaSolicitudes();

int main(int argc, char* argv[]) {
	pthread_t atendedorQR, atendedorInvitacion, atendedorPRO, coordinador;
	struct sigaction solicitudes = {0};
	struct sigaction terminar = {0};
	int aux;	
	
	contadorSolicitudes = 0;
	logFile = NULL;	
	solicitudes.sa_handler =  manejadoraSolicitudes;
	terminar.sa_handler = manejadoraTerminar;
	pthread_mutex_init(&mutexColaSolicitudes,NULL);
	pthread_mutex_init(&mutexColaSocial,NULL);
	pthread_mutex_init(&mutexLog,NULL);
	pthread_create(&atendedorInvitacion, NULL, hiloAtendedores, "0");
	pthread_create(&atendedorQR, NULL, hiloAtendedores, "1");
	pthread_create(&atendedorPRO, NULL, hiloAtendedores, "2");

	for(aux = 0; aux < 15; aux++) {
		colaSolicitudes[aux].id = 0;
		colaSolicitudes[aux].atendido = 0;
		colaSolicitudes[aux].tipo = 0;
	}

	while(contadorSolicitudes != -1) {
		if(-1 == sigaction(SIGUSR2,&solicitudes,NULL)) {
			perror("Error en el sigaction que recoge la señal SIGUSR2.");
			exit(-1);
		}	 
		if(-1 == sigaction(SIGUSR1,&solicitudes,NULL)) {
			perror("Error en el sigaction que recoge la señal SIGUSR1.");
			exit(-1);
		}
		if(-1 == sigaction(SIGINT,&terminar,NULL)) {
			perror("Error en el sigaction que recoge la señal SIGINT.");
			exit(-1);
		}
		pause();
	}

	exit(0);
	
}

int isEspacioEnColaSolicitudes() {
	int aux;
	for(aux = 0; aux < 15; aux++) {
		if(colaSolicitudes[aux].id == 0) {
			return aux;
		}
	}
	return -1;
}

int isColaLLena(int tipo) {
	int aux;
	for(aux = 0; aux < 15; aux++) {
		if(colaSolicitudes[aux].tipo == tipo || tipo == 2) {
			if(colaSolicitudes[aux].atendido == 0) {
				return aux;
			}
		}
	}	
	return -1;					
	
}

void manejadoraTerminar(int sig) {
	contadorSolicitudes = -1;
}

void manejadoraSolicitudes(int sig){
	pthread_mutex_lock(&mutexColaSolicitudes);
	int posEspacioVacio = isEspacioEnColaSolicitudes();

	if(posEspacioVacio != -1) {
		pthread_t hiloSolicitud;
		contadorSolicitudes++;
		colaSolicitudes[posEspacioVacio].id = contadorSolicitudes;
		colaSolicitudes[posEspacioVacio].atendido = 0; 
		if(sig == SIGUSR1){
			colaSolicitudes[posEspacioVacio].tipo = 0; 
		} else {
			colaSolicitudes[posEspacioVacio].tipo = 1; 
		}
		colaSolicitudes[posEspacioVacio].hilo = hiloSolicitud;
		printf("jj\n");
		pthread_create(&hiloSolicitud, NULL, hiloSolicitudes, &posEspacioVacio);   
		
	}
	pthread_mutex_unlock(&mutexColaSolicitudes);
}



void *hiloSolicitudes(void *solicitud) { 
	sleep(5);	
	while(colaSolicitudes[*(int *) solicitud].atendido == 0){
		sleep(5);
	}
 
	//pthread_exit(NULL); 
}


void *hiloAtendedores(void *tipo) { 
	int contadorTomarCafe = 0;
 	while(1) {
		//pthread_mutex_lock(&mutexColaSolicitudes);	
		
	
	}

	printf("%s\n",(char *) tipo);
}


/*void writeLogMessage(char *id, char *msg) { 
	// Calculamos la hora actual 
	time_t now = time(0); 
	struct tm *tlocal = localtime(&now);
	char stnow[19]; 
	strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal); 
	// Escribimos en el log 
	logFile = fopen(logFileName, "a"); 
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg); 
	fclose(logFile); 
}*/
