#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h> 
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h> 
#include <string.h>

// Declaración de variables globales.
pthread_mutex_t mutexColaSolicitudes;
pthread_mutex_t mutexColaSocial;
pthread_mutex_t mutexLog;
pthread_cond_t cond;
typedef struct {
	int id;
	int atendido;
	int tipo;
} Solicitud;
typedef struct {
 	int id; 
} Social;
typedef struct {
	pthread_t hilo;
} Atendedor;
Atendedor  * colaAtendedores;
Solicitud * colaSolicitudes;
Social colaSocial[4];
FILE * logFile;
int contadorSolicitudes, contadorActividades, finalizar, numeroSolicitudes, numeroAtendedores;

// Declaración de funciones auxiliares.
void manejadoraNuevaSolicitud(int sig);
void manejadoraTerminar(int sig);
void manejadoraAumentoAtendedores(int sig);
void manejadoraAumentoSolicitudes(int sig);
void nuevoAtendedor(int posEnColaAtendedor);
void *nuevaSolicitud(void *sig);
void *accionesCoordinadorSocial();
void *accionesAtendedor(void *tipoDeAtendedor);
void *actividadCultural(void *id);
void *accionesSolicitud(void *posCola);
int espacioEnColaSolicitudes();
void solicitudRechazada(int posEnCola);
void solicitudTramitada(char *cad, char *cad1, int posicion);
int buscadorPorTipos(int tipo,char * evento);
int buscadorPorTiposAux();
int tiempoAtencion(char *evento, int porcentaje, int posEnColaSolicitud);
int tipoDeAtencion (int porcentaje);
int calculaAleatorios(int min, int max);
void writeLogMessage(char *id, char *msg);

// Función principal.
int main(int argc, char* argv[]) {
	// Declaración de variables locales de la función principal.
	pthread_t coordinador;
	printf("%d\n", getpid());
	int aux;
	// Inicialización de variables locales, globales, condición y de los mutex.
	numeroSolicitudes = 15;
	numeroAtendedores = 3;
	contadorSolicitudes = 0;
	contadorActividades = 0;
	finalizar = 0;
	logFile = NULL;	
	fopen("Tsunami.log", "w");
	if(argc == 2){	
		numeroSolicitudes = atoi(argv[1]);
	} else if (argc == 3){
		numeroSolicitudes = atoi(argv[1]);
		numeroAtendedores = atoi(argv[2]);
	}
	colaSolicitudes = (Solicitud*)malloc(numeroSolicitudes*sizeof(Solicitud));
	colaAtendedores = (Atendedor*)malloc(numeroAtendedores*sizeof(Atendedor));
	struct sigaction solicitudes = {0};
	solicitudes.sa_handler =  manejadoraNuevaSolicitud;
	struct sigaction terminar = {0};
	terminar.sa_handler = manejadoraTerminar;
	struct sigaction nuevosAtendedores = {0};
	nuevosAtendedores.sa_handler = manejadoraAumentoAtendedores;
	struct sigaction nuevasSolicitudes = {0};
	nuevasSolicitudes.sa_handler = manejadoraAumentoSolicitudes;
	if(pthread_mutex_init(&mutexColaSolicitudes,NULL) != 0) {
		exit(-1);
	}
	if(pthread_mutex_init(&mutexColaSocial,NULL) != 0) {
		exit(-1);
	}
	if(pthread_mutex_init(&mutexLog,NULL) != 0) {
		exit(-1);
	}
	if(pthread_cond_init(&cond, NULL) != 0) {
        	exit(-1);
	}
	for(aux = 0; aux < numeroSolicitudes; aux++) {
		(*(colaSolicitudes + aux)).id = 0;
		(*(colaSolicitudes + aux)).atendido = 0;
		(*(colaSolicitudes + aux)).tipo = 0;
	}	
	for(aux = 0; aux < 4; aux++) {
		colaSocial[aux].id = 0;
	}
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
	if(-1 == sigaction(SIGTERM,&nuevosAtendedores,NULL)) {
		perror("Error en el sigaction que recoge la señal SIGTERM.");
		exit(-1);
	}
	if(-1 == sigaction(SIGPIPE,&nuevasSolicitudes,NULL)) {
		perror("Error en el sigaction que recoge la señal SIGPIPE.");
		exit(-1);
	}
	// Creación hilos de los atendedores y del coordinador social.
	pthread_create(&coordinador, NULL, accionesCoordinadorSocial, NULL);
	pthread_mutex_lock(&mutexColaSolicitudes);	
	for(aux = 0; aux < numeroAtendedores;) {
    		nuevoAtendedor(aux);
		pthread_mutex_lock(&mutexColaSolicitudes);
		aux++;	
	}
	// Bucle de señales recibidas hasta que la manejadora de la señal SIGINT cambia el valor de la variable finalizar a 1.
	while(finalizar != 1) { 
		pause();
	}
	// Bucle en el cual se duerme al hilo principal 1 segundo constantemente hasta que algún atendedor detecta que todas las solicitudes han sido procesadas y que la cola esta vacia.
	for(aux = 0; aux < numeroAtendedores; aux++) {
    		pthread_join((*(colaAtendedores + aux)).hilo,NULL);	
	}
	free(colaSolicitudes);
	exit(0);
	
}
// Función manejadora de la señal SIGPIPE.
void manejadoraAumentoSolicitudes(int sig) {
	int aux = numeroSolicitudes;
	char * modificadoSolicitudes = (char *)malloc(200 * sizeof(char));	
	pthread_mutex_lock(&mutexColaSocial);	
	pthread_mutex_lock(&mutexColaSolicitudes);
	printf("Insertar nuevo numero de solicitudes: ");
	scanf("%d",&numeroSolicitudes);
	colaSolicitudes = (Solicitud *) realloc(colaSolicitudes, numeroSolicitudes*sizeof(Solicitud));
	for(aux; aux<numeroSolicitudes; aux++){
		(*(colaSolicitudes + aux)).id = 0;
		(*(colaSolicitudes + aux)).atendido = 0;
		(*(colaSolicitudes + aux)).tipo = 0;	
	}
	pthread_mutex_unlock(&mutexColaSocial);	
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(modificadoSolicitudes, "Modificado a %d", numeroSolicitudes);
	pthread_mutex_lock(&mutexLog);
	writeLogMessage("Numero Solicitudes", modificadoSolicitudes);
	pthread_mutex_unlock(&mutexLog);
}

