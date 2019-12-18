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
void manejadora_inv(int sig);
void manejadora_qr(int sig);
void manejadora_ter(int sig);
void nuevaSolicitud(int sig);
void writeLogMessage(char *id, char *msg);
pid_t gettid(void);
int calculaAleatorios(int min, int max);
void *AccionesAtendedor(void *num);

int contadorSolicitud;
pthread_mutex_t cola;
pthread_mutex_t fichero;
pthread_mutex_t atendedore;


typedef struct{
 int id; 
 int atendido; 
 int tipo; 
 int hilo;
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


void manejadora_inv(int sig){
  printf("Invitacion inv\n");
  nuevaSolicitud(sig);

}

void manejadora_qr(int sig){
  printf("Invitacion qr\n");
  nuevaSolicitud(sig);
}

void manejadora_ter(int sig){

 printf("Invitacion rechazada\n");

}



int main(){

   solicitud s;
   atendedor a;


   struct sigaction ss = {0};
    ss.sa_handler=manejadora_inv;
    struct sigaction ll = {0};
    ll.sa_handler=manejadora_qr;
    struct sigaction hh = {0};
    hh.sa_handler=manejadora_ter;

    sigaction(SIGUSR1, &ss, NULL);
    sigaction(SIGUSR2, &ll, NULL);
    sigaction(SIGINT, &hh, NULL);
    
    if (pthread_mutex_init(&cola, NULL)!=0){
 	exit(-1);
 	}
   if (pthread_mutex_init(&fichero, NULL)!=0){
 	exit(-1);
	} 
   if (pthread_mutex_init(&atendedore, NULL)!=0){
 	exit(-1);
	}
    pthread_cond_t no_lleno;
    pthread_cond_init(&no_lleno, NULL);

    contadorSolicitud=0;
    s.id=1;
    s.atendido=0;
    s.tipo=0;

    a.atendiendo=0;
    a.tipo=0;

    pthread_t atendedor1, atendedor2, atendedor3; 
    int inv=1, qr=2, both=3;
    pthread_create(&atendedor1, NULL, AccionesAtendedor, (void*)&inv);
    pthread_create(&atendedor2, NULL, AccionesAtendedor, (void*)&qr);
    pthread_create(&atendedor3, NULL, AccionesAtendedor, (void*)&both);

   while(1){
      pause();

    }

}


void nuevaSolicitud(int sig){
     pthread_mutex_lock(&cola);
     int i=0;
    
     while(solicitudes[i].id!=0){

      i++;
      if(i==15){
	break;
	}

     }
  printf("El numero %d\n", i);
     if(i==15){
         printf("La cola esta llena\n");
        
       }else{
     
      contadorSolicitud=contadorSolicitud+1;
      solicitudes[i].id=contadorSolicitud;
      solicitudes[i].atendido=0;
      if(sig==SIGUSR1){
        solicitudes[i].tipo=1;
      }else if(sig==SIGUSR2){
        solicitudes[i].tipo=2;
      }
       pthread_t solicitud;
       pthread_create(&solicitud, NULL, AccionesSolicitud, (void*)&i);

    }


    for(int j=0; j<15; j++){
      printf("El id es: %d\n", solicitudes[j].id);


     }
pthread_mutex_unlock(&cola);
   
}


pid_t gettid(void) {
   return syscall(__NR_gettid);
 } 


void *AccionesSolicitud(void *num){

 printf("SIUUUUUUU PID=%d, SPID=%d\n", getpid(), gettid());
pthread_mutex_lock(&cola);
int posicion =*(int *)num;
pthread_mutex_unlock(&cola);
char * cad = malloc(12 * sizeof(char));
char * cad1 = malloc(12 * sizeof(char));

pthread_mutex_lock(&cola);
int  n=solicitudes[posicion].id; //aqui creo que no debria haber mutex
int n1=solicitudes[posicion].tipo;
pthread_mutex_unlock(&cola);
sprintf(cad, "%i", n);
sprintf(cad1, "%i", n1);
  pthread_mutex_lock(&fichero);  //voy a escribir en en el fichero por tanto mutex
  writeLogMessage(cad, cad1);
  pthread_mutex_unlock(&fichero);

while(1){
sleep(4);

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
		
	}

}

  }

void *AccionesAtendedor(void *num){
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
