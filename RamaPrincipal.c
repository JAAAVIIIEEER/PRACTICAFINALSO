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

/*
 *        PRÁCTICA FINAL SISTEMAS OPERATIVOS - TSUNAMI DEMOCRÁTICO CULTURAL LEONÉS
 *  	
 *        AUTORES :        JAVIER PRÁDANOS FOMBELLIDA
 *			   RAÚL SEVILLANO GONZÁLEZ
 *			   JAVIER RENERO BALGAÑÓN
 *			   CARLOS DÍEZ GUTIÉRREZ
 * 
 *    ------------------------------------------------------------------------
 *	  -                           PARTE OPCIONAL                             -
 *	  ------------------------------------------------------------------------
 *	  PRIMER ARGUMENTO : NÚMERO MAXIMO DE SOLICITUDES
 *	  SEGUNDO ARGUMENTO : NÚMERO DE ATENDEDORES
 *	  
 *	  ENVIAR SEÑAL 13/SIGPIPE PARA EXPANDIR EL NÚMERO MÁXIMO DE SOLICITUDES
 *        ENVIAR SEÑAL 15/SIGTERM PARA EXPANDIR EL NUMERO DE ATENDEDORES	
 *
 */

// Declaración de variables globales.
pthread_mutex_t mutexColaSolicitudes;
pthread_mutex_t mutexColaAtendedores;
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
void accionesSolicitud(int posCola);
int espacioEnColaSolicitudes();
void solicitudTramitadaRechazada(int posicion);
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
	int aux;
	// Inicialización de variables locales, globales, condición y de los mutex.
	numeroSolicitudes = 15;
	numeroAtendedores = 3;
	contadorSolicitudes = 0;
	contadorActividades = 0;
	finalizar = 0;
	logFile = NULL;	
	fopen("registroTiempos.log", "w");
	
	//En caso de que sea el argumento el numero de solicitudes y atendedores
	if (argc == 3){
		numeroSolicitudes = atoi(argv[1]);
		numeroAtendedores = atoi(argv[2]);
	}

	colaSolicitudes = (Solicitud*)malloc(numeroSolicitudes*sizeof(Solicitud));

	//Inicilizamos las estructuras sigaction
	struct sigaction solicitudes = {0};
	solicitudes.sa_handler =  manejadoraNuevaSolicitud;
	struct sigaction terminar = {0};
	terminar.sa_handler = manejadoraTerminar;
	struct sigaction nuevosAtendedores = {0};
	nuevosAtendedores.sa_handler = manejadoraAumentoAtendedores;
	struct sigaction nuevasSolicitudes = {0};
	nuevasSolicitudes.sa_handler = manejadoraAumentoSolicitudes;

	//Inicilizamos las variables globales
	if(pthread_mutex_init(&mutexColaSolicitudes,NULL) != 0) {
		exit(-1);
	}
	if(pthread_mutex_init(&mutexColaAtendedores,NULL) != 0) {
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
		//colaSolicitudes[aux].id = 0;
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
	// Creación hilos de los atendedores y del coordinador social. Se utilizan mutex asegurar la protección de la variable aux.
	pthread_create(&coordinador, NULL, accionesCoordinadorSocial, NULL);
	pthread_mutex_lock(&mutexColaAtendedores);	
	for(aux = 0; aux < numeroAtendedores;) {
    		nuevoAtendedor(aux);
		pthread_mutex_lock(&mutexColaAtendedores);
		aux++;	
	}
	pthread_mutex_unlock(&mutexColaAtendedores);
	// Bucle de señales recibidas hasta que la manejadora de la señal SIGINT cambia el valor de la variable finalizar a 1.
	while(finalizar != 1) { 
		pause();
	}
	// Bucle en el cual se duerme al hilo principal 1 segundo constantemente hasta que se detecta que todas las solicitudes han sido procesadas.
	int cont;
	do{
		cont=0;
		//Analizamos la cola de solicitudes
		for(aux = 0; aux < numeroSolicitudes; aux++) {
			//Si el id es mayor que 0 es que hay una solicitud
    			if((*(colaSolicitudes+aux)).id>0)
				cont++;
		}
		sleep(1);
	}while(cont>0);
	pthread_mutex_destroy(&mutexColaAtendedores);
	pthread_mutex_destroy(&mutexColaSocial);
	pthread_mutex_destroy(&mutexColaSolicitudes);
	pthread_mutex_destroy(&mutexLog);
	free(colaSolicitudes);
	return 0;	
}

