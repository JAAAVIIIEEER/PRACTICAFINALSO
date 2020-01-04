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

// Declaración variables globales.

pthread_mutex_t mutexColaSolicitudes;
pthread_mutex_t mutexColaSocial;
pthread_mutex_t mutexLog;
pthread_cond_t cond;

typedef struct {
	int id;
	int atendido;
	int tipo;
	pthread_t hilo;
} Solicitud;

typedef struct {
	int atendiendo;
  	int tipo;
} Atendedor;

typedef struct {
 	int id; 
} Social;

Atendedor  * colaAtendedores;
Solicitud * colaSolicitudes;
Social colaSocial[4];

FILE * logFile;

int contadorSolicitudes, contadorActividades, finalizar, numeroSolicitudes, numeroAtendedores;

// Declaración funciones auxiliares.
void manejadoraNuevaSolicitud(int sig);
void manejadoraTerminar(int sig);
void manejadoraAumentoAtendedores(int sig);
void manejadoraAumentoSolicitudes(int sig);
void *nuevaSolicitud(void *sig);
void *accionesCoordinadorSocial();
void *accionesAtendedor(void *tipo);
void *actividadCultural(void *id);
void *accionesSolicitud(void *posCola);
int espacioEnColaSolicitudes();
int colaSolicitudesVacia();
void solicitudRechazada(char *cad, char *cad1, int posicion);
void solicitudTramitada(char *cad, char *cad1, int posicion);
int buscadorPorPrioridades(int tipo, char *cad);
int tiempoAtencion(char *cad1, int porcentaje);
int tipoDeAtencion (int porcentaje);
int calculaAleatorios(int min, int max);
void writeLogMessage(char *id, char *msg);

