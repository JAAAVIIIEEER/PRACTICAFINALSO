#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h> 
#include <sys/types.h> 
#include <sys/syscall.h> 
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

//prototipos 
void *AccionesSolicitud(void *num);
void manejadora_solicitudes(int sig);
void manejadora_terminar(int sig);
void nuevaSolicitud(int sig);
void writeLogMessage(char *id, char *msg);
pid_t gettid(void);
int calculaAleatorios(int min, int max);
void *AccionesAtendedor(void *num);
void solicitudRechazada(char *cad, char *cad1, int posicion);
void *actividadCultural();
int algoAtendedores(int tipo);
int tiempoAtencion(char *cad1);
void solicitudTramitada(char *cad, char *cad1, int posicion);

int contadorSolicitud;
int contadorActividades;
pthread_mutex_t mutexColaSolicitudes;
pthread_mutex_t mutexLog; 
pthread_mutex_t atendedore;
pthread_cond_t cond;


typedef struct{
 int id; 
 int atendido; 
 int tipo; 
 pthread_t hilo;
}solicitud;

solicitud solicitudes[15];

typedef struct{
  int atendiendo;
  int tipo;
}atendedor;

atendedor atendedores[3];

typedef struct{
 int id; 
 int atendido; 
 int tipo; 
}usuario;

usuario usuarios[4];

FILE *logFile;


void manejadora_solicitudes(int sig){
  
  nuevaSolicitud(sig);

}

void manejadora_terminar(int sig){

 printf("Acabando programa\n");
   pthread_mutex_lock(&mutexColaSolicitudes);
   contadorSolicitud = -1;
   pthread_mutex_unlock(&mutexColaSolicitudes);

}

int main(){
   	solicitud s;
   	atendedor a;
   	int aux;	
   	fopen("hola.log", "w"); 

   	struct sigaction solicitudesSen = {0};
   	solicitudesSen.sa_handler=manejadora_solicitudes;
   	struct sigaction terminar = {0};
   	terminar.sa_handler=manejadora_terminar;

    	sigaction(SIGUSR1, &solicitudesSen, NULL);
    	sigaction(SIGUSR2, &solicitudesSen, NULL);
    	sigaction(SIGINT, &terminar, NULL);
    
    	if (pthread_mutex_init(&mutexColaSolicitudes, NULL)!=0){
 		exit(-1);
 	}
   	if (pthread_mutex_init(&mutexLog, NULL)!=0){
 		exit(-1);
	} 
  	if (pthread_mutex_init(&atendedore, NULL)!=0){
 		exit(-1);
	}
    	contadorSolicitud=0;
	contadorActividades=0;

	if(pthread_cond_init(&cond, NULL)!=0) exit(-1);
   
	for(aux = 0; aux < 15; aux++) {
		solicitudes[aux].id = 0;
		solicitudes[aux].atendido = 0;
		solicitudes[aux].tipo = 0;
	}

	for(aux = 0; aux < 3; aux++) {
		atendedores[aux].atendiendo=0;
    		atendedores[aux].tipo=(aux+1);
	}
  

    	pthread_t atendedor1, atendedor2, atendedor3; 
    	int inv=1, qr=2, both=3;

    	pthread_create(&atendedor1, NULL, AccionesAtendedor, (void*)&inv);
    	pthread_create(&atendedor2, NULL, AccionesAtendedor, (void*)&qr);
    	pthread_create(&atendedor3, NULL, AccionesAtendedor, (void*)&both);


   	pthread_mutex_lock(&mutexColaSolicitudes); 
   	while(contadorSolicitud != -1){
   		pthread_mutex_unlock(&mutexColaSolicitudes);
      		pause();
    	}

//terminar todas las solicitudes que hay en la cola

}