// Función manejadora de la señal SIGTERM.
void manejadoraAumentoAtendedores(int sig) {
	pthread_t nuevosAtendedores;
	int aux = numeroAtendedores;
	char * modificadoAtendedores = (char *)malloc(200 * sizeof(char));	
	pthread_mutex_lock(&mutexColaSolicitudes);
	printf("Insertar nuevo numero de atendedores: ");
	scanf("%d",&numeroAtendedores);
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(modificadoAtendedores, "Modificado a %d", numeroAtendedores);
	pthread_mutex_lock(&mutexLog);
	writeLogMessage("Numero Atendedores", modificadoAtendedores);
	pthread_mutex_unlock(&mutexLog);
	for(aux; aux < numeroAtendedores; aux++) {
		pthread_mutex_lock(&mutexColaSolicitudes);
    		pthread_create(&nuevosAtendedores, NULL, accionesAtendedor, (void*)&aux);
	}
}

// Función manejadora de la señal SIGINT.
void manejadoraTerminar(int sig) {
	finalizar = 1;
}

// Función manejadora de las señales SIGUSR1 y SIGUSR2.
void manejadoraNuevaSolicitud(int sig) {
	pthread_t hiloProvisional;
	// Solicitamos acceso a la cola colaSolicitudes y bloqueamos para proteger la posición de memoria de la variable sig.
	pthread_mutex_lock(&mutexColaSolicitudes);
	// Se genera un hilo provisional independientemente de si hay espacio en la cola de solicitudes o no. El hilo "padre" vuelve rápidamente a esperar por más señales a la función principal.
	pthread_create(&hiloProvisional, NULL, nuevaSolicitud, (void *)&sig); 
}

