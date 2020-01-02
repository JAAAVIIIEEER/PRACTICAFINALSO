#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h> 
#include <sys/types.h> 
#include <sys/syscall.h> 
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

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
int algoAtendedores(int tipo, char *cad);
int tiempoAtencion(char *cad1, int porcentaje);
void solicitudTramitada(char *cad, char *cad1, int posicion);
int procedimiento(int porcentaje);
void *accionesCoordinador();




int contadorSolicitud;
int contadorActividades;
pthread_mutex_t mutexColaSolicitudes;
pthread_mutex_t mutexLog; 
pthread_mutex_t mutexColaSocial;
pthread_cond_t cond;


typedef struct{
 int id; 
 int atendido; 
 int tipo; 
 pthread_t hilo;
}solicitud;

solicitud * solicitudes;

typedef struct{
  int atendiendo;
  int tipo;
}atendedor;

atendedor * atendedores;

typedef struct{
 int id; 

}social;

social usuarios[4];

FILE *logFile;

int numeroSolicitudes;
int numeroAtendedores;


void manejadora_solicitudes(int sig){
  
  nuevaSolicitud(sig);

}

void manejadora_terminar(int sig){

 //printf("Acabando programa\n");
   pthread_mutex_lock(&mutexColaSolicitudes);
   contadorSolicitud = -1;
   pthread_mutex_unlock(&mutexColaSolicitudes);

}

int main(int argc, char* argv[]){
	printf("%d\n", getpid());
	numeroSolicitudes=15;
	numeroAtendedores=3;
	if(argc==2){	
		numeroSolicitudes=atoi(argv[1]);
	} else if (argc==3){
		numeroSolicitudes=atoi(argv[1]);
		numeroAtendedores=atoi(argv[2]);
	}	
   	int i;

	solicitudes=calloc(numeroSolicitudes,sizeof(solicitud));
	atendedores=calloc(numeroAtendedores,sizeof(atendedor));

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
  	if (pthread_mutex_init(&mutexColaSocial, NULL)!=0){
 		exit(-1);
	}
    	contadorSolicitud=0;
	contadorActividades=0;

	if(pthread_cond_init(&cond, NULL)!=0) exit(-1);
   
	for(i = 0; i < numeroSolicitudes; i++) {
		solicitudes[i].id = 0;
		solicitudes[i].atendido = 0;
		solicitudes[i].tipo = 0;
	}

	for(i = 0; i < numeroAtendedores; i++) {
		atendedores[i].atendiendo=0;
    		atendedores[i].tipo=(i+1);
	}
  	
	for(i = 0; i < 4; i++) {
		usuarios[i].id=0;
                
        }

    	pthread_t trabajadores; 
	
	for(i=0; i<numeroAtendedores;i++){
		pthread_mutex_lock(&mutexColaSocial);
    		pthread_create(&trabajadores, NULL, AccionesAtendedor, (void*)&i);
	}
	pthread_create(&trabajadores, NULL, accionesCoordinador, NULL);


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
  	char * cad = malloc(50 * sizeof(char));
	char * cad1 = malloc(50 * sizeof(char));
	
     	while(solicitudes[i].id!=0){
      		i++;
      		//si llega al final de la cola se para el bucle
      		if(i==numeroSolicitudes){
			break;
		}
    	}
   
     	if(i!=numeroSolicitudes){
      		contadorSolicitud++;
		sprintf(cad, "Solicitud %d", contadorSolicitud);
		sprintf(cad1, "Aniadida");
		pthread_mutex_lock(&mutexLog); 
		writeLogMessage(cad,cad1);
		pthread_mutex_unlock(&mutexLog); 
      		if(sig==SIGUSR1){
        		solicitudes[i].tipo=1;
			sprintf(cad1, "Tipo: Invitación");
      		}else if(sig==SIGUSR2){
        		solicitudes[i].tipo=2;
			sprintf(cad1, "Tipo: QR");
     		}
		pthread_mutex_lock(&mutexLog); 
		writeLogMessage(cad,cad1);
		pthread_mutex_unlock(&mutexLog);
		solicitudes[i].id=contadorSolicitud;
       		pthread_t hiloSolicitud;
       		solicitudes[i].hilo = hiloSolicitud;
       		pthread_create(&hiloSolicitud, NULL, AccionesSolicitud, (void*)&i);
		
    	}
	pthread_mutex_unlock(&mutexColaSolicitudes);  
	
}


pid_t gettid(void) {
	return syscall(__NR_gettid);
} 