void nuevaSolicitud(int sig){
     	pthread_mutex_lock(&mutexColaSolicitudes);
     	int i=0;
  
     	while(solicitudes[i].id!=0){
      		i++;
      		//si llega al final de la cola se para el bucle
      		if(i==15){
			break;
		}
    	}
    	printf("El numero %d\n", i);

    	//se ignora la llamada
     	if(i==15){
         	printf("Se ignora la llamada\n");       
	}else{  
      		contadorSolicitud=contadorSolicitud+1;
      		solicitudes[i].id=contadorSolicitud;
      		if(sig==SIGUSR1){
			printf("Invitacion inv\n");
        		solicitudes[i].tipo=1;
      		}else if(sig==SIGUSR2){
			printf("Invitacion qr\n");
        		solicitudes[i].tipo=2;
     		}
       		pthread_t hiloSolicitud;
       		solicitudes[i].hilo = hiloSolicitud;
       		pthread_create(&hiloSolicitud, NULL, AccionesSolicitud, (void*)&i);

    	}
    	for(int j=0; j<15; j++){
      		printf("El id es: %d\n", solicitudes[j].id);
     	}
	pthread_mutex_unlock(&mutexColaSolicitudes);   
}


pid_t gettid(void) {
	return syscall(__NR_gettid);
} 


void *AccionesSolicitud(void *id){
	//printf("SIUUUUUUU PID=%d, SPID=%d\n", getpid(), gettid());
	pthread_mutex_lock(&mutexColaSolicitudes);
	int posicion =*(int *)id;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	char * cad = malloc(30 * sizeof(char));
	char * cad1 = malloc(30 * sizeof(char));
	pthread_mutex_lock(&mutexColaSolicitudes);
	int  n=solicitudes[posicion].id; //mutex porque puede ser modificado por un atendedor
	int n1=solicitudes[posicion].tipo;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(cad, "Solicitud %d", n);
	sprintf(cad1, "Aniadida");
	pthread_mutex_lock(&mutexLog);  
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);
	if(n1==1){
		sprintf(cad1, "Tipo: Invitación");
	} else {
		sprintf(cad1, "Tipo: QR");
	}
	pthread_mutex_lock(&mutexLog);  //voy a escribir en en el fichero por tanto mutex
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);

	while(1){
		sleep(4);
		//pthread_mutex_lock(&mutexColaSolicitudes); //duda poner mutex
   		if(solicitudes[posicion].atendido==0){
			//pthread_mutex_unlock(&mutexColaSolicitudes);
			printf("No esta siendo atendida %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
			//pthread_mutex_lock(&mutexColaSolicitudes);
			if(solicitudes[posicion].tipo==1){
			//pthread_mutex_unlock(&mutexColaSolicitudes);
				printf("Es de invitacion %d\n", posicion);
				if(calculaAleatorios(1, 100)<=10){
					printf("La invitacion se canso\n");
                         		solicitudRechazada(cad, cad1, posicion);
				}		
			}
			//pthread_mutex_lock(&mutexColaSolicitudes);
			if(solicitudes[posicion].tipo==2){
				//pthread_mutex_unlock(&mutexColaSolicitudes);
				printf("Es de QR  %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
				if(calculaAleatorios(1, 100)<=30){
					printf("La invitacion se rechazo\n");
 					solicitudRechazada(cad, cad1, posicion);
				}
			}
			if(calculaAleatorios(1, 100)<=15){
				printf("La invitacion se rechazo porque no es fiable\n");
				solicitudRechazada(cad, cad1, posicion);
			}                
			//pthread_mutex_lock(&mutexColaSolicitudes);
		}else if(solicitudes[posicion].atendido==1){
			//pthread_mutex_unlock(&mutexColaSolicitudes);
			while(solicitudes[posicion].atendido==1){
				sleep(1);	
				printf("Esta siendo atendida\n");

			}
			printf("Ha acado el atendedor\n");
			solicitudTramitada(cad, cad1, posicion);
			//esperar a que termine
			//decide participar o no en una actividad social
		}

	}

}

void solicitudRechazada(char *cad, char *cad1, int posicion){
	sprintf(cad1, "Rechazada");
	pthread_mutex_lock(&mutexLog);
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);
  	pthread_mutex_lock(&mutexColaSolicitudes);
  	solicitudes[posicion].tipo=0;
  	solicitudes[posicion].id=0;
  	solicitudes[posicion].atendido=0;
  	pthread_mutex_unlock(&mutexColaSolicitudes);
  	pthread_exit(NULL);
}