// Función manejadora de la señal SIGPIPE. Dedicada a poder aumentar la cola de solicitudes.
void manejadoraAumentoSolicitudes(int sig) {
	int aux = numeroSolicitudes;
	char * modificadoSolicitudes = (char *)malloc(200 * sizeof(char));
	
	pthread_mutex_lock(&mutexColaSolicitudes);
	printf("\033[1;32m");
	printf("Insertar nuevo numero de solicitudes: \n");
	printf("\033[0m");
	scanf("%d",&numeroSolicitudes);
	colaSolicitudes = (Solicitud *) realloc(colaSolicitudes, numeroSolicitudes*sizeof(Solicitud));
	for(aux; aux<numeroSolicitudes; aux++){
		(*(colaSolicitudes + aux)).id = 0;
		(*(colaSolicitudes + aux)).atendido = 0;
		(*(colaSolicitudes + aux)).tipo = 0;	
	}
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(modificadoSolicitudes, "Modificado a %d solicitudes.", numeroSolicitudes);
	pthread_mutex_lock(&mutexLog);
	writeLogMessage("Numero Solicitudes", modificadoSolicitudes);
	pthread_mutex_unlock(&mutexLog);
}

// Función manejadora de la señal SIGTERM. Dedicada a poder aumentar los atendedores disponibles, siendo estos nuevos de tipo PRO.
void manejadoraAumentoAtendedores(int sig) {
	int aux = numeroAtendedores;
	int auxSol = numeroSolicitudes;
	char * modificadoAtendedores = (char *)malloc(200 * sizeof(char));
	char * modificadoSolicitudes = (char *)malloc(200 * sizeof(char));

	pthread_mutex_lock(&mutexColaSolicitudes);
	printf("\033[1;32m");
	printf("Insertar nuevo numero de solicitudes: \n");
	printf("\033[0m");
	scanf("%d",&numeroSolicitudes);
	colaSolicitudes = (Solicitud *) realloc(colaSolicitudes, numeroSolicitudes*sizeof(Solicitud));
	for(auxSol; auxSol<numeroSolicitudes; auxSol++){
		(*(colaSolicitudes + auxSol)).id = 0;
		(*(colaSolicitudes + auxSol)).atendido = 0;
		(*(colaSolicitudes + auxSol)).tipo = 0;	
	}
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(modificadoSolicitudes, "Modificado a %d solicitudes.", numeroSolicitudes);
	pthread_mutex_lock(&mutexLog);
	writeLogMessage("Numero Solicitudes", modificadoSolicitudes);
	pthread_mutex_unlock(&mutexLog);

	pthread_mutex_lock(&mutexColaAtendedores);
	printf("\033[1;32m");
	printf("Insertar nuevo numero de atendedores: \n");
	printf("\033[0m");
	scanf("%d",&numeroAtendedores);
	pthread_mutex_unlock(&mutexColaAtendedores);
	sprintf(modificadoAtendedores, "Modificado a %d atendedores.", numeroAtendedores);
	pthread_mutex_lock(&mutexLog);
	writeLogMessage("Numero Atendedores", modificadoAtendedores);
	pthread_mutex_unlock(&mutexLog);

	//Se usan mutex como en el main para proteger la variable aux.
	pthread_mutex_lock(&mutexColaAtendedores);
	for(aux; aux < numeroAtendedores;) {
		nuevoAtendedor(aux);
		pthread_mutex_lock(&mutexColaAtendedores);
		aux++;
	}
	pthread_mutex_unlock(&mutexColaAtendedores);
}

// Función manejadora de la señal SIGINT.
void manejadoraTerminar(int sig) {
	finalizar = 1;
}

// Función manejadora de las señales SIGUSR1 y SIGUSR2.
void manejadoraNuevaSolicitud(int sig) {
	pthread_t hiloSolicitud;	
 	// Se genera el hilo correspondiente con la variable local hiloSolicitud.
	pthread_create(&hiloSolicitud, NULL, *nuevaSolicitud, (void *)&sig);
}