// Función principal.
int main(int argc, char* argv[]) {
	// Declaración variables locales de la función principal.
	pthread_t trabajadores;
	int aux;
	// Inicialización de variables locales, globales, condición y de los mutex.
	numeroSolicitudes = 15;
	numeroAtendedores = 3;
	if(argc == 2){	
		numeroSolicitudes = atoi(argv[1]);
	} else if (argc == 3){
		numeroSolicitudes = atoi(argv[1]);
		numeroAtendedores = atoi(argv[2]);
	}
	colaSolicitudes = (Solicitud*)malloc(numeroSolicitudes*sizeof(Solicitud));
	colaAtendedores = (Atendedor*)malloc(numeroAtendedores*sizeof(Atendedor));

   	fopen("hola.log", "w");
	contadorSolicitudes = 0;
	contadorActividades = 0;
	finalizar = 0;
	logFile = NULL;	
	struct sigaction solicitudes = {0};
	solicitudes.sa_handler =  manejadoraNuevaSolicitud;
	struct sigaction terminar = {0};
	terminar.sa_handler = manejadoraTerminar;
	//struct sigaction nuevosAtendedores = {0};
	//nuevosAtendedores.sa_handler = manejadoraAumentoAtendedores;
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
	for(aux = 0; aux < numeroAtendedores; aux++) {
		(*(colaAtendedores + aux)).atendiendo = 0;
		(*(colaAtendedores + aux)).tipo = aux + 1; 
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
	/*if(-1 == sigaction(SIGKILL,&nuevosAtendedores,NULL)) {
		perror("Error en el sigaction que recoge la señal SIGKILL.");
		exit(-1);
	}*/
	if(-1 == sigaction(SIGPIPE,&nuevasSolicitudes,NULL)) {
		perror("Error en el sigaction que recoge la señal SIGPIPE.");
		exit(-1);
	}
	// Creación hilos de los atendedores.
	for(aux = 1; aux <= numeroAtendedores; aux++){
    		pthread_create(&trabajadores, NULL, accionesAtendedor, (void*)&aux);
	}
	pthread_create(&trabajadores, NULL, accionesCoordinadorSocial, NULL);	
////////////////////////////////////////
	//// FALTAN JOINNSSSSSSSSS ///////
///////////////////////////////////////////
	// Bucle de señales recibidas hasta que la manejadora de la señal SIGINT cambia el valor de la variable finalizar a 1.
	while(finalizar != 1) { 
		pause();
	}
	free(colaSolicitudes);
	free(colaAtendedores);

	exit(0);
	
}
// Función manejadora de la señal SIGPIPE.
void manejadoraAumentoSolicitudes(int sig) {
	int aux;
	pthread_mutex_lock(&mutexColaSocial);	
	pthread_mutex_lock(&mutexColaSolicitudes);
	printf("Insertar nuevo numero de solicitudes : ");
	scanf("%d",&numeroSolicitudes);
	colaSolicitudes = (Solicitud *) realloc(colaSolicitudes, numeroSolicitudes);
	pthread_mutex_unlock(&mutexColaSocial);	
	pthread_mutex_unlock(&mutexColaSolicitudes);
}

// Función manejadora de la señal SIGKILL.
/*void manejadoraAumentoAtendedores(int sig) {
	/*pthread_t nuevosAtendedores;
	int nuevoNumeroAtendedores,aux,i = 2;
	pthread_mutex_lock(&mutexColaSocial);	
	pthread_mutex_lock(&mutexColaSolicitudes);
	printf("Insertar nuevo numero de atendedores : \n");
	scanf("%d",&nuevoNumeroAtendedores);
	colaAtendedores = (Atendedor *) realloc(colaAtendedores, numeroAtendedores);
	pthread_mutex_unlock(&mutexColaSocial);	
	pthread_mutex_unlock(&mutexColaSolicitudes);
	for(aux = 0; aux < nuevoNumeroAtendedores; aux++){
		pthread_mutex_lock(&mutexColaSocial);
    		pthread_create(&nuevosAtendedores, NULL, accionesAtendedor, (void*)&i);
	}
}*/

// Función manejadora de la señal SIGINT.
void manejadoraTerminar(int sig) {
	finalizar = 1;
}

// Función manejadora de las señales SIGUSR1 y SIGUSR2.
void manejadoraNuevaSolicitud(int sig) {
	pthread_t hiloProvisional;
	// Se genera un hilo provisional independientemente de si hay espacio en la cola de solicitudes o no. El hilo "padre" vuelve rápidamente a esperar por más señales a la función principal.
	pthread_create(&hiloProvisional, NULL, nuevaSolicitud, (void *)&sig); 
}

// Función dedicada a generar el hilo solicitud en caso de existir espacio libre en la cola de solicitudes.
void *nuevaSolicitud(void *sig) {
	printf("hola\n");
	// Solicitamos acceso a la cola colaSolicitudes.
	pthread_mutex_lock(&mutexColaSolicitudes);  
	printf("pene\n");
	// Comprobación espacio libre en cola, en caso AFIRMATIVO el método isEspacioEnColaSolicitudes() devuelve la posición del espacio libre, por el contrario devuelve el valor -1, no entrando en el if y saliendo del metódo tras desbloquear el mutex.
	int posEspacioVacio = espacioEnColaSolicitudes();

	if(posEspacioVacio != -1) {
		printf("pe %d \n",posEspacioVacio);
		contadorSolicitudes++;   
		(*(colaSolicitudes + posEspacioVacio)).id = contadorSolicitudes;
		if(*(int *)sig == SIGUSR1){
			(*(colaSolicitudes + posEspacioVacio)).tipo = 1; // En caso de que la señal tratada sea SIGUSR1 se pone el atributo tipo a 1.
		} else {
			(*(colaSolicitudes + posEspacioVacio)).tipo = 2; // En caso de que la señal tratada sea SIGUSR2 se pone el atributo tipo a 2.
		}
		printf("%d\n",posEspacioVacio);
		pthread_t hiloSolicitud;
		(*(colaSolicitudes + posEspacioVacio)).hilo = hiloSolicitud; 
		// MENSAJE Y LOG DE AÑADIDA DEL TIPO X.
		pthread_mutex_unlock(&mutexColaSolicitudes);
 		// Se genera el hilo correspondiente con la variable local hiloSolicitud y se elimina el hilo provisional.
		pthread_create(&hiloSolicitud, NULL, *accionesSolicitud, (void *)&posEspacioVacio);
		pthread_exit(NULL); 
	} else {
		// Se desbloquea la cola colaSolicitudes si no hay espacio en dicha cola y se elimina el hilo provisional.
		pthread_mutex_unlock(&mutexColaSolicitudes);
		pthread_exit(NULL);
	}
	
}

void *accionesSolicitud(void *posEnCola){
	int posicion =*(int *)posEnCola;
//LOG
	char * cad = malloc(30 * sizeof(char));
	char * cad1 = malloc(30 * sizeof(char));
	pthread_mutex_lock(&mutexColaSolicitudes);
	int  n = (*(colaSolicitudes + posicion)).id; //mutex porque puede ser modificado por un atendedor
	int n1 = (*(colaSolicitudes + posicion)).tipo;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(cad, "Solicitud %d", n);
	sprintf(cad1, "Aniadida");
	pthread_mutex_lock(&mutexLog);  
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);
	if(n1 == 1){
		sprintf(cad1, "Tipo: Invitación");
	} else {
		sprintf(cad1, "Tipo: QR");
	}
	pthread_mutex_lock(&mutexLog);  
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);
//LOG
	while(1){
		sleep(4);
		pthread_mutex_lock(&mutexColaSolicitudes);
   		if((*(colaSolicitudes + posicion)).atendido == 0) {
			if((*(colaSolicitudes + posicion)).tipo == 1) {
				printf("Es de invitacion %d\n", posicion);
				if(calculaAleatorios(1, 100)<=10){
					//printf("La invitacion se canso\n");
                         		solicitudRechazada(cad, cad1, posicion);
				}		
			}

			if((*(colaSolicitudes + posicion)).tipo == 2) {
				
				//printf("Es de QR  %d PID=%d, SPID=%d\n", posicion , getpid(), gettid());
				if(calculaAleatorios(1, 100)<=30){
				//	printf("La invitacion se rechazo\n");
 					solicitudRechazada(cad, cad1, posicion);
				}
			}
			if(calculaAleatorios(1, 100)<=15){
			//	printf("La invitacion se rechazo porque no es fiable\n");
				solicitudRechazada(cad, cad1, posicion);
			}                
			pthread_mutex_unlock(&mutexColaSolicitudes);
		}else{
			while((*(colaSolicitudes + posicion)).atendido == 1) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				sleep(1);
				pthread_mutex_lock(&mutexColaSolicitudes);	
				printf("Esta siendo atendida\n");
			}
			
			if((*(colaSolicitudes + posicion)).atendido == 2) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				int actividad = calculaAleatorios(1, 2);
				
				if(actividad == 1) {
					while(contadorActividades == 4) {
						sleep(3);
					}
					//entra en la cola actividades
					pthread_mutex_lock(&mutexColaSocial);
					pthread_mutex_lock(&mutexColaSolicitudes);					
					colaSocial[contadorActividades++].id = (*(colaSolicitudes + posicion)).id;		
					
					sprintf(cad1, "Preparado Actividad");
					sprintf(cad, "Solicitud %d", (*(colaSolicitudes + posicion)).id);
					pthread_mutex_lock(&mutexLog); 
					writeLogMessage(cad, cad1);
					pthread_mutex_unlock(&mutexLog);
					printf("ContadorActividades %d\n", contadorActividades);
					if(contadorActividades==4){
						pthread_cond_signal(&cond);
						printf("Hola coordinador\n");
					}
					pthread_mutex_unlock(&mutexColaSocial);
  					(*(colaSolicitudes + posicion)).tipo=0;
  					(*(colaSolicitudes + posicion)).id=0;
  					(*(colaSolicitudes + posicion)).atendido=0;
  					pthread_mutex_unlock(&mutexColaSolicitudes);
					pthread_exit(NULL);

			
				} else {
					sprintf(cad1, "Tramitada sin actividad");
					pthread_mutex_lock(&mutexLog);	
					writeLogMessage(cad, cad1);
					pthread_mutex_unlock(&mutexLog);
					solicitudTramitada(cad,cad1,posicion);
				}
				
			} else if((*(colaSolicitudes + posicion)).atendido == 3) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
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
	sprintf(cad1, "Rechazada");
	pthread_mutex_lock(&mutexLog);
	writeLogMessage(cad, cad1);
	pthread_mutex_unlock(&mutexLog);
  	pthread_mutex_lock(&mutexColaSolicitudes);
  	(*(colaSolicitudes + posicion)).tipo=0;
  	(*(colaSolicitudes + posicion)).id=0;
  	(*(colaSolicitudes + posicion)).atendido=0;
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