// Función dedicada a generar el hilo solicitud en caso de existir espacio libre en la cola de solicitudes.
void *nuevaSolicitud(void *sig) {
	int senal = (*(int *)sig);
	int posEspacioVacio;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	// Comprobación espacio libre en cola, en caso AFIRMATIVO el método isEspacioEnColaSolicitudes() devuelve la posición del espacio libre, por el contrario devuelve el valor -1, no entrando en el if y saliendo del metódo tras desbloquear el mutex.
	pthread_mutex_lock(&mutexColaSolicitudes);
	posEspacioVacio = espacioEnColaSolicitudes();

	if(posEspacioVacio != -1 && finalizar !=1) {
		contadorSolicitudes++;   
		(*(colaSolicitudes + posEspacioVacio)).id = contadorSolicitudes;
		if(senal == SIGUSR1){
			(*(colaSolicitudes + posEspacioVacio)).tipo = 1; // En caso de que la señal tratada sea SIGUSR1 se pone el atributo tipo a 1.
		} else {
			(*(colaSolicitudes + posEspacioVacio)).tipo = 2; // En caso de que la señal tratada sea SIGUSR2 se pone el atributo tipo a 2.
		}
		pthread_t hiloSolicitud;	
 		// Se genera el hilo correspondiente con la variable local hiloSolicitud y se elimina el hilo provisional.
		pthread_create(&hiloSolicitud, NULL, *accionesSolicitud, (void *)&posEspacioVacio);
		pthread_join(hiloSolicitud,NULL);
		pthread_exit(NULL); 
	} else {  
		// Se desbloquea la cola colaSolicitudes si no hay espacio en dicha cola y se elimina el hilo provisional.
		pthread_mutex_unlock(&mutexColaSolicitudes);
		pthread_exit(NULL);
	}
	
}

void *accionesSolicitud(void *posEnCola){
	int posicion =(*(int *)posEnCola);
	char * identificadorSolicitud = (char *)malloc((numeroSolicitudes + 10)* sizeof(char));
	char * eventoSolicitud = (char *)malloc(200 * sizeof(char));
	// Se indica que una nueva solicitud ha sido añadida en la cola.
	sprintf(identificadorSolicitud, "Solicitud_%d", (*(colaSolicitudes + posicion)).id);
	sprintf(eventoSolicitud, "Registrada en la cola de solicitudes. ");
	
	if((*(colaSolicitudes + posicion)).tipo == 1){
		strcat(eventoSolicitud, "Tipo de solicitud: Invitacion.");
	} else {
		strcat(eventoSolicitud, "Tipo de solicitud: QR.");
	}
	// Se desbloquea el acceso a la cola de solicitudes y se bloquea el acceso a los logs.
	pthread_mutex_unlock(&mutexColaSolicitudes);
	pthread_mutex_lock(&mutexLog);  
	writeLogMessage(identificadorSolicitud, eventoSolicitud);
	pthread_mutex_unlock(&mutexLog);
	// Se desbloquea el acceso a los logs.
	while(1) {
		// Se duerme el hilo solicitud 3 segundos.
		sleep(3);
		// Se bloquea el acceso a la cola de solicitudes debido a que se lee. 
		pthread_mutex_lock(&mutexColaSolicitudes);
		// Si la solicitud no está siendo atendida en ese instante, se comprueba si va a ser descartada.
   		if((*(colaSolicitudes + posicion)).atendido == 0) {
			// Se comprueba si la solicitud de tipo invitación va a ser descartada por cansarse de esperar. Probabilidad 10%.
			if((*(colaSolicitudes + posicion)).tipo == 1) {
				if(calculaAleatorios(1, 10) == 1) {
					sprintf(eventoSolicitud, "Descartada del sistema por cansarse de esperar.");
					pthread_mutex_lock(&mutexLog);
					writeLogMessage(identificadorSolicitud, eventoSolicitud);
					pthread_mutex_unlock(&mutexLog);
                         		solicitudRechazada(posicion);
				}		
			}
			// Se comprueba si la solicitud de tipo QR va a ser descartada por no considerarse fiable. Probabilidad 30%.
			if((*(colaSolicitudes + posicion)).tipo == 2) {
				if(calculaAleatorios(1, 10) <= 3){
					sprintf(eventoSolicitud, "Descartada del sistema por no considerarse fiable.");
					pthread_mutex_lock(&mutexLog);
					writeLogMessage(identificadorSolicitud, eventoSolicitud);
					pthread_mutex_unlock(&mutexLog);
                         		solicitudRechazada(posicion);
				}
			}
			// Como la solicitud no ha sido descartada hasta ahora, se comprueba si va a ser descartada por mal funcionamiento de la aplicación. Probabilidad 15%.
			if(calculaAleatorios(1, 20) <= 3){
				sprintf(eventoSolicitud, "Descartada por mal funcionamiento de la aplicación.");
				pthread_mutex_lock(&mutexLog);
				writeLogMessage(identificadorSolicitud, eventoSolicitud);
				pthread_mutex_unlock(&mutexLog);
                         	solicitudRechazada(posicion);
			}                
			pthread_mutex_unlock(&mutexColaSolicitudes);
		} else {
		
			while((*(colaSolicitudes + posicion)).atendido == 1) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				sleep(1);
				pthread_mutex_lock(&mutexColaSolicitudes);	
			}
			if((*(colaSolicitudes + posicion)).atendido == 2) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				int actividad = calculaAleatorios(1, 2);
				if(actividad == 1) {
					while(contadorActividades == 4) {
						sleep(1);
					}
					//entra en la cola actividades
					pthread_mutex_lock(&mutexColaSocial);
					pthread_mutex_lock(&mutexColaSolicitudes);					
					colaSocial[contadorActividades++].id = (*(colaSolicitudes + posicion)).id;		
					sprintf(eventoSolicitud, "Preparado Actividad");
					pthread_mutex_lock(&mutexLog); 
					writeLogMessage(identificadorSolicitud, eventoSolicitud);
					pthread_mutex_unlock(&mutexLog);
					if(contadorActividades==4){
						pthread_cond_signal(&cond);
					}
					pthread_mutex_unlock(&mutexColaSocial);
  					(*(colaSolicitudes + posicion)).tipo=0;
  					(*(colaSolicitudes + posicion)).id=0;
  					(*(colaSolicitudes + posicion)).atendido=0;
  					pthread_mutex_unlock(&mutexColaSolicitudes);
					pthread_exit(NULL);

			
				} else {
					sprintf(eventoSolicitud, "Tramitada sin actividad");
					pthread_mutex_lock(&mutexLog);	
					writeLogMessage(identificadorSolicitud, eventoSolicitud);
					pthread_mutex_unlock(&mutexLog);
					solicitudTramitada(identificadorSolicitud,eventoSolicitud,posicion);
				}
				
			} else if((*(colaSolicitudes + posicion)).atendido == 3) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				sprintf(eventoSolicitud, "Tramitada sin actividad");
				pthread_mutex_lock(&mutexLog);	
				writeLogMessage(identificadorSolicitud, eventoSolicitud);
				pthread_mutex_unlock(&mutexLog);	
				solicitudTramitada(identificadorSolicitud, eventoSolicitud, posicion);

			}
			//esperar a que termine
			//decide participar o no en una actividad social
		}

	}

}