// Función dedicada a mantener el hilo solicitud en caso de existir espacio libre en la cola de solicitudes. En caso contrario se elimina.
void *nuevaSolicitud(void *sig) {
	int senal = (*(int *)sig);
	int posEspacioVacio;
	// Comprobación espacio libre en cola, en caso AFIRMATIVO el método espacioEnColaSolicitudes() devuelve la posición del espacio libre, por el contrario devuelve el valor -1, no entrando en el if y saliendo del metódo tras desbloquear el mutex y finalizando el hilo solicitud.
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
		pthread_mutex_unlock(&mutexColaSolicitudes);
		accionesSolicitud(posEspacioVacio);
		pthread_exit(NULL); 
	} else {  
		// Se desbloquea la cola colaSolicitudes si no hay espacio en dicha cola y se elimina el hilo solicitud.
		pthread_mutex_unlock(&mutexColaSolicitudes);
		pthread_exit(NULL);
	}
	
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

//Funcion que gestiona la solicitud en caso de que tenga espacio en la cola
void accionesSolicitud(int posicion){
	char * identificadorSolicitud = (char *)malloc((numeroSolicitudes + 10)* sizeof(char));
	char * eventoSolicitud = (char *)malloc(200 * sizeof(char));
	int actividad;
	// Se indica que una nueva solicitud ha sido añadida en la cola.
	sprintf(identificadorSolicitud, "Solicitud_%d", (*(colaSolicitudes + posicion)).id);
	sprintf(eventoSolicitud, "Registrada en la cola de solicitudes. ");
	//No hace falta que se bloquee para comprobar ya que son variables que no van a ser modificadas
	if((*(colaSolicitudes + posicion)).tipo == 1){
		strcat(eventoSolicitud, "Tipo de solicitud: Invitacion.");
	} else {
		strcat(eventoSolicitud, "Tipo de solicitud: QR.");
	}
	//Se bloquea el acceso a los logs.
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
				pthread_mutex_unlock(&mutexColaSolicitudes);
				if(calculaAleatorios(1, 10) == 1) {
					sprintf(eventoSolicitud, "Descartada del sistema por cansarse de esperar.");
					pthread_mutex_lock(&mutexLog);
					writeLogMessage(identificadorSolicitud, eventoSolicitud);
					pthread_mutex_unlock(&mutexLog);
                         		solicitudTramitadaRechazada(posicion);
				}		
			}
			// Se comprueba si la solicitud de tipo QR va a ser descartada por no considerarse fiable. Probabilidad 30%.
			if((*(colaSolicitudes + posicion)).tipo == 2) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				if(calculaAleatorios(1, 10) <= 3){
					sprintf(eventoSolicitud, "Descartada del sistema por no considerarse fiable.");
					pthread_mutex_lock(&mutexLog);
					writeLogMessage(identificadorSolicitud, eventoSolicitud);
					pthread_mutex_unlock(&mutexLog);
					solicitudTramitadaRechazada(posicion);
				}
			}
			// Como la solicitud no ha sido descartada hasta ahora, se comprueba si va a ser descartada por mal funcionamiento de la aplicación. Probabilidad 15%.
			if(calculaAleatorios(1, 20) <= 3){
				sprintf(eventoSolicitud, "Descartada por mal funcionamiento de la aplicacion.");
				pthread_mutex_lock(&mutexLog);
				writeLogMessage(identificadorSolicitud, eventoSolicitud);
				pthread_mutex_unlock(&mutexLog);
                         	solicitudTramitadaRechazada(posicion);
			}
		//En caso de que este siendo atendida la solicitud              			
		} else {	
			while((*(colaSolicitudes + posicion)).atendido == 1) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				sleep(1);
				pthread_mutex_lock(&mutexColaSolicitudes);	
			}
			//En caso de que se pueda vincular a una actividad cultural
			if((*(colaSolicitudes + posicion)).atendido == 2) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				sprintf(eventoSolicitud, "Solicitando vinculacion con la actividad cultural...");
				pthread_mutex_lock(&mutexLog); 
				writeLogMessage(identificadorSolicitud, eventoSolicitud);
				pthread_mutex_unlock(&mutexLog);
				actividad = calculaAleatorios(1, 2);
				//Si se vincula a una actividad cultural
				if(actividad == 1) {
					sprintf(eventoSolicitud, "Vinculada a la actividad cultural.");
					pthread_mutex_lock(&mutexLog); 
					writeLogMessage(identificadorSolicitud, eventoSolicitud);
					pthread_mutex_unlock(&mutexLog);
					pthread_mutex_lock(&mutexColaSocial);
					while(contadorActividades == 4) {
						pthread_mutex_unlock(&mutexColaSocial);
						sleep(1);
						pthread_mutex_lock(&mutexColaSocial);
					}			
					//Entra en la cola actividades
					pthread_mutex_lock(&mutexColaSolicitudes);			
					colaSocial[contadorActividades++].id = (*(colaSolicitudes + posicion)).id;
					pthread_mutex_unlock(&mutexColaSocial);
					pthread_mutex_unlock(&mutexColaSolicitudes);		
					if(contadorActividades==4){
						pthread_cond_signal(&cond);
					}				
  					solicitudTramitadaRechazada(posicion);
				//En caso de que no se vincule a una actividad cultural
				} else {
					sprintf(eventoSolicitud, "Tramitada sin actividad cultural.");
					pthread_mutex_lock(&mutexLog);	
					writeLogMessage(identificadorSolicitud, eventoSolicitud);
					pthread_mutex_unlock(&mutexLog);
					solicitudTramitadaRechazada(posicion);
				}

			//En caso de que sea con antecedentes y no se pueda vincular			
			} else if((*(colaSolicitudes + posicion)).atendido == 3) {
				pthread_mutex_unlock(&mutexColaSolicitudes);
				sprintf(eventoSolicitud, "Tramitada sin actividad cultural.");
				pthread_mutex_lock(&mutexLog);	
				writeLogMessage(identificadorSolicitud, eventoSolicitud);
				pthread_mutex_unlock(&mutexLog);	
				solicitudTramitadaRechazada(posicion);

			}
		}

	}

}