void *AccionesSolicitud(void *id){
	pthread_mutex_lock(&mutexColaSolicitudes);
	int posicion =*(int *)id;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	char * cad = malloc(30 * sizeof(char));
	char * cad1 = malloc(30 * sizeof(char));
	pthread_mutex_lock(&mutexColaSolicitudes);
	int  n=solicitudes[posicion].id;
	int n1=solicitudes[posicion].tipo;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(cad, "Solicitud %d", n);
	while(1){
		sleep(4);
		//pthread_mutex_lock(&mutexColaSolicitudes); //duda poner mutex
   		if(solicitudes[posicion].atendido==0){
			//pthread_mutex_unlock(&mutexColaSolicitudes);
			//printf("No esta siendo atendida %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
			//pthread_mutex_lock(&mutexColaSolicitudes);
			if(solicitudes[posicion].tipo==1){
			//pthread_mutex_unlock(&mutexColaSolicitudes);
			//	printf("Es de invitacion %d\n", posicion);
				if(calculaAleatorios(1, 100)<=10){
			//		printf("La invitacion se canso\n");
                         		solicitudRechazada(cad, cad1, posicion);
				}		
			}
			//pthread_mutex_lock(&mutexColaSolicitudes);
			if(solicitudes[posicion].tipo==2){
				//pthread_mutex_unlock(&mutexColaSolicitudes);
			//	printf("Es de QR  %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
				if(calculaAleatorios(1, 100)<=30){
				//	printf("La invitacion se rechazo\n");
 					solicitudRechazada(cad, cad1, posicion);
				}
			}
			if(calculaAleatorios(1, 100)<=15){
			//	printf("La invitacion se rechazo porque no es fiable\n");
				solicitudRechazada(cad, cad1, posicion);
			}                
			//pthread_mutex_lock(&mutexColaSolicitudes);
		}else{
			//pthread_mutex_unlock(&mutexColaSolicitudes);
			while(solicitudes[posicion].atendido==1){
				sleep(1);	
			//	printf("Esta siendo atendida\n");

			}
			
			if(solicitudes[posicion].atendido==2){
				int actividad = calculaAleatorios(1, 2);
				
				if(actividad==1){
					while(contadorActividades==4){
						sleep(3);
					}
					//entra en la cola actividades
					pthread_mutex_lock(&mutexColaSocial);					
					usuarios[contadorActividades++].id=solicitudes[posicion].id;		
					
					sprintf(cad1, "Preparado Actividad");
					pthread_mutex_lock(&mutexLog); 
					writeLogMessage(cad, cad1);
					pthread_mutex_unlock(&mutexLog);
					if(contadorActividades==4){
						pthread_cond_signal(&cond);
					}
					pthread_mutex_unlock(&mutexColaSocial);

					pthread_mutex_lock(&mutexColaSolicitudes);
  					solicitudes[posicion].tipo=0;
  					solicitudes[posicion].id=0;
  					solicitudes[posicion].atendido=0;
  					pthread_mutex_unlock(&mutexColaSolicitudes);
					pthread_exit(NULL);

			
				} else {
					sprintf(cad1, "Tramitada sin actividad");
					pthread_mutex_lock(&mutexLog);	
					writeLogMessage(cad, cad1);
					pthread_mutex_unlock(&mutexLog);
					solicitudTramitada(cad,cad1,posicion);
				}
				
			}else if(solicitudes[posicion].atendido==3){
				sprintf(cad1, "Tramitada sin actividad");
				pthread_mutex_lock(&mutexLog);	
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&mutexLog);	
				solicitudTramitada(cad, cad1, posicion);

			}
			//esperar a que termine
			//decide participar o no en una actividad social
		}

	}

}

void solicitudRechazada(char *cad, char *cad1, int posicion){
	sprintf(cad1, "Abandona la espera");
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
	int tipo=(*(int *)num);
	pthread_mutex_unlock(&mutexColaSocial);
	char * cad = (char*)malloc(200 * sizeof(char));
	char * cad1 = (char*)malloc(200 * sizeof(char));
	int tiempo;
    	while(1){
			int valor = algoAtendedores(tipo, cad);
			if(valor==-1){
				sleep(1);
			}else{
				int porcentaje=calculaAleatorios(1, 100);
				int flag = procedimiento(porcentaje);	
				tiempo = tiempoAtencion(cad1, porcentaje);				
				pthread_mutex_lock(&mutexLog); 
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&mutexLog);
				printf("%d\n", tiempo);
				sleep(tiempo);
				printf("%d B\n", tiempo);
				sprintf(cad1, strcat(cad1, " terminada en %d segundos"), tiempo);
  				pthread_mutex_lock(&mutexColaSolicitudes);
				pthread_mutex_lock(&mutexLog); 
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&mutexLog);					
				solicitudes[valor].atendido=flag;
				atendedores[tipo-1].atendiendo=0;
				pthread_mutex_unlock(&mutexColaSolicitudes);
				contadorVecesAtiende=contadorVecesAtiende+1;
				if(contadorVecesAtiende==5){
					//printf("El atendedor de Invitaciones descansa\n");
					sprintf(cad, "Atendedor %d", tipo);
					sprintf(cad1, "Inicio descanso");
					pthread_mutex_lock(&mutexLog); 
					writeLogMessage(cad, cad1);
					pthread_mutex_unlock(&mutexLog);				
					sleep(10);		
					pthread_mutex_lock(&mutexLog); 
					sprintf(cad1, "Fin descanso");
					writeLogMessage(cad, cad1);
					pthread_mutex_unlock(&mutexLog);
					contadorVecesAtiende=0;			
				}	
			}      
		

			
   	}
}