void solicitudRechazada(int posEnCola){
  	(*(colaSolicitudes + posEnCola)).tipo = 0;
  	(*(colaSolicitudes + posEnCola)).id = 0;
  	(*(colaSolicitudes + posEnCola)).atendido = 0;
  	pthread_mutex_unlock(&mutexColaSolicitudes);
  	pthread_exit(NULL);
}

void solicitudTramitada(char *cad, char *cad1, int posicion){
  	pthread_mutex_lock(&mutexColaSolicitudes);
  	(*(colaSolicitudes + posicion)).tipo=0;
  	(*(colaSolicitudes + posicion)).id=0;
  	(*(colaSolicitudes + posicion)).atendido=0;
  	pthread_mutex_unlock(&mutexColaSolicitudes);
  	pthread_exit(NULL);
	//pasar a la cola de actividades
}

void nuevoAtendedor(int posEnColaAtendedor) {
	pthread_t nuevoAtendedor;
	(*(colaAtendedores + posEnColaAtendedor)).hilo = nuevoAtendedor;	
	pthread_create(&nuevoAtendedor, NULL, accionesAtendedor, (void*)&posEnColaAtendedor);		
}

void *accionesAtendedor(void *posEnColaAtendedor) {
        int contadorVecesAtiende = 0, posEnColaSolicitud = 0, porcentaje = 0, flagAtendido = 0, tiempoDeAtencion = 0, posAtendedor = (*(int *)posEnColaAtendedor);
	printf("HOLA ATENDEDOR %d\n", posAtendedor);
	pthread_mutex_unlock(&mutexColaSolicitudes);
	char * identificador = (char *) malloc((10 + numeroAtendedores) * sizeof(char));
	sprintf(identificador, "Atendedor_%d", posAtendedor);
	char * evento = (char *) malloc(200 * sizeof(char));
	
    	while(finalizar != 1 || posEnColaSolicitud != -1) {
		posEnColaSolicitud = buscadorPorTipos(posAtendedor,evento);
		if(posEnColaSolicitud == -1){
			sleep(1);
		} else {
			pthread_mutex_lock(&mutexLog); 
			writeLogMessage(identificador, evento);
			pthread_mutex_unlock(&mutexLog);

			porcentaje = calculaAleatorios(1, 100);
			tiempoDeAtencion = tiempoAtencion(evento, porcentaje,posEnColaSolicitud);
			flagAtendido = tipoDeAtencion(porcentaje);
					
			sleep(tiempoDeAtencion);

			pthread_mutex_lock(&mutexLog); 
			writeLogMessage(identificador, evento);	
			pthread_mutex_unlock(&mutexLog);
			pthread_mutex_lock(&mutexColaSolicitudes);			
			(*(colaSolicitudes + posEnColaSolicitud)).atendido = flagAtendido;
			pthread_mutex_unlock(&mutexColaSolicitudes);
			contadorVecesAtiende++;
			if(contadorVecesAtiende == 5) {
				contadorVecesAtiende=0;
				pthread_mutex_lock(&mutexLog); 
				sprintf(evento, "Inicio descanso");
				writeLogMessage(identificador, evento);
				pthread_mutex_unlock(&mutexLog);				
				sleep(10);		
				pthread_mutex_lock(&mutexLog); 
				sprintf(evento, "Fin descanso");
				writeLogMessage(identificador, evento);
				pthread_mutex_unlock(&mutexLog);			
			}		
		}      
   	}
	pthread_exit(NULL);
}