void *accionesAtendedor(void *tipoDeAtendedor){
        int contadorVecesAtiende = 0, colaVacia = 0,posEnCola = 0, porcentaje = 0, flag = 0, tiempoDeAtencion = 0;
	int tipo = (*(int *)tipoDeAtendedor);
	char * cad = (char*)malloc(4000 * sizeof(char));
	char * cad1 = (char*)malloc(4000 * sizeof(char));

    	while(finalizar != 1 || colaVacia != 1) {
		colaVacia = colaSolicitudesVacia();
		if(colaVacia == 1){
			sleep(1);
		} else {
			posEnCola = buscadorPorPrioridades(tipo, cad);
			porcentaje = calculaAleatorios(1, 100);
			flag = tipoDeAtencion(porcentaje);	
			tiempoDeAtencion = tiempoAtencion(cad1, porcentaje);
			pthread_mutex_lock(&mutexLog); 
			writeLogMessage(cad, cad1);
			pthread_mutex_unlock(&mutexLog);
					
			sleep(tiempoDeAtencion);

			sprintf(cad1, strcat(cad1, " terminada"));
			pthread_mutex_lock(&mutexColaSolicitudes);
			pthread_mutex_lock(&mutexLog); 
			writeLogMessage(cad, cad1);				
			(*(colaSolicitudes + posEnCola)).atendido = flag;
			(*(colaAtendedores + (tipo - 1))).atendiendo = 0;
			pthread_mutex_unlock(&mutexLog);
			pthread_mutex_unlock(&mutexColaSolicitudes);

			contadorVecesAtiende++;
			if(contadorVecesAtiende==5){
				contadorVecesAtiende=0;
				//printf("El atendedor de Invitaciones descansa\n");
				pthread_mutex_lock(&mutexLog); 
				sprintf(cad1, "Inicio descanso");
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&mutexLog);				
				sleep(10);		
				pthread_mutex_lock(&mutexLog); 
				sprintf(cad1, "Fin descanso");
				writeLogMessage(cad, cad1);
				pthread_mutex_unlock(&mutexLog);			
			}		
		}      
   	}
}

