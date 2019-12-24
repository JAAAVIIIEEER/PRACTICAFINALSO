#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h> 
#include <sys/syscall.h> 

// Declaración variables globales
pthread_mutex_t mutexColaSolicitudes;
pthread_mutex_t mutexColaSocial;
pthread_mutex_t mutexLog;
pthread_mutex_t mutexColaAtendedores;

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

Atendedor colaAtendedores[3];
Solicitud colaSolicitudes[15];
Solicitud colaSocial[4];

FILE * logFile;
int contadorSolicitudes;

// Declaración funciones auxiliares
void manejadoraSolicitudes(int sig);
void manejadoraTerminar(int sig);
void *hiloSolicitudes(void *solicitud);
void *hiloAtendedores(void *tipo);
int isEspacioEnColaSolicitudes();
int calculaAleatorios(int min, int max);
void solicitudRechazada(char *cad, char *cad1, int posicion);
pid_t gettid(void);
void writeLogMessage(char *id, char *msg);

// Función principal.
int main(int argc, char* argv[]) {
	// Declaración variables locales de la función principal.
	pthread_t atendedorQR, atendedorInvitacion, atendedorPRO, coordinador;
	struct sigaction solicitudes = {0};
	struct sigaction terminar = {0};
	int aux;	
	// Inicialización de algunas variables locales y de otras globales.
	fopen("hola.log", "w"); 
	contadorSolicitudes = 0;
	logFile = NULL;	
	solicitudes.sa_handler =  manejadoraSolicitudes;
	terminar.sa_handler = manejadoraTerminar;
	pthread_mutex_init(&mutexColaSolicitudes,NULL);
	pthread_mutex_init(&mutexColaAtendedores,NULL);
	pthread_mutex_init(&mutexColaSocial,NULL);
	pthread_mutex_init(&mutexLog,NULL);
	pthread_cond_t no_lleno;
	pthread_cond_init(&no_lleno, NULL);
	for(aux = 0; aux < 15; aux++) {
		colaSolicitudes[aux].id = 0;
		colaSolicitudes[aux].atendido = 0;
		colaSolicitudes[aux].tipo = 0;
	}
	for(aux = 0; aux < 4; aux++) {
		colaSocial[aux].id = 0;
		colaSocial[aux].atendido = 0;
		colaSocial[aux].tipo = 0;
	}
	// Creación hilos de los atendedores.
	pthread_create(&atendedorInvitacion, NULL, hiloAtendedores, "0");
	pthread_create(&atendedorQR, NULL, hiloAtendedores, "1");
	pthread_create(&atendedorPRO, NULL, hiloAtendedores, "2");
	// Tratamiento de las señales recibidas mediante sigaction.
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
	// Bucle de señales recibidas hasta que la manejadora de la señal SIGINT cambia el valor de contadorSolicitudes a -1.
	pthread_mutex_lock(&mutexColaSolicitudes); 
	while(contadorSolicitudes != -1) {
	pthread_mutex_unlock(&mutexColaSolicitudes); 
		pause();
	}

	exit(0);
	
}

// Función dedicada a buscar huecos en la cola de solicitudes. Si encuentra alguno devuelve la posición del encontrado, sino, devuelve -1.
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

// Función manejadora de la señal SIGINT.
void manejadoraTerminar(int sig) {
	contadorSolicitudes = -1;
}

// Función manejadora de las señales SIGUSR1 y SIGUSR2.
void manejadoraSolicitudes(int sig){
	pthread_mutex_lock(&mutexColaSolicitudes);  // Bloqueo cola solicitudes.
	// Comprobación espacio libre en cola, en caso AFIRMATIVO el método isEspacioEnColaSolicitudes() devuelve la posición del espacio libre, por el contrario, en caso NEGATIVO devuelve -1, no entrando en el if y saliendo del metódo tras desbloquear el mutex.
	int posEspacioVacio = isEspacioEnColaSolicitudes();
	if(posEspacioVacio != -1) {
		pthread_t hiloSolicitud; // Se crear una variable local que genera el tid de un hilo nuevo.
		contadorSolicitudes++;   
		colaSolicitudes[posEspacioVacio].id = contadorSolicitudes;
		if(sig == SIGUSR1){
			colaSolicitudes[posEspacioVacio].tipo = 1; // En caso de que la señal tratada sea SIGUSR1 se pone el atributo tipo a 1.
		} else {
			colaSolicitudes[posEspacioVacio].tipo = 2; // En caso de que la señal tratada sea SIGUSR2 se pone el atributo tipo a 2.
		}
		colaSolicitudes[posEspacioVacio].hilo = hiloSolicitud; 
 		// Se genera el hilo correspondiente con la variable local hiloSolicitud.
		pthread_create(&hiloSolicitud, NULL, hiloSolicitudes, (void *)&posEspacioVacio); 
	} else {
		// Se desbloquea la cola de solicitudes si no hay espacio en la cola disponible.
		pthread_mutex_unlock(&mutexColaSolicitudes);
	}
}