// Función dedicada a calcular el tipo de atención para cada solicitud según el porcentaje. En solo un 10% de los casos devolverá un valor de 3 lo cual indica que la solicitud tiene antecedentes. 
int tipoDeAtencion(int porcentaje) {
	if(porcentaje <= 90) {
		return 2;	
		
	} else {
		return 3;
	}
 }

// Función dedicada a buscar solicitudes en la cola colaSolicitudes en función de las prioridades de cada atendedor.
int buscadorPorTipos(int tipo, char * evento) {
	int i = 0, posEnCola = 0, aux = 0;
	//VALOR AUX valor del de mayor espera SI ENCUENTRA ALGUNA SOLICITUD
	//VALOR AUX=0 SI NO ENCUENTRA SOLICITUD
	pthread_mutex_lock(&mutexColaSolicitudes);
	if(tipo < 3) {
		for(i = 0; i < numeroSolicitudes; i++) {
			if((*(colaSolicitudes + i)).tipo == tipo && (*(colaSolicitudes + i)).atendido == 0) {
				if(aux == 0 && (*(colaSolicitudes + i)).id>0) {
					posEnCola = i;
					aux = (*(colaSolicitudes + i)).id;
				} else if((*(colaSolicitudes + i)).id < aux && (*(colaSolicitudes + i)).id>0) {
					posEnCola = i;
					aux = (*(colaSolicitudes + i)).id;			
				}			
			}	
		}
		if(aux == 0) {
			if(tipo == 1) {
				posEnCola = buscadorPorTiposAux();
			} else {
				posEnCola = buscadorPorTiposAux();
			}
		}
	} else {
		for(i = 0; i < numeroSolicitudes; i++) {
			if((*(colaSolicitudes + i)).id != 0 && (*(colaSolicitudes + i)).atendido == 0) {
				if(aux == 0 && (*(colaSolicitudes + i)).id>0) {
					posEnCola = i;
					aux = (*(colaSolicitudes + i)).id;
				} else if((*(colaSolicitudes + i)).id < aux && (*(colaSolicitudes + i)).id>0) {
					posEnCola = i;
					aux = (*(colaSolicitudes + i)).id;
				}
			}
		}
		if(aux == 0) {
			posEnCola = -1;
		}
	}
	if(posEnCola == -1) {
		pthread_mutex_unlock(&mutexColaSolicitudes);
		return -1;
	}
	(*(colaSolicitudes + posEnCola)).atendido = 1;
	sprintf(evento, "Atendiendo Solicitud_%d...",(*(colaSolicitudes + posEnCola)).id);
	pthread_mutex_unlock(&mutexColaSolicitudes);
	return posEnCola;
}