//Funcion que se encarga de liberar el espacio en la cola de solicitudes
void solicitudTramitadaRechazada(int posicion) {
  	pthread_mutex_lock(&mutexColaSolicitudes);
  	(*(colaSolicitudes + posicion)).tipo=0;
  	(*(colaSolicitudes + posicion)).id=0;
  	(*(colaSolicitudes + posicion)).atendido=0;
  	pthread_mutex_unlock(&mutexColaSolicitudes);
  	pthread_exit(NULL);
}

//Funcion que se encarga de crear nuevos atendedores
void nuevoAtendedor(int posEnColaAtendedor) {
	pthread_t nuevoAtendedor;
	pthread_create(&nuevoAtendedor, NULL, accionesAtendedor, (void*)&posEnColaAtendedor);		
}


//Funcion encargada de hacer que los atendedores atiendan las solicitudes
void *accionesAtendedor(void *posEnColaAtendedor) {
        int contadorVecesAtiende = 0, posEnColaSolicitud = 0, porcentaje = 0, flagAtendido = 0, tiempoDeAtencion = 0, posAtendedor = (*(int *)posEnColaAtendedor)+1, descanso = 1;
	pthread_mutex_unlock(&mutexColaAtendedores);
	char * identificador = (char *) malloc((10 + numeroAtendedores) * sizeof(char));
	sprintf(identificador, "Atendedor_%d", posAtendedor);
	char * evento = (char *) malloc(200 * sizeof(char));
	
    	while(finalizar != 1 || posEnColaSolicitud != -1) {
		posEnColaSolicitud = buscadorPorTipos(posAtendedor,evento);
		if(posEnColaSolicitud == -1){
			sleep(1);
		} else {
			porcentaje = calculaAleatorios(1, 100);
			flagAtendido = tipoDeAtencion(porcentaje);

			pthread_mutex_lock(&mutexLog); 
			writeLogMessage(identificador, evento);
			pthread_mutex_unlock(&mutexLog);

			tiempoDeAtencion = tiempoAtencion(evento, porcentaje,posEnColaSolicitud);	
					
			sleep(tiempoDeAtencion);

			pthread_mutex_lock(&mutexLog); 
			writeLogMessage(identificador, evento);	
			pthread_mutex_unlock(&mutexLog);
			pthread_mutex_lock(&mutexColaSolicitudes);			
			(*(colaSolicitudes + posEnColaSolicitud)).atendido = flagAtendido;
			pthread_mutex_unlock(&mutexColaSolicitudes);
			contadorVecesAtiende++;
			if(contadorVecesAtiende == 5) {
				while(descanso == 1)
				{
				contadorVecesAtiende=0;
				sprintf(evento, "Inicio descanso de 10 segundos.");
				pthread_mutex_lock(&mutexLog); 
				writeLogMessage(identificador, evento);
				pthread_mutex_unlock(&mutexLog);				
				sleep(10);
				sprintf(evento, "Fin descanso de 10 segundos.");	
				pthread_mutex_lock(&mutexLog); 
				writeLogMessage(identificador, evento);
				pthread_mutex_unlock(&mutexLog);
				descanso = 0;
				}	
			}		
		}      
   	}
	pthread_exit(NULL);
}