void solicitudTramitada(char *cad, char *cad1, int posicion){
	sprintf(cad1, "Tramitada");
	pthread_mutex_lock(&mutexLog);
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);
  	pthread_mutex_lock(&mutexColaSolicitudes);
  	solicitudes[posicion].tipo=0;
  	solicitudes[posicion].id=0;
  	solicitudes[posicion].atendido=0;
  	pthread_mutex_unlock(&mutexColaSolicitudes);
  	pthread_exit(NULL);
	//pasar a la cola de actividades
}


void *AccionesAtendedor(void *num){
        int contadorVecesAtiende=0;
	pthread_mutex_lock(&atendedore);
	int tipo=*(int *)num;
	pthread_mutex_unlock(&atendedore);
	char * cad = malloc(30 * sizeof(char));
	char * cad1 = malloc(30 * sizeof(char));
	sprintf(cad, "Atendedor %d", tipo);
	//sprintf(cad1, "Atendiendo");
    	while(1){
		if(tipo==1){
			int valor = algoAtendedores(tipo);
			if(valor==-1){
				sleep(1);
					
			}else{
				int tiempo = tiempoAtencion(cad1);
				printf("Holaaaaa\n");
				pthread_mutex_lock(&atendedore);
  				pthread_mutex_lock(&mutexColaSolicitudes);
				solicitudes[valor].atendido=1;
				atendedores[tipo-1].atendiendo=1;
				pthread_mutex_unlock(&atendedore);
				pthread_mutex_unlock(&mutexColaSolicitudes);
				pthread_mutex_lock(&mutexLog); 
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&mutexLog);
				sleep(tiempo);			
				pthread_mutex_lock(&atendedore);
  				pthread_mutex_lock(&mutexColaSolicitudes);
				solicitudes[valor].atendido=0;
				atendedores[tipo-1].atendiendo=0;
				pthread_mutex_unlock(&atendedore);
				pthread_mutex_unlock(&mutexColaSolicitudes);
				contadorVecesAtiende=contadorVecesAtiende+1;
				if(contadorVecesAtiende==5){
					printf("El atendedor de Invitaciones descansa\n");
					sleep(10);			
				}		
			}
		
	

          	
		}else if(tipo==2){			
			int valor = algoAtendedores(tipo);
			if(valor==-1){		
				sleep(1);
			}else{
				int tiempo = tiempoAtencion(cad1);
				pthread_mutex_lock(&atendedore);
  				pthread_mutex_lock(&mutexColaSolicitudes);
				solicitudes[valor].atendido=1;
				atendedores[tipo-1].atendiendo=1;
				pthread_mutex_unlock(&atendedore);
				pthread_mutex_unlock(&mutexColaSolicitudes);
				pthread_mutex_lock(&mutexLog); 
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&mutexLog);
				sleep(tiempo);			
				pthread_mutex_lock(&atendedore);
  				pthread_mutex_lock(&mutexColaSolicitudes);
				solicitudes[valor].atendido=0;
				atendedores[tipo-1].atendiendo=0;
				pthread_mutex_unlock(&atendedore);
				pthread_mutex_unlock(&mutexColaSolicitudes);
				contadorVecesAtiende=contadorVecesAtiende+1;

				if(contadorVecesAtiende==5){
					printf("El atendedor de qr descansa\n");
					sleep(10);			
				}
			}
          
		}else{
			printf("Es el atendedor PRO %d PID=%d, SPID=%d\n", tipo , getpid(), gettid());
			int valor = algoAtendedores(tipo);
			if(valor==-1){
				sleep(1);			
			}else{
				int tiempo = tiempoAtencion(cad1);
				printf("Holaaaaa\n");
				pthread_mutex_lock(&atendedore);
  				pthread_mutex_lock(&mutexColaSolicitudes);
				solicitudes[valor].atendido=1;
				atendedores[tipo-1].atendiendo=1;
				pthread_mutex_unlock(&atendedore);
				pthread_mutex_unlock(&mutexColaSolicitudes);
				pthread_mutex_lock(&mutexLog); 
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&mutexLog);
				sleep(tiempo);			
				pthread_mutex_lock(&atendedore);
  				pthread_mutex_lock(&mutexColaSolicitudes);
				solicitudes[valor].atendido=0;
				atendedores[tipo-1].atendiendo=0;
				pthread_mutex_unlock(&atendedore);
				pthread_mutex_unlock(&mutexColaSolicitudes);
				contadorVecesAtiende=contadorVecesAtiende+1;
				if(contadorVecesAtiende==5){
					printf("El atendedor pro descansa\n");
					sleep(10);			
				}		
			}	
       		}
   	}
}