int buscadorPorTiposAux() {
	int i = 0, aux = 0, posEnCola = 0;
	for(i = 0; i < numeroSolicitudes; i++) {
			if((*(colaSolicitudes + i)).atendido == 0) {
				if(aux == 0 && (*(colaSolicitudes + i)).id>0) {
					posEnCola = i;
					aux = (*(colaSolicitudes + i)).id;
				} else if((*(colaSolicitudes + i)).id < aux && (*(colaSolicitudes + i)).id>0) {
					posEnCola = i;	
					aux = (*(colaSolicitudes + i)).id;
				}
			}	
	}
	if(aux == 0) {
		return -1;		
	}
	return posEnCola;
}

// Función dedicada a calcular el tiempo de atención en función del porcentaje calculado previamente.
int tiempoAtencion(char *evento, int porcentaje, int posEnColaSolicitud) {
	int tiempoAtendiendo;
	if(porcentaje <= 70) {
		tiempoAtendiendo = calculaAleatorios(1, 4);
		sprintf(evento,"Solicitud_%d atendida correctamente en %d segundos.",(*(colaSolicitudes + posEnColaSolicitud)).id,tiempoAtendiendo);
	} else if(porcentaje > 70 && porcentaje <= 90) {
		tiempoAtendiendo = calculaAleatorios(2, 6);
		sprintf(evento,"Solicitud_%d atendida con errores en %d segundos.",(*(colaSolicitudes + posEnColaSolicitud)).id,tiempoAtendiendo);
	}else if(porcentaje > 90) {
		tiempoAtendiendo = calculaAleatorios(6, 10);
		sprintf(evento,"Solicitud_%d atendida con antecedentes en %d segundos.",(*(colaSolicitudes + posEnColaSolicitud)).id,tiempoAtendiendo);
	}
	return tiempoAtendiendo;
}



void *accionesCoordinadorSocial(){
	pthread_t nuevoHilo;
	while(1){
		int i;
		pthread_mutex_lock(&mutexColaSocial);
		pthread_cond_wait(&cond, &mutexColaSocial);	
		pthread_mutex_lock(&mutexLog);
		writeLogMessage("Actividad", "Comenzando");
		pthread_mutex_unlock(&mutexLog);
		for(i=0; i<4;i++){
			pthread_create(&nuevoHilo, NULL, actividadCultural, (void*)&colaSocial[i].id);
		}
		pthread_cond_wait(&cond, &mutexColaSocial);
		pthread_mutex_lock(&mutexLog);
		writeLogMessage("Actividad", "Finalizando");
		pthread_mutex_unlock(&mutexLog);
		contadorActividades=0;
		int aux;
		for(i = 0; i < 3; i++) {
			colaSocial[i].id=0;     		
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
		if(colaSocial[i].id==idAux){
			colaSocial[i].id=0;
		}
	}
	for(i=0; i<4;i++){
		if(colaSocial[i].id==0){
			contador++;
		}
	}
	if(contador==4){
		pthread_cond_signal(&cond);
	}
	pthread_exit(0);
}

// Función dedicada a buscar huecos en la cola de colaSolicitudes. Si encuentra alguno devuelve la posición del encontrado, sino, devuelve -1.
int espacioEnColaSolicitudes() {
	int aux;
	for(aux = 0; aux < numeroSolicitudes; aux++) {
		if((*(colaSolicitudes + aux)).id == 0) {
			return aux;
		}
	}
	return -1;
}

int calculaAleatorios(int min, int max) {
	return rand() % (max - min + 1) + min;	
}

void writeLogMessage(char *id, char *msg) { 
	// Se calcula la hora actual.
	time_t now = time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal);
	// Se escribe en el log. 
	logFile = fopen("Tsunami.log", "a");
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
	fclose(logFile);
	printf("[%s] %s: %s\n", stnow, id, msg);
}