// Función dedicada a calcular el tipo de atención para cada solicitud según el porcentaje. En solo un 10% de los casos devolverá un valor de 3 lo cual indica que la solicitud tiene antecedentes y no puede intentar entrar en una actividad. 
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
	pthread_mutex_unlock(&mutexColaSolicitudes);
	sprintf(evento, "Atendiendo Solicitud_%d...",(*(colaSolicitudes + posEnCola)).id);
	return posEnCola;
}

// Función auxiliar a buscadorPorTipos para casos especiales en los que un atendedor de tipo 1 o 2 no encuentre solicitudes de su mismo tipo.
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


//Funcion dedicada a desencadenar las actividades culturales.
void *accionesCoordinadorSocial(){
	pthread_t hiloActividad;
	while(1){
		int i;
		pthread_mutex_lock(&mutexColaSocial);
		pthread_cond_wait(&cond, &mutexColaSocial);	
		pthread_mutex_lock(&mutexLog);
		writeLogMessage("Actividad cultural", "Comenzando...");
		pthread_mutex_unlock(&mutexLog);
		//Creamos los hilos que desencadenarán la actividad cultural
		for(i = 0; i < 4;i++) {
			pthread_create(&hiloActividad, NULL, actividadCultural, (void*)&colaSocial[i].id);
		}
		pthread_cond_wait(&cond, &mutexColaSocial);
		pthread_mutex_lock(&mutexLog);
		writeLogMessage("Actividad cultural", "Finalizando...");
		pthread_mutex_unlock(&mutexLog);
		contadorActividades = 0;
		pthread_mutex_unlock(&mutexColaSocial);
	}
}

//Funcion dedicada a que cada hilo de actividad desencadene esta misma.
void *actividadCultural(void *id){
	char * identificador = malloc(80 * sizeof(char));
	char * evento = malloc(80 * sizeof(char));
	int idAux = *(int *)id;
	int i, contador=0;
	sleep(3);
	sprintf(evento, "Fin actividad cultural");
	sprintf(identificador, "Solicitud_%d", idAux);
	pthread_mutex_lock(&mutexLog); 
	writeLogMessage(identificador, evento);
	pthread_mutex_unlock(&mutexLog);
	pthread_mutex_lock(&mutexColaSocial); 
	for(i = 0; i < 4; i++) {
		if(colaSocial[i].id == idAux){
			colaSocial[i].id = 0;
		}
	}
	for(i = 0; i < 4;i++) {
		if(colaSocial[i].id == 0) { 
			contador++;
		}
	}
	if(contador == 4){
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&mutexColaSocial); 
	pthread_exit(NULL);
}



// Función dedicada a calcular un número aleatorio comprendido entre un número y otro número.
int calculaAleatorios(int min, int max) {
	srand(time(NULL));
	return rand() % (max - min + 1) + min;	
}


// Función dedicada a escribir en log. Recibe como parámetros un identificador del mensaje log y el evento ocurrido.
void writeLogMessage(char *id, char *msg) { 
	// Se calcula la hora actual.
	time_t now = time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal);
	// Se escribe en el log. 
	logFile = fopen("registroTiempos.log", "a");
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
	fclose(logFile);
	printf("[%s] %s: %s\n", stnow, id, msg);
}