// Función dedicada a calcular el tipo de atención para cada solicitud según el porcentaje. En solo un 10% de los casos devolverá un valor de 3 lo cual indica que la solicitud tiene antecedentes. 
int tipoDeAtencion(int porcentaje) {
	if(porcentaje <= 90) {
		return 2;	
		
	} else {
		return 3;
	}
 }
//  A OPTIMIZAR, NO HE VISTO MAYOR CUTRADA EN MI VIDA, MAYOR = 1600?????????????
// Función dedicada a buscar las solicitudes en la cola colaSolicitudes en función de las prioridades.
int buscadorPorPrioridades(int tipo, char *cad){
	(*(colaAtendedores + (tipo-1))).tipo=tipo;
	int i;
	int mayor=1600;
	int valor=-1;
	//busca en la cola de solicitudes
	pthread_mutex_lock(&mutexColaSolicitudes);
	if(tipo < 3){
      	  	for(i=0; i<numeroSolicitudes; i++){
			if((*(colaSolicitudes + i)).tipo==tipo && (*(colaSolicitudes + i)).atendido==0){
				if((*(colaSolicitudes + i)).id<mayor && (*(colaSolicitudes + i)).id>0){
					mayor=(*(colaSolicitudes + i)).id;
					valor=i;	
				}
				
			}		
		}
		if(valor==-1){
			for(i=0; i<numeroSolicitudes; i++){
				if((*(colaSolicitudes + i)).id<mayor && (*(colaSolicitudes + i)).id>0 && (*(colaSolicitudes + i)).atendido==0){
					mayor=(*(colaSolicitudes + i)).id;
					valor=i;
				}			
			}
		
		}	
	}else{	
		for(i=0; i<numeroSolicitudes; i++){
	        	if((*(colaSolicitudes + i)).id<mayor && (*(colaSolicitudes + i)).id>0 && (*(colaSolicitudes + i)).atendido==0){
				mayor=(*(colaSolicitudes + i)).id;
				valor=i;
				}	
		
		}		
	}
	(*(colaSolicitudes + valor)).atendido = 1;
	pthread_mutex_unlock(&mutexColaSolicitudes);
	(*(colaAtendedores + (tipo - 1))).atendiendo=1;
	sprintf(cad, "Atendedor %d Atiende solicitud %d", tipo, mayor);
	return valor;
}

// Función dedicada a calcular el tiempo de atención en función del porcentaje calculado previamente.
int tiempoAtencion(char *cad1, int porcentaje) {
	int tiempoAtendiendo;
	if(porcentaje <= 70) {
		tiempoAtendiendo = calculaAleatorios(1, 4);
		sprintf(cad1, "Atención sin errores.");
	} else if(porcentaje <= 90) {
		tiempoAtendiendo = calculaAleatorios(2, 6);
		sprintf(cad1, "Atención con errores.");
	}else if(porcentaje <= 100) {
		tiempoAtendiendo = calculaAleatorios(6, 10);
		sprintf(cad1, "Atención por antecedentes.");
	}
	return tiempoAtendiendo;
}



void *accionesCoordinadorSocial(){
	pthread_t nuevoHilo;
	while(1){
		pthread_mutex_lock(&mutexColaSocial);
		pthread_cond_wait(&cond, &mutexColaSocial);	
		pthread_mutex_lock(&mutexLog);
		writeLogMessage("Actividad", "Comenzando");
		pthread_mutex_unlock(&mutexLog);
		for(int i=0; i<4;i++){
			pthread_create(&nuevoHilo, NULL, actividadCultural, (void*)&colaSocial[i].id);
		}
		pthread_cond_wait(&cond, &mutexColaSocial);
		pthread_mutex_lock(&mutexLog);
		writeLogMessage("Actividad", "Finalizando");
		pthread_mutex_unlock(&mutexLog);
		contadorActividades=0;
		int aux;
		for(aux = 0; aux < 3; aux++) {
			colaSocial[aux].id=0;     		
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

//
int colaSolicitudesVacia() {
	int aux;
	for(aux = 0; aux < numeroSolicitudes; aux++) {
		if((*(colaSolicitudes + aux)).id != 0) {
			return 0;
		}
	}
	return 1;
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
	logFile = fopen("hola.log", "a");
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
	fclose(logFile);
	printf("[%s] %s: %s\n", stnow, id, msg);
}