int algoAtendedores(int tipo){
	pthread_mutex_lock(&atendedore);
	atendedores[tipo-1].tipo=tipo;
	pthread_mutex_unlock(&atendedore);	
	int i;
	int mayor=1600;
	int valor=-1;
	//busca en la cola de solicitudes
	pthread_mutex_lock(&mutexColaSolicitudes);
	if(tipo!=3){
      	  	for(i=0; i<15; i++){
			if(solicitudes[i].tipo==tipo && solicitudes[i].atendido==0){
				if(solicitudes[i].id<mayor && solicitudes[i].id!=0 && solicitudes[i].atendido==0){
					mayor=solicitudes[i].id;
					valor=i;
				}	
			}
		
		}
		if(valor==-1){
			printf("No encontro de su tipo\n");
			for(i=0; i<15; i++){
			    if(solicitudes[i].id<mayor && solicitudes[i].id!=0 && solicitudes[i].atendido==0){
				mayor=solicitudes[i].id;
				valor=i;
				}	
			    }
		
		}
	}else{
		for(i=0; i<15; i++){
	        	if(solicitudes[i].id<mayor && solicitudes[i].id!=0 && solicitudes[i].atendido==0){
				mayor=solicitudes[i].id;
				valor=i;
				}	
		}
	}
	pthread_mutex_unlock(&mutexColaSolicitudes);
	return valor;

}

int tiempoAtencion(char *cad1){			
			int porcentaje=calculaAleatorios(1, 100);
			int tiempoAtendiendo;
			if(porcentaje<=70){
				printf("La solicitud esta siendo atendida correctamente\n");	
				tiempoAtendiendo=(calculaAleatorios(1, 4));
				sprintf(cad1, "Atención sin errores");
				
				//puede asociarse a una actividad cultural
			}else if(porcentaje<=90){
				printf("La solicitud esta siendo atendida con errores\n");
				tiempoAtendiendo=(calculaAleatorios(2, 6));
				sprintf(cad1, "Fin atención por errores");
				
				//puede asociarse a una actividad cultural
			}else if(porcentaje<=100){
				printf("La solicitud esta siendo atendida con antecedentes\n");
				tiempoAtendiendo=(calculaAleatorios(6, 10));
				sprintf(cad1, "Fin atención por antecedentes");
				
				//liberaria hueco en la cola de solicitudes y se iria
			}
	return tiempoAtendiendo;
}



void *accionesCoordinador(){
	pthread_t nuevoHilo;
	pthread_mutex_lock(&atendedore);
	while(contadorActividades!=4){
		pthread_cond_wait(&cond, &atendedore);
	}	
	pthread_mutex_lock(&mutexLog);
	writeLogMessage("Actividad", "Actividad comenzando");
	pthread_mutex_unlock(&mutexLog);
	for(int i=0; i<4;i++){
		pthread_create(&nuevoHilo, NULL, actividadCultural, NULL);
	}
	pthread_mutex_lock(&mutexLog);
	writeLogMessage("Actividad", "Actividad finalizando");
	pthread_mutex_unlock(&mutexLog);
	contadorActividades=0;
	pthread_mutex_unlock(&atendedore);
}

void *actividadCultural(){
	sleep(3);
	pthread_exit(0);
}


int calculaAleatorios(int min, int max) {
	return rand() % (max-min+1) + min;	
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