int procedimiento(int porcentaje){
	if(porcentaje<=70){
		return 2;	
			
	}else if(porcentaje<=90){
		return 2;
		
	}else if(porcentaje<=100){
		return 3;
	}
 }


int algoAtendedores(int tipo, char *cad){
	atendedores[tipo-1].tipo=tipo;
	int i;
	int mayor=1600;
	int valor=-1;
	//busca en la cola de solicitudes
	pthread_mutex_lock(&mutexColaSolicitudes);
	if(tipo!=3){
      	  	for(i=0; i<numeroSolicitudes; i++){
			if(solicitudes[i].tipo==tipo && solicitudes[i].atendido==0){
				if(solicitudes[i].id<mayor && solicitudes[i].id!=0 && solicitudes[i].atendido==0){
					mayor=solicitudes[i].id;
					valor=i;	
				}
				
			}
		
		}
		if(valor==-1){
			for(i=0; i<numeroSolicitudes; i++){
			    if(solicitudes[i].id<mayor && solicitudes[i].id!=0 && solicitudes[i].atendido==0){
				mayor=solicitudes[i].id;
				valor=i;
				}	
			    }
		
		}
		
	}else{
		for(i=0; i<numeroSolicitudes; i++){
	        	if(solicitudes[i].id<mayor && solicitudes[i].id!=0 && solicitudes[i].atendido==0){
				mayor=solicitudes[i].id;
				valor=i;
				}	
		
		}
		
	}
	solicitudes[valor].atendido=1;
	atendedores[tipo-1].atendiendo=1;
	sprintf(cad, "Atendedor %d Atiende solicitud %d", tipo, mayor);
	pthread_mutex_unlock(&mutexColaSolicitudes);
	
	return valor;

}

int tiempoAtencion(char *cad1, int porcentaje){			
			
			int tiempoAtendiendo;
			if(porcentaje<=70){
				//printf("La solicitud esta siendo atendida correctamente\n");	
				tiempoAtendiendo=(calculaAleatorios(1, 4));
				sprintf(cad1, "Atención sin errores");
				
				//puede asociarse a una actividad cultural
			}else if(porcentaje<=90){
				//printf("La solicitud esta siendo atendida con errores\n");
				tiempoAtendiendo=(calculaAleatorios(2, 6));
				sprintf(cad1, "Atención con errores");

				//puede asociarse a una actividad cultural
			}else if(porcentaje<=100){
				//printf("La solicitud esta siendo atendida con antecedentes\n");
				tiempoAtendiendo=(calculaAleatorios(6, 10));
				sprintf(cad1, "Atención por antecedentes");
				//liberaria hueco en la cola de solicitudes y se iria
			}
	return tiempoAtendiendo;
}



void *accionesCoordinador(){
	pthread_t nuevoHilo;
	while(1){
		pthread_mutex_lock(&mutexColaSocial);
		pthread_cond_wait(&cond, &mutexColaSocial);	
		pthread_mutex_lock(&mutexLog);
		writeLogMessage("Actividad", "Comenzando");
		pthread_mutex_unlock(&mutexLog);
		for(int i=0; i<4;i++){
			pthread_create(&nuevoHilo, NULL, actividadCultural, (void*)&usuarios[i].id);
		}
		pthread_cond_wait(&cond, &mutexColaSocial);
		pthread_mutex_lock(&mutexLog);
		writeLogMessage("Actividad", "Finalizando");
		pthread_mutex_unlock(&mutexLog);
		contadorActividades=0;
		int aux;
		for(aux = 0; aux < 3; aux++) {
			usuarios[aux].id=0;     		
       		}	
		pthread_mutex_unlock(&mutexColaSocial);
	}
}

void *actividadCultural(void *id){
	char * cad = malloc(80 * sizeof(char));
	char * cad1 = malloc(80 * sizeof(char));
	int idAux=*(int *)id;
	int i, contador=0;
	sleep(3);
	sprintf(cad1, "Fin Actividad");
	sprintf(cad, "Solicitud %d", idAux);
	pthread_mutex_lock(&mutexLog); 
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);
	for(i=0; i<4;i++){
		if(usuarios[i].id==idAux){
			usuarios[i].id=0;
		}
	}
	for(i=0; i<4;i++){
		if(usuarios[i].id==0){
			contador++;
		}
	}
	if(contador==4){
		pthread_cond_signal(&cond);
	}
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

