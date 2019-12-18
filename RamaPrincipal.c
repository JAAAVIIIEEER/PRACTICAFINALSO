#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h> 

// DECLARACIÓN VARIABLES GLOBALES
pthread_mutex_t mutexColaSolicitudes;
pthread_mutex_t mutexColaSocial;
pthread_mutex_t mutexLog;

typedef struct {
	int id;
	int atendido;
	int tipo;
	pthread_t hilo;
} Solicitud;

typedef struct{
  int atendiendo;
  int tipo;
} Atendedor;

Atendedor atendedores[3]
Solicitud colaSolicitudes[15];
Solicitud colaSocial[4];

FILE * logFile;
int contadorSolicitudes;

void manejadoraSolicitudes(int sig);
void manejadoraTerminar(int sig);
void *hiloSolicitudes(void *solicitud);
void *hiloAtendedores(void *tipo);
int isEspacioEnColaSolicitudes();
int calculaAleatorios(int min, int max);
pid_t gettid(void);

int main(int argc, char* argv[]) {
	pthread_t atendedorQR, atendedorInvitacion, atendedorPRO, coordinador;
	struct sigaction solicitudes = {0};
	struct sigaction terminar = {0};
	int aux;	
	
	contadorSolicitudes = 0;
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
	while(contadorSolicitudes != -1) {
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

	// SI COLA LLENA, SE SALE Y LIBERA MUTEX DE COLA SOLICITUDES, SINO, HACE TODO EL PROCEDIMIENTO
	if(posEspacioVacio != -1) {
		pthread_t hiloSolicitud;
		contadorSolicitudes++;
		colaSolicitudes[posEspacioVacio].id = contadorSolicitudes;
		if(sig == SIGUSR1){
			colaSolicitudes[posEspacioVacio].tipo = 1; 
		} else {
			colaSolicitudes[posEspacioVacio].tipo = 2; 
		}
		colaSolicitudes[posEspacioVacio].hilo = hiloSolicitud;
		pthread_create(&hiloSolicitud, NULL, hiloSolicitudes, (void *)&posEspacioVacio);   
		
	}
	pthread_mutex_unlock(&mutexColaSolicitudes);
}



void *hiloSolicitudes(void *posSolicitud) { 
	pthread_mutex_lock(&mutexColaSolicitudes);
	int posicion =*(int *)posSolicitud;
	pthread_mutex_unlock(&mutexColaSolicitudes);

	char * cad = malloc(12 * sizeof(char));
	char * cad1 = malloc(12 * sizeof(char));

	pthread_mutex_lock(&mutexColaSolicitudes);
	int  n=solicitudes[posicion].id; 
	int n1=solicitudes[posicion].tipo;
	pthread_mutex_unlock(&mutexColaSolicitudes);

	sprintf(cad, "%d", n);
	sprintf(cad1, "%d", n1);

  	pthread_mutex_lock(&mutexLog);  //voy a escribir en en el fichero por tanto mutex
  	writeLogMessage(cad, cad1);
  	pthread_mutex_unlock(&mutexLog);

	sleep(4);

	while(colaSolicitudes[posSolicitud].atendido == 0) {

   		if(solicitudes[posicion].atendido==0){
			printf("No esta siendo atendida %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
			if(solicitudes[posicion].tipo==1){
				printf("Es de invitacion %d\n", posicion);
				if(calculaAleatorios(1, 100)<=10){
					printf("La invitacion se canso\n");
                	                pthread_mutex_lock(&fichero);
					writeLogMessage(cad, cad1);
					pthread_mutex_unlock(&fichero);
 					pthread_mutex_lock(&cola);
					solicitudes[posicion].tipo=0;
					solicitudes[posicion].id=0;
					solicitudes[posicion].atendido=0;
					pthread_mutex_unlock(&cola);
					pthread_exit(NULL);
				}
				if(solicitudes[posicion].atendido==1){
					printf("Esta siendo atendida\n");
					
				}
				
			}
			if(solicitudes[posicion].tipo==2){
				printf("Es de QR  %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
				if(calculaAleatorios(1, 100)<=30){
					printf("La invitacion se rechazo\n");
 					pthread_mutex_lock(&fichero);
					writeLogMessage(cad, cad1);
					pthread_mutex_unlock(&fichero);
					pthread_mutex_lock(&cola);
					solicitudes[posicion].tipo=0;
					solicitudes[posicion].id=0;
					solicitudes[posicion].atendido=0;
					pthread_mutex_unlock(&cola);
					pthread_exit(NULL);
					
				}
			}
			if(calculaAleatorios(1, 100)<=15){
				printf("La invitacion se rechazo porque no es fiable\n");
				pthread_mutex_lock(&fichero);
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&fichero);
				pthread_mutex_lock(&cola);
				solicitudes[posicion].tipo=0;
				solicitudes[posicion].id=0;
				solicitudes[posicion].atendido=0;
				pthread_mutex_unlock(&cola);
				pthread_exit(NULL);
			}                
		}else if(solicitudes[posicion].atendido==1){
			break;		
		}

	}
}


void *hiloAtendedores(void *tipo) { 
	int i;
	pthread_mutex_lock(&atendedore);
	int posicion=*(int *)num;
	pthread_mutex_unlock(&atendedore);
	if(posicion==1){
	   while(1){
		printf("Es el atendedor de solicitudes invitacion %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
		atendedores[posicion-1].tipo=1;
		int mayor=1600;
		int valor=0;
		//busca en la cola de solicitudes
		pthread_mutex_lock(&cola);
                for(i=0; i<15; i++){
//duda como es posible que se puedan atender 3 solicitudes a la vez si solo podemos acceder a la cola de uno en uno??
			if(solicitudes[i].tipo==1){
				if(solicitudes[i].id<mayor && solicitudes[i].id!=0){
					mayor=solicitudes[i].id;
					valor=i;
				}	
			}
			solicitudes[valor].atendido=1;
		/*	pthread_mutex_lock(&fichero);
			writeLogMessage(cad, cad1);
			pthread_mutex_unlock(&fichero);*/
		}
		if(mayor==1600){
			printf("No encontro de su tipo\n");
			for(i=0; i<15; i++){
			    if(solicitudes[i].id<mayor && solicitudes[i].id!=0){
				mayor=solicitudes[i].id;
				valor=i;
				}	
			    }
			solicitudes[valor].atendido=1;
		}
		pthread_mutex_unlock(&cola);
		if(mayor==1600){
			
			sleep(1);
					
		}else{
			int porcentaje=calculaAleatorios(1, 100);
			if(porcentaje<=70){
				printf("La solicitud esta siendo atendida correctamente\n");	
				sleep(calculaAleatorios(1, 4));
			}else if(porcentaje<=90){
				printf("La solicitud esta siendo atendida con errores\n");
				sleep(calculaAleatorios(2, 6));
			}else if(porcentaje<=100){
				printf("La solicitud esta siendo atendida con antecedentes\n");
				sleep(calculaAleatorios(6, 10));
			}
			pthread_mutex_lock(&cola);
			solicitudes[valor].atendido=0;
			//
			pthread_mutex_unlock(&cola);
		}
		
		

          }
	
		
	}else if(posicion==2){
		printf("Es el atendedor de solicitudes qr %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
		atendedores[posicion-1].tipo=2;
	
	}else if(posicion==3){
		printf("Es el atendedor PRO %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
		atendedores[posicion-1].tipo=3;
	}
}


void writeLogMessage(char *id, char *msg) { 
	// Calculamos la hora actual 
	time_t now = time(0); 
	struct tm *tlocal = localtime(&now);
	char stnow[19]; 
	strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal); 
	// Escribimos en el log 
	logFile = fopen("hola.log", "a"); 
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg); 
	fclose(logFile); 
}