// Función dedicada para los hilos solicitudes.
void *hiloSolicitudes(void *posEspacioVacio) {
	//printf("SIUUUUUUU PID=%d, SPID=%d\n", getpid(), gettid());
	int posicion =*(int *)posEspacioVacio;
	// Se desbloquea la cola de solicitudes solo si se ha asegurado que el hilo creado en la manejadoraSolicitudes ha obtenido en su función su valor de posición y no el de otro hilo solicitud que pudiera llegar inmediatamente después, ya que el padre puede seguir recibiendo señales.
	pthread_mutex_unlock(&mutexColaSolicitudes); 

	char * cad = malloc(12 * sizeof(char));
	char * cad1 = malloc(12 * sizeof(char));
 	// Se bloquea la cola de solicitudes para que un hilo atendedor no se introduzca a la vez que un solicitante. 
	pthread_mutex_lock(&mutexColaSolicitudes);
	int n = colaSolicitudes[posicion].id;
	int n1 = colaSolicitudes[posicion].tipo;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(cad, "%d", n);
	sprintf(cad1, "%d", n1);
  	pthread_mutex_lock(&mutexLog);  //voy a escribir en en el fichero por tanto mutex
  	writeLogMessage(cad, cad1);
  	pthread_mutex_unlock(&mutexLog);

	while(1) {
		sleep(4);
   		if(colaSolicitudes[posicion].atendido == 0){
			printf("El hilo solicitud %d colocado en la posición %d no está siendo atendido.\n", gettid(), posicion);
			if(colaSolicitudes[posicion].tipo==1){
				printf("El hilo solicitud %d colocado en la posición %d es de invitación.\n", gettid(), posicion);
				if(calculaAleatorios(1, 100) <= 10) {
					printf("El hilo solicitud %d colocado en la posición %d se ha cansado de esperar.\n", gettid(), posicion);
                	         	solicitudRechazada(cad, cad1, posicion);
				}
				
			}		
	
			if(colaSolicitudes[posicion].tipo == 2){
				printf("El hilo solicitud %d colocado en la posición %d es de QR.\n", gettid(), posicion);
				if(calculaAleatorios(1, 100)<=30){
					printf("El hilo solicitud %d colocado en la posición %d se ha rechazado.\n", gettid(), posicion);
 					solicitudRechazada(cad, cad1, posicion); 
		
				}
			}
			if(calculaAleatorios(1, 100) <= 15){
					printf("El hilo solicitud %d colocado en la posición %d se ha rechazado porque no es fiable\n", gettid(), posicion);
					solicitudRechazada(cad, cad1, posicion);
			}                
		}else if(colaSolicitudes[posicion].atendido == 1){
			while(colaSolicitudes[posicion].atendido == 1){} // Hilo solicitud espera hasta dejar de ser atendido.
			
		}			
			//// COLA SOCIAL	
	}
 }

void solicitudRechazada(char *cad, char *cad1, int posicion){
	pthread_mutex_lock(&mutexLog);
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);
	pthread_mutex_lock(&mutexColaSolicitudes);
	colaSolicitudes[posicion].tipo=0;
	colaSolicitudes[posicion].id=0;
	colaSolicitudes[posicion].atendido=0;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	pthread_exit(NULL);
  }


void *hiloAtendedores(void *num){
        int i;
	pthread_mutex_lock(&mutexColaAtendedores);
	int posicion = *(int *)num;
	pthread_mutex_unlock(&mutexColaAtendedores);
	if(posicion==1){
	   while(1){
		printf("Es el atendedor de solicitudes invitacion %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
		colaAtendedores[posicion-1].tipo=1;
		int mayor=1600;
		int valor=0;
		//busca en la cola de solicitudes
		pthread_mutex_lock(&mutexColaSolicitudes);
                for(i=0; i<15; i++){
			if(colaSolicitudes[i].tipo==1){
				if(colaSolicitudes[i].id<mayor && colaSolicitudes[i].id!=0){
					mayor=colaSolicitudes[i].id;
					valor=i;
				}	
			}
			colaSolicitudes[valor].atendido=1;
		/*	pthread_mutex_lock(&mutexLog);
			writeLogMessage(cad, cad1);
			pthread_mutex_unlock(&mutexLog);*/
		}
		if(mayor==1600){
			printf("No encontro de su tipo\n");
			for(i=0; i<15; i++){
			    if(colaSolicitudes[i].id < mayor && colaSolicitudes[i].id != 0){
				mayor=colaSolicitudes[i].id;
				valor=i;
				}	
			    }
			colaSolicitudes[valor].atendido=1;
		}
		pthread_mutex_unlock(&mutexColaSolicitudes);
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
			pthread_mutex_lock(&mutexColaSolicitudes);
			colaSolicitudes[valor].atendido=0;
			pthread_mutex_unlock(&mutexColaSolicitudes);
		}
		
		

          }
	
		
	}else if(posicion==2){
		printf("Es el atendedor de solicitudes qr %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
		colaAtendedores[posicion-1].tipo=2;
	
	}else if(posicion==3){
		printf("Es el atendedor PRO %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
		colaAtendedores[posicion-1].tipo=3;
	}
}


int calculaAleatorios(int min, int max) {
	return rand() % (max-min+1) + min;	
}

pid_t gettid(void) {
   return syscall(__NR_gettid);
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
